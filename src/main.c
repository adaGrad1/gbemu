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

struct dim {
    size_t h;
    size_t w;
};

typedef struct colormap {
    uint8_t map[4][3] ;
} colormap_t;

colormap_t get_cmap(uint8_t idx){
    colormap_t toret = {0};
    switch(idx % 4){
        case 0: // Amber
            return (colormap_t) {
                map: {
                    {255, 191, 0},   // Bright amber (pixel_val 0)
                    {204, 153, 0},   // Medium amber (pixel_val 1) 
                    {102, 76, 0},    // Dark amber (pixel_val 2)
                    {0, 0, 0}        // Black (pixel_val 3)
                }
            };
        case 1: // Classic Game Boy Green
            return (colormap_t) {
                map: {
                    {155, 188, 15},  // Lightest green (pixel_val 0)
                    {139, 172, 15},  // Light green (pixel_val 1)
                    {48, 98, 48},    // Dark green (pixel_val 2)
                    {15, 56, 15}     // Darkest green (pixel_val 3)
                }
            };
        case 2: // Game Boy Light Off-White
            return (colormap_t) {
                map: {
                    {248, 248, 220}, // Off-white (pixel_val 0)
                    {224, 224, 204}, // Light gray (pixel_val 1)
                    {136, 136, 128}, // Medium gray (pixel_val 2)
                    {52, 52, 48}     // Dark gray (pixel_val 3)
                }
            };
        case 3: // Blue Tint
            return (colormap_t) {
                map: {
                    {224, 248, 255}, // Light blue-white (pixel_val 0)
                    {176, 196, 222}, // Light blue-gray (pixel_val 1)
                    {72, 108, 144},  // Medium blue (pixel_val 2)
                    {24, 36, 48}     // Dark blue (pixel_val 3)
                }
            };
    }
}

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
    // init_apu(apu);
        
    // Initialize Game Boy state after boot ROM
    
    FILE *fp = fopen("./roms/sml.gb", "rb");
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
    
    
    // Create texture for PPU framebuffer
    Image ppu_image = GenImageColor(WIDTH, HEIGHT, BLACK);
    Texture2D ppu_texture = LoadTextureFromImage(ppu_image);
    UnloadImage(ppu_image);
    gameboy_state->pc = 0x100;
    uint8_t colormap_idx = 0;
    uint64_t frames = 0;
    while (!WindowShouldClose()) {
        // Handle backtick key press to cycle colormaps
        if (IsKeyPressed(KEY_GRAVE)) {
            colormap_idx++;
        }
        printf("frame %d\n", frames++);
        for(int scanline = 0; scanline < 154; scanline++){
            gameboy_state->ram[0xFF44] = scanline;
            while(gameboy_state->cycles < 456){
                handle_interrupts(gameboy_state);
                uint8_t c = step(gameboy_state);
                gameboy_state->cycles += c;
                
                // Generate audio samples for each CPU cycle
                // float sample = tick_apu(apu, gameboy_state);
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
        colormap_t c = get_cmap(colormap_idx);
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                uint8_t pixel_val = ppu->display[y][x];
                // Convert Game Boy 2-bit color to grayscale
                // Classic amber monochrome palette: bright orange -> dark orange -> black

                pixels[y * WIDTH + x] = (Color){ 
                    c.map[pixel_val][0], 
                    c.map[pixel_val][1], 
                    c.map[pixel_val][2], 
                    255 
                };
            }
        }
        UpdateTexture(ppu_texture, pixels);

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTextureEx(ppu_texture, (Vector2){0, 0}, 0.0f, WIN_SCALE, WHITE);
        EndDrawing();
    }
    
    UnloadTexture(ppu_texture);
    CloseAudioDevice();
    CloseWindow();

    return EXIT_SUCCESS;
}
