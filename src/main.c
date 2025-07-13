#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>

#include <cJSON.h>
#include <raylib.h>

#include "util.h"
#include "json_test.h"
#include "instr_helpers.h"
#include "instr.h"
#include "ppu.h"
#include "main.h"
#include "cpu.h"
#include "mmu.h"
#include "apu.h"


#define SM83_DIR "./sm83/v1/"
#define WINDOW_TITLE "Game Boy Emulator"
#define TARGET_FPS 60
#define WIN_SCALE 4
#define AUDIO_BUFFER_SIZE 4213440
#define SAMPLE_RATE 4213440 // CPU cycles per second

struct dim {
    size_t h;
    size_t w;
};

int main(int argc, char* argv[]) {
    if (argc > 1 && !strcmp(argv[1], "test")){
        json_test_main(argc-1, argv+1);
        exit(0);
    }
    // PPU uses overdrawn 256x256 screen
    struct dim vdim = { .w = WIDTH, .h = HEIGHT };
    // physical screen
    struct dim pdim = { .w = vdim.w * WIN_SCALE, .h = vdim.h * WIN_SCALE };
    gb_t *gameboy_state = calloc(1, sizeof(gb_t));
    ppu_t *ppu = calloc(1, sizeof(ppu_t));
    apu_t *apu = calloc(1, sizeof(apu_t));
    
    // Audio buffer for collecting samples
    float *audio_buffer = calloc(AUDIO_BUFFER_SIZE, sizeof(float));
    int audio_buffer_pos = 0;
    
    // Initialize Game Boy state after boot ROM
    
    FILE *fp = fopen("./roms/kirby.gb", "rb");
    if (!fp) {
        printf("File not found!\n");
        exit(1);
    }
    int bytesRead =
      fread(gameboy_state->rom, 1, 1 << 20, fp);
    fclose(fp);
    mmu_init(gameboy_state);

    InitWindow(pdim.w, pdim.h, WINDOW_TITLE);
    SetTargetFPS(TARGET_FPS);
    
    // Initialize audio
    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(AUDIO_BUFFER_SIZE);
    AudioStream audio_stream = LoadAudioStream(SAMPLE_RATE, 32, 1);
    PlayAudioStream(audio_stream);
    
    // Create texture for PPU framebuffer
    Image ppu_image = GenImageColor(WIDTH, HEIGHT, BLACK);
    Texture2D ppu_texture = LoadTextureFromImage(ppu_image);
    UnloadImage(ppu_image);
    gameboy_state->pc = 0x100;
    while (!WindowShouldClose()) {
        for(int scanline = 0; scanline < 154; scanline++){
            gameboy_state->ram[0xFF44] = scanline;
            while(gameboy_state->cycles < 456){
                handle_interrupts(gameboy_state);
                uint8_t c = step(gameboy_state);
                gameboy_state->cycles += c;
                
                // Generate audio samples for each CPU cycle
                float sample = tick_apu(apu, gameboy_state);
                for (int cycle = 0; cycle < c; cycle++) {
                    if(audio_buffer_pos < AUDIO_BUFFER_SIZE)
                        audio_buffer[audio_buffer_pos++] = sample;
                    
                    // Submit buffer when full
                    if (audio_buffer_pos >= AUDIO_BUFFER_SIZE / 2) {
                        if (IsAudioStreamProcessed(audio_stream)) {
                            UpdateAudioStream(audio_stream, audio_buffer, audio_buffer_pos);
                            audio_buffer_pos = 0;
                        }
                    }
                }
            }
            if (scanline == 144) {
                gameboy_state->ram[0xFF0F] |= 1;
            }
            gameboy_state->cycles -= 456;
            if(scanline < 144) {
                ppu->scanline = scanline;
                update_ppu(ppu, gameboy_state);
            }
        }

        // Convert PPU display buffer to raylib texture
        Color pixels[HEIGHT * WIDTH];
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                uint8_t pixel_val = ppu->display[y][x];
                // Convert Game Boy 2-bit color to grayscale
                uint8_t gray_val = 255 - (pixel_val * 85); // 0->255, 1->170, 2->85, 3->0
                pixels[y * WIDTH + x] = (Color){ gray_val, gray_val, gray_val, 255 };
            }
        }
        UpdateTexture(ppu_texture, pixels);

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTextureEx(ppu_texture, (Vector2){0, 0}, 0.0f, WIN_SCALE, WHITE);
        EndDrawing();
    }
    
    UnloadTexture(ppu_texture);
    UnloadAudioStream(audio_stream);
    CloseAudioDevice();
    CloseWindow();

    return EXIT_SUCCESS;
}
