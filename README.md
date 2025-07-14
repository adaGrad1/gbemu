# Game Boy Emulator
A working gameboy (classic, not color) emulator! Not aiming for any semblance of accuracy.

Currently tested + playable with Tetris and Kirby's Dream Land

## Build

Initialize git submodules

```console
$ git submodule update
$ git submodule sync
$ git submodule init sm83
```

Manual build:
```console
$ gcc -c -o cjson.o vendor/cJSON/cJSON.c
$ gcc -c -o main.o src/main.c
$ gcc -o gbemu main.o cjson.o
$ rm main.o cjson.o
$ ./build/gbemu
```

With GNU make:
```console
$ make
$ ./build/gbemu
```

Or with nix:
```console
$ nix develop
$ make
$ ./build/gdemu
```

## Acknowledgements

cJSON.c: https://github.com/DaveGamble/cJSON (MIT license)
sm83: https://github.com/SingleStepTests/sm83
