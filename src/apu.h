#ifndef APU_H
#define APU_H

#include "main.h"

#define AUDIO_BUFFER_SIZE 44100
#define SAMPLE_RATE 44100 // CPU cycles per second

typedef struct audio_buffer {
    float audio_buffer[AUDIO_BUFFER_SIZE];
    uint32_t audio_buffer_pos;
    AudioStream audio_stream;
} audio_buffer_t;

typedef struct apu {
    // APU state will go here
    uint64_t prev_clock;
    audio_buffer_t buffer;
} apu_t;

void init_apu(apu_t* a);
float tick_apu(apu_t* a, gb_t* s);

#endif