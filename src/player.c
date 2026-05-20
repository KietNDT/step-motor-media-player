// src/player.c — Hybrid with Song3 registry, button next song, per‑motor volume
#include "player.h"
#include "songs.h"          // now defines Song3 and song_list[]
#include "stm32f4xx.h"

// ---------------------------------------------------------------------------
// PER‑MOTOR VOLUME (0‑100)
// ---------------------------------------------------------------------------
static uint8_t vol_melody = 80;
static uint8_t vol_harmony = 60;
static uint8_t vol_bass = 50;

// ---------------------------------------------------------------------------
// 3-VOICE SEQUENCER STATE
// ---------------------------------------------------------------------------
typedef struct {
    const Step*       steps;
    uint16_t          length;
    volatile uint16_t note_idx;
    volatile uint16_t note_ms;
    volatile uint8_t  load_next;
} VoiceState;

static volatile uint8_t  current_song_idx = 0;
static VoiceState voice_melody  = { 0, 0, 0, 0, 1 };
static VoiceState voice_harmony = { 0, 0, 0, 0, 1 };
static VoiceState voice_bass    = { 0, 0, 0, 0, 1 };

static volatile uint32_t ms_tick = 0;

// ---------------------------------------------------------------------------
// CLOCK — HSI → PLL → 100MHz
// ---------------------------------------------------------------------------
static void clock_init(void) {
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY));
    RCC->PLLCFGR = (16<<0)|(200<<6)|(0<<16)|(0<<22);
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));
    FLASH->ACR = FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN | 3;
    RCC->CFGR  = (0<<4)|(4<<10)|(0<<13);
    RCC->CFGR |= 2;
    while (((RCC->CFGR>>2)&3) != 2);
}

// ---------------------------------------------------------------------------
// DIR PINS (PB0, PB8, PB2) — set low (forward)
// ---------------------------------------------------------------------------
static void dir_pins_init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    GPIOB->MODER &= ~((3<<0) | (3<<16) | (3<<4));
    GPIOB->MODER |=  ((1<<0) | (1<<16) | (1<<4));
    GPIOB->ODR &= ~((1<<0) | (1<<8) | (1<<2));
}

// ---------------------------------------------------------------------------
// TIM1 — PWM on PA8 (TIM1_CH1) — Melody
// ---------------------------------------------------------------------------
static void tim1_init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    GPIOA->MODER  = (GPIOA->MODER & ~(3<<16)) | (2<<16);
    GPIOA->AFR[1] = (GPIOA->AFR[1] & ~0xF) | 1;
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
    TIM1->PSC   = 99;
    TIM1->ARR   = NOTE_ARR[NOTE_A4];
    TIM1->CCR1  = 0;
    TIM1->CCMR1 = (6<<4)|(1<<3);
    TIM1->CCER  = TIM_CCER_CC1E;
    TIM1->BDTR  = TIM_BDTR_MOE;
    TIM1->CR1  |= TIM_CR1_ARPE;
    TIM1->EGR   = TIM_EGR_UG;
    TIM1->CR1  |= TIM_CR1_CEN;
}

// ---------------------------------------------------------------------------
// TIM2 — PWM on PA5 (TIM2_CH1) — Harmony
// ---------------------------------------------------------------------------
static void tim2_init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    GPIOA->MODER  = (GPIOA->MODER & ~(3<<10)) | (2<<10);
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~(0xF<<20)) | (1<<20);
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    TIM2->PSC   = 99;
    TIM2->ARR   = NOTE_ARR[NOTE_A4];
    TIM2->CCR1  = 0;
    TIM2->CCMR1 = (6<<4)|(1<<3);
    TIM2->CCER  = TIM_CCER_CC1E;
    TIM2->CR1  |= TIM_CR1_ARPE;
    TIM2->EGR   = TIM_EGR_UG;
    TIM2->CR1  |= TIM_CR1_CEN;
}

// ---------------------------------------------------------------------------
// TIM3 — PWM on PA6 (TIM3_CH1) — Bass
// ---------------------------------------------------------------------------
static void tim3_init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    GPIOA->MODER  = (GPIOA->MODER & ~(3<<12)) | (2<<12);
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~(0xF<<24)) | (2<<24);
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    TIM3->PSC   = 99;
    TIM3->ARR   = NOTE_ARR[NOTE_A4];
    TIM3->CCR1  = 0;
    TIM3->CCMR1 = (6<<4)|(1<<3);
    TIM3->CCER  = TIM_CCER_CC1E;
    TIM3->CR1  |= TIM_CR1_ARPE;
    TIM3->EGR   = TIM_EGR_UG;
    TIM3->CR1  |= TIM_CR1_CEN;
}

// ---------------------------------------------------------------------------
// TIM4 — 1ms tick IRQ
// ---------------------------------------------------------------------------
static void tim4_init(void) {
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
    TIM4->PSC  = 999;
    TIM4->ARR  = 99;
    TIM4->DIER = TIM_DIER_UIE;
    TIM4->EGR  = TIM_EGR_UG;
    TIM4->SR   = 0;
    TIM4->CR1  = TIM_CR1_CEN;
    NVIC_SetPriority(TIM4_IRQn, 1);
    NVIC_EnableIRQ(TIM4_IRQn);
}

// ---------------------------------------------------------------------------
// SMOOTH NOTE PLAY (no timer stop) WITH PER‑MOTOR VOLUME
// ---------------------------------------------------------------------------
static inline void play_note_on_timer(TIM_TypeDef* tim, uint8_t note, uint8_t vol) {
    uint32_t arr = NOTE_ARR[note];
    if (arr == 0) {
        tim->CCR1 = 0;
        tim->EGR = TIM_EGR_UG;
        return;
    }
    tim->ARR  = arr;
    tim->CCR1 = (arr * vol) / 200;
    tim->EGR  = TIM_EGR_UG;
}

static inline void play_motor1(uint8_t note) { play_note_on_timer(TIM1, note, vol_melody); }
static inline void play_motor2(uint8_t note) { play_note_on_timer(TIM2, note, vol_harmony); }
static inline void play_motor3(uint8_t note) { play_note_on_timer(TIM3, note, vol_bass); }

// ---------------------------------------------------------------------------
// VOICE TICK
// ---------------------------------------------------------------------------
static inline void tick_voice(VoiceState* v, void (*play_fn)(uint8_t)) {
    if (!v->steps || v->length == 0) return;
    if (v->load_next) {
        v->load_next = 0;
        play_fn(v->steps[v->note_idx].note);
        v->note_ms = v->steps[v->note_idx].dur_ms;
        v->note_idx++;
        if (v->note_idx >= v->length) v->note_idx = 0;   // loop
    }
    if (v->note_ms > 0) v->note_ms--;
    if (v->note_ms == 0) v->load_next = 1;
}

// ---------------------------------------------------------------------------
// TIM4 ISR
// ---------------------------------------------------------------------------
void TIM4_IRQHandler(void) {
    TIM4->SR &= ~TIM_SR_UIF;
    ms_tick++;
    tick_voice(&voice_melody,  play_motor1);
    tick_voice(&voice_harmony, play_motor2);
    tick_voice(&voice_bass,    play_motor3);
}

// ---------------------------------------------------------------------------
// BUTTON (PD0) — next song
// ---------------------------------------------------------------------------
static void button_init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    GPIOD->MODER &= ~(3<<0);          // input
    GPIOD->PUPDR = (GPIOD->PUPDR & ~(3<<0)) | (1<<0);  // pull‑up
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    SYSCFG->EXTICR[0] = (SYSCFG->EXTICR[0] & ~(0xF<<0)) | (3<<0); // PD0 -> EXTI0
    EXTI->IMR |= (1<<0);
    EXTI->RTSR |= (1<<0);             // rising edge
    EXTI->FTSR &= ~(1<<0);
    NVIC_SetPriority(EXTI0_IRQn, 2);
    NVIC_EnableIRQ(EXTI0_IRQn);
}

void EXTI0_IRQHandler(void) {
    EXTI->PR = (1<<0);
    static uint32_t last_press = 0;
    if (ms_tick - last_press < 300) return;
    last_press = ms_tick;
    player_next_song();
}

// ---------------------------------------------------------------------------
// PUBLIC API
// ---------------------------------------------------------------------------
void player_init(void) {
    clock_init();
    dir_pins_init();
    tim1_init();
    tim2_init();
    tim3_init();
    tim4_init();
    button_init();
}

// Load a song by index from the registry
void player_play_song(uint8_t idx) {
    if (idx >= SONG_COUNT) return;
    const Song3* s = &song_list[idx];
    // Stop all motors
    TIM1->CCR1 = 0; TIM2->CCR1 = 0; TIM3->CCR1 = 0;
    TIM1->EGR = TIM2->EGR = TIM3->EGR = TIM_EGR_UG;

    voice_melody.steps   = s->melody;
    voice_melody.length  = s->melody_len;
    voice_melody.note_idx = 0;
    voice_melody.note_ms  = 0;
    voice_melody.load_next = 1;

    voice_harmony.steps   = s->harmony;
    voice_harmony.length  = s->harmony_len;
    voice_harmony.note_idx = 0;
    voice_harmony.note_ms  = 0;
    voice_harmony.load_next = 1;

    voice_bass.steps   = s->bass;
    voice_bass.length  = s->bass_len;
    voice_bass.note_idx = 0;
    voice_bass.note_ms  = 0;
    voice_bass.load_next = 1;

    current_song_idx = idx;
}

void player_next_song(void) {
    uint8_t next = (current_song_idx + 1) % SONG_COUNT;
    player_play_song(next);
}

void player_play_3ch(const Step* melody, const Step* harmony, const Step* bass,
                     uint16_t mel_len, uint16_t har_len, uint16_t bas_len) {
    // Direct play (used only if you bypass song registry)
    TIM1->CCR1 = 0; TIM2->CCR1 = 0; TIM3->CCR1 = 0;
    TIM1->EGR = TIM2->EGR = TIM3->EGR = TIM_EGR_UG;

    voice_melody.steps   = melody;   voice_melody.length  = mel_len;
    voice_melody.note_idx = 0;       voice_melody.note_ms  = 0;   voice_melody.load_next = 1;
    voice_harmony.steps  = harmony;  voice_harmony.length = har_len;
    voice_harmony.note_idx = 0;      voice_harmony.note_ms = 0;   voice_harmony.load_next = 1;
    voice_bass.steps     = bass;     voice_bass.length    = bas_len;
    voice_bass.note_idx  = 0;        voice_bass.note_ms   = 0;    voice_bass.load_next = 1;

    // not updating current_song_idx, but that's fine
}

void player_set_volume_channel(MotorChannel channel, uint8_t vol) {
    if (vol > 100) vol = 100;
    switch (channel) {
        case MOTOR_MELODY:  vol_melody  = vol; break;
        case MOTOR_HARMONY: vol_harmony = vol; break;
        case MOTOR_BASS:    vol_bass    = vol; break;
        default: break;
    }
}

void player_set_volume(uint8_t vol) {
    player_set_volume_channel(MOTOR_MELODY, vol);
    player_set_volume_channel(MOTOR_HARMONY, vol);
    player_set_volume_channel(MOTOR_BASS, vol);
}

void player_stop(void) {
    TIM1->CCR1 = 0; TIM2->CCR1 = 0; TIM3->CCR1 = 0;
    TIM1->EGR = TIM2->EGR = TIM3->EGR = TIM_EGR_UG;
    voice_melody.steps = voice_harmony.steps = voice_bass.steps = NULL;
}

// Legacy stub (single‑motor not used)
void player_play(uint8_t idx) { (void)idx; }