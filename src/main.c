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

#include <cJSON.h>
#include <raylib.h>

#include "util.h"
#include "jtest.h"
#include "instr.h"
#include "ppu.h"
#include "main.h"


#define SM83_DIR "./sm83/v1/"
#define WINDOW_TITLE "Game Boy Emulator"
#define TARGET_FPS 10
#define WIN_SCALE 5

struct dim {
    size_t h;
    size_t w;
};

// BSS reserve
#define FILE_BUF_LEN 0x10000
char file_buf[FILE_BUF_LEN] = {0};

int cmp_str (const void *a, const void *b) {
    return strcmp(*(char**)a, *(char**)b);
}

int test_file_list(char **filenames, size_t *filecount) {
    int error = 0;
    struct dirent *dir = {0};
    DIR *d = {0};
    assert (filenames != NULL && "Failed to alloc filenames");

    d = opendir(SM83_DIR); // Open the current directory
    int n_files = 0;
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if(dir->d_name[0] == '.') continue; // exclude . and ..
            filenames[n_files] = strdup(dir->d_name);
            n_files += 1;
        }
        *filecount = n_files;
    } else {
        printf("Failed to read %s\n", SM83_DIR);
        error = 1;
    }
    if (d) closedir(d);
    return error;
}

int read_file(char *file_path, struct string *s) {
    int err = 0;
    int fd = open(file_path, O_RDONLY);
    if (fd < 0) {
        error ("Failed to read %s due to %s", file_path, strerror(errno));
        err = 1;
        goto error;
    }

    ssize_t n = 0;
    while ((n = read(fd, file_buf, FILE_BUF_LEN)) > 0) {
        string_appendn(s, file_buf, n);
    }

error:
    if (fd >= 0) close(fd);
    return err;
}

int parse_file(char *file_path, struct sm83_test **sm83_tests, size_t *length) {
    int err = 0;
    // get raw data
    struct string file_data = {0};
    (void)read_file(file_path, &file_data);
    if (file_data.str == NULL) {
        error("Failed to read file %s", file_path);
        err = 1;
        goto error;
    }
    // parse json
    cJSON *tests = cJSON_Parse((char*) file_data.str);
    if (tests == NULL) {
        const char *json_err = cJSON_GetErrorPtr();
        assert (json_err != NULL && "Failed to know JSON error");
        error("Failed to parse file %s with cJSON: %s", file_path, json_err);
        err = 2;
        goto error;
    }
    ssize_t test_len = cJSON_GetArraySize(tests);
    if (test_len < 0) {
        error("Expected test array with non-negative size for file %s",
              file_path);
        err = 3;
        goto error;
    }
    // tests
    *length = test_len;
    struct sm83_test *sm83ts = calloc(test_len + 1, sizeof(struct sm83_test));
    if (sm83ts == NULL) {
        error("Failed to allocate sm83 tests");
        err = 4;
        goto error;
    }
    size_t test_idx = 0;

    // TODO: definitely missing some registers, will check later
    #define KNAMES_LEN 12
    static char *knames[KNAMES_LEN] =
        { "pc", "sp", "a", "b", "c", "d", "e", "f", "h", "l", "ime", "ei" };

    // TODO: clean up allocations on errors
    struct gbs_kv kv;
    struct ram_state rstate;
    struct sm83_test st;
    cJSON *test = NULL;
    cJSON *rpair = NULL;
    cJSON *rp_t = NULL;
    cJSON_ArrayForEach(test, tests) {
        kv = (struct gbs_kv) {0};
        rstate = (struct ram_state) {0};
        st = (struct sm83_test) {0};

        cJSON *name = cJSON_GetObjectItemCaseSensitive(test, "name");
        if (!cJSON_IsString(name) || name->valuestring == NULL) {
            error("Missing 'name', Malformed test file %s", file_path);
            err = 4;
            goto error;
        }
        st.name = strdup(name->valuestring);
        // initials
        cJSON *initial = cJSON_GetObjectItemCaseSensitive(test, "initial");
        if (!cJSON_IsObject(initial)) {
            error("Missing 'initial', malformed test file %s", file_path);
            err = 4;
            goto error;
        }
        // register, etc states
        for (int i = 0; i < KNAMES_LEN; i++) {
            cJSON *t = cJSON_GetObjectItemCaseSensitive(initial, knames[i]);
            if (t == NULL || !cJSON_IsNumber(t)) continue;
            kv.k = strdup(knames[i]);
            kv.v = t->valueint;
            (void)kvs_append(&st.initial.kvs, &kv);
        }
        // ram state
        cJSON *initial_ram = cJSON_GetObjectItemCaseSensitive(initial, "ram");
        if (!cJSON_IsArray(initial_ram)) {
            error("Missing 'final', malformed test file %s", file_path);
            err = 4;
            goto error;
        }
        cJSON_ArrayForEach(rpair, initial_ram) {
            // TODO: validate that the pair is in fact a pair with array size == 2
            rp_t = cJSON_GetArrayItem(rpair, 0);
            assert (rp_t != NULL && cJSON_IsNumber(rp_t)); // TODO: proper err chk
            rstate.pos = rp_t->valueint;
            rp_t = cJSON_GetArrayItem(rpair, 1);
            assert (rp_t != NULL && cJSON_IsNumber(rp_t)); // TODO: proper err chk
            rstate.val = rp_t->valueint;
            (void)rams_append(&st.initial.rams, &rstate);
        }

        // finals
        cJSON *final = cJSON_GetObjectItemCaseSensitive(test, "final");
        if (!cJSON_IsObject(final)) {
            error("Missing 'final', malformed test file %s", file_path);
            err = 4;
            goto error;
        }
        // register, etc states
        for (int i = 0; i < KNAMES_LEN; i++) {
            cJSON *t = cJSON_GetObjectItemCaseSensitive(final, knames[i]);
            if (t == NULL || !cJSON_IsNumber(t)) continue;
            kv.k = strdup(knames[i]);
            kv.v = t->valueint;
            kvs_append(&st.final.kvs, &kv);
        }
        // ram state
        cJSON *final_ram = cJSON_GetObjectItemCaseSensitive(final, "ram");
        if (!cJSON_IsArray(final_ram)) {
            error("Missing 'final', malformed test file %s", file_path);
            err = 4;
            goto error;
        }
        cJSON_ArrayForEach(rpair, final_ram) {
            // TODO: validate that the pair is in fact a pair with array size == 2
            rp_t = cJSON_GetArrayItem(rpair, 0);
            assert (rp_t != NULL && cJSON_IsNumber(rp_t)); // TODO: proper err chk
            rstate.pos = rp_t->valueint;
            rp_t = cJSON_GetArrayItem(rpair, 1);
            assert (rp_t != NULL && cJSON_IsNumber(rp_t)); // TODO: proper err chk
            rstate.val = rp_t->valueint;
            (void)rams_append(&st.final.rams, &rstate);
        }
        cJSON *cycles = cJSON_GetObjectItemCaseSensitive(test, "cycles");
        st.n_cycles = cJSON_GetArraySize(cycles);

        sm83ts[test_idx++] = st;
    }
    *sm83_tests = sm83ts;
error:
    // TODO: cleanup all allocations
    if (file_data.str != NULL) free (file_data.str);
    return err;
}

int run_tests(struct sm83_test *tests, size_t count, void *gbm_state) {
    size_t success_count = 0;
    for (int i = 0; i < count; i++) {
        struct sm83_test current_test = tests[i];
        int pass = run_sm83_test(current_test);
        if (pass == 0) {
            success_count++;
            continue; // only show tests that failed
        }
        // printf("\t[%s]: %s\n", current_test.name, pass ? "FAILURE" : "SUCCESS");
    }
    printf("SM83 RESULTS: %zu/%zu\n", success_count, count);
    return success_count == count;
}

void dump_rom(struct string *rom_data) {
    for (size_t i = 0; i < rom_data->len; i++) {
        if (isprint(rom_data->str[i]))
            printf("%c", rom_data->str[i]);
        else
            printf(".");
    }
    printf("\n");
}
// #define ONLY_TESTS 1
int main(int argc, char* argv[]) {
#ifdef ONLY_TESTS
    char **filenames = calloc(1024, sizeof(char*));
    size_t filenames_count = 0;
    if(argc < 2){
        printf("Running all tests.");
        test_file_list (filenames, &filenames_count);
        qsort(filenames, filenames_count, sizeof(char*), cmp_str);
    } else {
        printf("Running specified tests.");
        filenames_count = argc-1;
        for(int i = 1; i < argc; i++){
            filenames[i-1] = malloc(sizeof(argv[i]));
            strcpy(filenames[i-1], argv[i]);
        }
    }

    printf("Test Files\n");
    for (size_t i = 0; i < filenames_count && i < 10; i++)
        printf("%zu: %s\n", i, filenames[i]);

    struct sm83_test *tests = NULL;
    size_t tests_len = 0;
    uint16_t successes = 0;
    uint16_t tests_run = 0;
    for (size_t i = 0; i < filenames_count; i++){
        char* file = malloc(strlen(SM83_DIR) + strlen(filenames[i]) + 1);
        strcpy(file, SM83_DIR);
        strcat(file, filenames[i]);
        parse_file(file, &tests, &tests_len);
        assert (tests != NULL);
        printf("running test %s -- ", filenames[i]);
        #ifdef VERBOSE
        sm83_test_dump(tests, tests_len);
        tests_len = 10;
        #endif
        successes += run_tests(tests, tests_len, NULL);
        tests_run++;
    }
    printf("%d / %d opcodes fully passed tests\n", successes, tests_run);
#else
    // PPU uses overdrawn 256x256 screen
    struct dim vdim = { .w = 256, .h = 256 };
    // physical screen
    struct dim pdim = { .w = vdim.w * WIN_SCALE, .h = vdim.h * WIN_SCALE };
    gb_t *gameboy_state = calloc(1, sizeof(gb_t));
    ppu_t *ppu = calloc(1, sizeof(ppu_t));
    
    // Initialize Game Boy state after boot ROM
    gameboy_state->pc = 0x0100;  // ROM entry point
    gameboy_state->sp = 0xFFFE;  // Stack pointer
    gameboy_state->reg[RA] = 0x01;  // A register
    gameboy_state->reg[RF] = 0xB0;  // Flags
    gameboy_state->reg[RB] = 0x00;  // B
    gameboy_state->reg[RC] = 0x13;  // C
    gameboy_state->reg[RD] = 0x00;  // D
    gameboy_state->reg[RE] = 0xD8;  // E
    gameboy_state->reg[RH] = 0x01;  // H
    gameboy_state->reg[RL] = 0x4D;  // L
    
    // Initialize PPU registers
    gameboy_state->ram[0xFF40] = 0x91;  // LCDC - LCD enabled, BG on
    FILE *fp = fopen("./roms/tetris.gb", "rb");
    if (!fp) {
        printf("File not found!\n");
        exit(1);
    }
    int bytesRead =
      fread(gameboy_state->ram, 1, 0x10000, fp);
    fclose(fp);

    InitWindow(pdim.w, pdim.h, WINDOW_TITLE);
    SetTargetFPS(TARGET_FPS);
    
    // Create texture for PPU framebuffer
    Image ppu_image = GenImageColor(256, 256, BLACK);
    Texture2D ppu_texture = LoadTextureFromImage(ppu_image);
    UnloadImage(ppu_image);
    
    // Run a few thousand cycles to see what happens
    for (int i = 0; i < 50000; i++) {
        uint16_t old_pc = gameboy_state->pc;
        gameboy_state->cycles += step(gameboy_state);
        
        if (i % 100 == 0) {
            printf("Step %d: PC: 0x%04X -> 0x%04X, cycles: %llu\n", 
                   i, old_pc, gameboy_state->pc, gameboy_state->cycles);
        }
        
        // Check if any VRAM has been written
        if (i % 1000 == 0) {
            printf("VRAM check: 0x8000=%02X 0x8001=%02X, tilemap: 0x9800=%02X 0x9801=%02X\n",
                   gameboy_state->ram[0x8000], gameboy_state->ram[0x8001],
                   gameboy_state->ram[0x9800], gameboy_state->ram[0x9801]);
        }
    }
    
    printf("After 5000 steps, entering main loop...\n");
    
    while (!WindowShouldClose()) {
        uint16_t old_pc = gameboy_state->pc;
        for(int q = 0; q < 10000; q++){
            gameboy_state->ram[0xFF0F] = 1;
            gameboy_state->ram[0xFF44] = (gameboy_state->ram[0xFF44]+1) % 154;
            for(int i = 0; i < 10; i++){
                gameboy_state->cycles += step(gameboy_state);
            }
        }
        update_ppu(ppu, gameboy_state);

        // Convert PPU display buffer to raylib texture
        Color pixels[256 * 256];
        for (int y = 0; y < 256; y++) {
            for (int x = 0; x < 256; x++) {
                uint8_t pixel_val = ppu->display[y][x];
                // Convert Game Boy 2-bit color to grayscale
                uint8_t gray_val = 255 - (pixel_val * 85); // 0->255, 1->170, 2->85, 3->0
                pixels[y * 256 + x] = (Color){ gray_val, gray_val, gray_val, 255 };
            }
        }
        UpdateTexture(ppu_texture, pixels);

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTextureEx(ppu_texture, (Vector2){0, 0}, 0.0f, WIN_SCALE, WHITE);
        EndDrawing();
    }
    
    UnloadTexture(ppu_texture);
    CloseWindow();
#endif // ONLY_TESTS

    return EXIT_SUCCESS;
}
