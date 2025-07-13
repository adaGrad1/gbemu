#include <stdio.h>
#include <raylib.h>

#include "apu.h"
#include "math.h"

void init_buffer(audio_buffer_t* buf){
    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(AUDIO_BUFFER_SIZE);
    buf->audio_stream = LoadAudioStream(SAMPLE_RATE, 32, 1);
    PlayAudioStream(buf->audio_stream);
}

void init_apu(apu_t* a){
    init_buffer(&a->buffer);
}

void push_audio_buffer(float sample, audio_buffer_t* buf){
    if(buf->audio_buffer_pos < AUDIO_BUFFER_SIZE)
        buf->audio_buffer[buf->audio_buffer_pos++] = sample;
    
    // Submit buffer when full
    if (buf >= AUDIO_BUFFER_SIZE / 2) {
        if (IsAudioStreamProcessed(buf->audio_stream)) {
            UpdateAudioStream(buf->audio_stream, buf->audio_buffer, buf->audio_buffer_pos);
            buf->audio_buffer_pos = 0;
        }
    }
}


float tick_apu(apu_t* a, gb_t* s) {
    // For now, return silence
    // TODO: implement 4 channel audio synthesis
    uint16_t cycles = s->cycles_total - a->prev_clock;
    a->prev_clock = s->cycles_total;
    for(uint8_t i = 0; i < cycles; i++){
        push_audio_buffer(16000*sin(s->cycles_total / 14.0), &a->buffer);
    }
    // return 16000*sin(s->cycles_total / 44.0);

}