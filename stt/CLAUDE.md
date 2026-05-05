# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

C++17 program that integrates whisper.cpp for automatic speech recognition (ASR). Takes a 16kHz mono WAV file and transcribes it to text (default: Japanese), with timestamps.

## Build & Run

```bash
# Build
cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && cmake --build . -j

# Run
./myproj <model.ggml.bin> <audio_16kHz.wav>

# With library path for CUDA
LD_LIBRARY_PATH=~/whisper.cpp/build/src:~/whisper.cpp/build/ggml/src:~/whisper.cpp/build/ggml/src/ggml-cuda:$LD_LIBRARY_PATH ./myproj <model> <audio>
```

## Architecture

- **`main.cpp`** — Single entry point. Flow: load model (`whisper_init_from_file_with_params`) → read WAV (skip 44-byte header, PCM16→float32) → configure params (`whisper_full_default_params`, language="ja") → run transcription (`whisper_full`) → print segments with timestamps → cleanup (`whisper_free`).
- **`headers/`** — Copied whisper.cpp C API headers (whisper.h, ggml.h, ggml-cpu.h, ggml-backend.h, ggml-alloc.h). These are static copies from the whisper.cpp dependency.
- **`CMakeLists.txt`** — Links against whisper.cpp shared libs (`libwhisper.so`, `libggml.so`, `libggml-base.so`, `libggml-cpu.so`, optionally `libggml-cuda.so`). Whisper.cpp root is hardcoded to `~/whisper.cpp`.

## Key Details

- Audio input must be 16kHz mono PCM16 WAV. Use ffmpeg to convert: `ffmpeg -y -i input.wav -ar 16000 -ac 1 -c:a pcm_s16le output_16k.wav`
- Model sizes: tiny (75MB), base (142MB), small (466MB, recommended), medium (1.5GB)
- CUDA GPU acceleration is automatic when linked against `libggml-cuda.so`
- No test framework or linter is configured
