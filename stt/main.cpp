#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <thread>
#include <chrono>
#include <SDL.h>
#include "whisper.h"
#include "common-sdl.h"

// Configuration constants
constexpr int32_t AUDIO_RING_BUFFER_SIZE = 30000;     // 30 seconds
constexpr int64_t VAD_COOLDOWN_MS        = 2000;      // 2 seconds
constexpr int32_t VAD_INFERENCE_LENGTH_MS = 5000;     // 5 seconds
constexpr int32_t VAD_PROBE_LENGTH_MS    = 2000;      // 2 seconds
constexpr int32_t DEFAULT_STEP_MS        = 500;       // default chunk size
constexpr int32_t DEFAULT_KEEP_MS        = 200;       // default overlap size
constexpr int32_t VAD_FRAME_MS           = 100;       // VAD frame duration
constexpr int32_t MIN_STEP_MS            = 100;       // minimum chunk size
constexpr int32_t MAX_STEP_MS            = 30000;     // maximum chunk size
constexpr float   DEFAULT_VAD_THRESHOLD  = 0.15f;
constexpr int32_t VAD_N_FRAMES           = 20;        // frames to check for speech
constexpr float   VAD_MIN_ENERGY_THRESHOLD = 0.02f;
constexpr float   VAD_MEDIAN_THRESHOLD   = 1e-6f;

struct params {
    std::string model;
    int32_t device_id = -1;  // -1 = auto (first available)
    int32_t step_ms   = DEFAULT_STEP_MS;
    int32_t keep_ms   = DEFAULT_KEEP_MS;
    bool vad_enable   = true;
    float vad_thold   = DEFAULT_VAD_THRESHOLD;
};

// Energy-based VAD (simplified from whisper.cpp common.cpp)
static bool vad_simple(const float *pcm, int sample_rate, int n_samples,
                       float thold_prob, int n_frames, float &prob) {
    const int frame_len = sample_rate * VAD_FRAME_MS / 1000; // 1600 at 16kHz
    const int n_frames_total = n_samples / frame_len;

    if (n_frames_total < 1) {
        fprintf(stderr, "Warning: VAD probe too short (got %d samples, %d frames)\n", n_samples, n_frames_total);
        prob = 0.0f;
        return false;
    }

    std::vector<float> energies(n_frames_total);
    for (int i = 0; i < n_frames_total; ++i) {
        float energy = 0.0f;
        for (int j = 0; j < frame_len; ++j) {
            const float s = pcm[i * frame_len + j];
            energy += s * s;
        }
        energies[i] = std::sqrt(energy / frame_len); // RMS
    }

    // Find median energy for threshold
    std::vector<float> sorted_energies = energies;
    std::sort(sorted_energies.begin(), sorted_energies.end());
    const int mid = n_frames_total / 2;
    float energy_median = (n_frames_total % 2 == 0)
        ? (sorted_energies[mid - 1] + sorted_energies[mid]) * 0.5f
        : sorted_energies[mid];

    // Check recent frames for speech
    const int n_check = std::min(n_frames, n_frames_total);
    int speech_frames = 0;
    for (int i = n_frames_total - n_check; i < n_frames_total; ++i) {
        if (energy_median > VAD_MEDIAN_THRESHOLD && energies[i] > energy_median * thold_prob) {
            speech_frames++;
        } else if (energies[i] > VAD_MIN_ENERGY_THRESHOLD) {
            speech_frames++;
        }
    }

    prob = (float)speech_frames / (float)n_check;
    return prob >= thold_prob;
}

// Format timestamp as MM:ss.cc string
static std::string format_timestamp(int64_t t) {
    int64_t sec = t / 100;
    int cc = (int)(t % 100);
    char buf[32];
    snprintf(buf, sizeof(buf), "%02lld:%02lld.%02d",
             (long long)sec / 60, (long long)sec % 60, cc * 10);
    return std::string(buf);
}

// Format timestamp as MM:ss.cc
static void print_ts(int64_t t) {
    printf("%s", format_timestamp(t).c_str());
}

static bool parse_arg(params &p, const char *arg, int argc, char *argv[], int &i) {
    if (strcmp(arg, "--step") == 0 && i + 1 < argc) {
        int val = atoi(argv[++i]);
        if (val < MIN_STEP_MS || val > MAX_STEP_MS) {
            fprintf(stderr, "Warning: --step %d out of range [%d, %d], using default\n",
                    val, MIN_STEP_MS, MAX_STEP_MS);
            p.step_ms = DEFAULT_STEP_MS;
        } else {
            p.step_ms = val;
        }
        return true;
    }
    if (strncmp(arg, "--step=", 7) == 0) {
        int val = atoi(arg + 7);
        if (val < MIN_STEP_MS || val > MAX_STEP_MS) {
            fprintf(stderr, "Warning: --step %d out of range [%d, %d], using default\n",
                    val, MIN_STEP_MS, MAX_STEP_MS);
            p.step_ms = DEFAULT_STEP_MS;
        } else {
            p.step_ms = val;
        }
        return true;
    }
    if (strcmp(arg, "--keep") == 0 && i + 1 < argc) {
        int val = atoi(argv[++i]);
        if (val < 0 || val > MAX_STEP_MS) {
            fprintf(stderr, "Warning: --keep %d out of range [0, %d], using default\n", val, MAX_STEP_MS);
            p.keep_ms = DEFAULT_KEEP_MS;
        } else {
            p.keep_ms = val;
        }
        return true;
    }
    if (strncmp(arg, "--keep=", 7) == 0) {
        int val = atoi(arg + 7);
        if (val < 0 || val > MAX_STEP_MS) {
            fprintf(stderr, "Warning: --keep %d out of range [0, %d], using default\n", val, MAX_STEP_MS);
            p.keep_ms = DEFAULT_KEEP_MS;
        } else {
            p.keep_ms = val;
        }
        return true;
    }
    if (strcmp(arg, "--vad") == 0 && i + 1 < argc) {
        std::string val(argv[++i]);
        p.vad_enable = (val != "false" && val != "0");
        return true;
    }
    if (strncmp(arg, "--vad=", 6) == 0) {
        std::string val(arg + 6);
        p.vad_enable = (val != "false" && val != "0");
        return true;
    }
    if (strcmp(arg, "--device") == 0 && i + 1 < argc) {
        int val = atoi(argv[++i]);
        if (val < 0) {
            fprintf(stderr, "Warning: --device %d is negative, using auto-selection\n", val);
            p.device_id = -1;
        } else {
            p.device_id = val;
        }
        return true;
    }
    if (strncmp(arg, "--device=", 9) == 0) {
        int val = atoi(arg + 9);
        if (val < 0) {
            fprintf(stderr, "Warning: --device %d is negative, using auto-selection\n", val);
            p.device_id = -1;
        } else {
            p.device_id = val;
        }
        return true;
    }
    return false;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <model.ggml.bin> [--device N] [--step N] [--keep N] [--vad [true|false]]\n", argv[0]);
        printf("\nOptions:\n");
        printf("  --device N   capture device index (default: auto)\n");
        printf("  --step N     chunk size in ms (default: 500)\n");
        printf("  --keep N     overlap size in ms (default: 200)\n");
        printf("  --vad [on|off]  voice activity detection (default: on)\n");
        return 1;
    }

    params p;
    p.model = argv[1];
    for (int i = 2; i < argc; ++i) {
        if (argv[i][0] == '-') {
            parse_arg(p, argv[i], argc, argv, i);
        }
    }

    // Load model
    whisper_context_params cparams = whisper_context_default_params();
    whisper_context *ctx = whisper_init_from_file_with_params(p.model.c_str(), cparams);
    if (!ctx) {
        fprintf(stderr, "Error: failed to load model from '%s'\n", p.model.c_str());
        return 1;
    }

    printf("Model loaded: %s\n", p.model.c_str());

    // Configure whisper parameters
    whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    wparams.print_progress     = false;
    wparams.print_realtime     = false;
    wparams.print_timestamps   = true;
    wparams.language           = "ja";
    wparams.translate          = false;
    wparams.single_segment     = true;

    // List available capture devices
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "Error: SDL_InitSubSystem failed: %s\n", SDL_GetError());
        whisper_free(ctx);
        return 1;
    }
    int n_devices = SDL_GetNumAudioDevices(SDL_TRUE);
    if (n_devices <= 0) {
        fprintf(stderr, "Error: no capture devices found (%s)\n", SDL_GetError());
        whisper_free(ctx);
        SDL_Quit();
        return 1;
    }
    printf("%d capture device(s):\n", n_devices);
    for (int i = 0; i < n_devices; ++i) {
        printf("  [%d] %s\n", i, SDL_GetAudioDeviceName(i, SDL_TRUE));
    }

    int capture_id = (p.device_id >= 0 && p.device_id < n_devices) ? p.device_id : 0;
    printf("Using device #%d: %s\n", capture_id, SDL_GetAudioDeviceName(capture_id, SDL_TRUE));

    // Initialize audio capture
    audio_async audio(AUDIO_RING_BUFFER_SIZE); // Ring buffer size
    if (!audio.init(capture_id, 16000)) {
        fprintf(stderr, "Error: failed to initialize audio capture on device #%d\n", capture_id);
        whisper_free(ctx);
        return 1;
    }

    printf("Audio streaming started...\n");
    audio.resume();

    // Overlap buffer: keep last `keep_ms` samples from previous chunk (16kHz -> ms*16)
    int n_samples_keep = p.keep_ms * 16;
    std::vector<float> pcmf32_old(n_samples_keep > 0 ? n_samples_keep : 0);

    // VAD state
    auto t_last_detection = std::chrono::steady_clock::now();

    while (sdl_poll_events()) {
        if (p.vad_enable) {
            // === VAD MODE: only transcribe when speech is detected ===
            auto now = std::chrono::steady_clock::now();
            int64_t elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - t_last_detection).count();

            if (elapsed_ms < VAD_COOLDOWN_MS) {
                // Cooldown: skip until enough time has passed
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }

            // Probe: grab 2 seconds of audio for VAD check
            std::vector<float> pcmf32_probe;
            audio.get(VAD_PROBE_LENGTH_MS, pcmf32_probe);
            if (pcmf32_probe.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }

            float vad_prob = 0.0f;
            if (!vad_simple(pcmf32_probe.data(), 16000, (int)pcmf32_probe.size(),
                            p.vad_thold, VAD_N_FRAMES, vad_prob)) {
                // Silence detected - wait for next speech
                continue;
            }

            printf("Speech detected (prob=%.2f)\n", vad_prob);
            t_last_detection = std::chrono::steady_clock::now();

            // Grab audio window for inference
            audio.get(VAD_INFERENCE_LENGTH_MS, pcmf32_probe);
            if (pcmf32_probe.empty()) {
                continue;
            }

            // Run inference
            if (whisper_full(ctx, wparams, pcmf32_probe.data(), (int)pcmf32_probe.size()) != 0) {
                fprintf(stderr, "Error: whisper_full failed\n");
                break;
            }

            // Output segments
            int n_segments = whisper_full_n_segments(ctx);
            for (int i = 0; i < n_segments; ++i) {
                const char *text = whisper_full_get_segment_text(ctx, i);
                int64_t t0 = whisper_full_get_segment_t0(ctx, i);
                int64_t t1 = whisper_full_get_segment_t1(ctx, i);
                print_ts(t0); printf(" --> "); print_ts(t1); printf(" %s\n", text);
            }
            fflush(stdout);

        } else {
            // === FIXED WINDOW MODE: transcribe at regular intervals ===
            std::vector<float> pcmf32_new;
            audio.get(p.step_ms, pcmf32_new);
            if (pcmf32_new.empty()) {
                // Underflow: clear buffer and retry
                fprintf(stderr, "Warning: audio underflow\n");
                audio.clear();
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }

            // Build chunk: [old_tail][new_chunk] for overlap continuity
            int n_samples_new = (int)pcmf32_new.size();
            int n_samples_take = (int)pcmf32_old.size();
            std::vector<float> pcmf32_chunk(n_samples_take + n_samples_new);
            if (n_samples_take > 0) {
                memcpy(pcmf32_chunk.data(), pcmf32_old.data(), n_samples_take * sizeof(float));
            }
            memcpy(pcmf32_chunk.data() + n_samples_take, pcmf32_new.data(), n_samples_new * sizeof(float));

            // Run inference
            if (whisper_full(ctx, wparams, pcmf32_chunk.data(), (int)pcmf32_chunk.size()) != 0) {
                fprintf(stderr, "Error: whisper_full failed\n");
                break;
            }

            // Output segments
            int n_segments = whisper_full_n_segments(ctx);
            for (int i = 0; i < n_segments; ++i) {
                const char *text = whisper_full_get_segment_text(ctx, i);
                int64_t t0 = whisper_full_get_segment_t0(ctx, i);
                int64_t t1 = whisper_full_get_segment_t1(ctx, i);
                print_ts(t0); printf(" --> "); print_ts(t1); printf(" %s\n", text);
            }
            fflush(stdout);

            // Save chunk tail for next overlap (keep_ms worth of samples)
            int n_tail = std::min((int)pcmf32_chunk.size(), n_samples_keep);
            pcmf32_old.assign(pcmf32_chunk.end() - n_tail, pcmf32_chunk.end());
        }
    }

    printf("---\n");

    // Graceful shutdown
    audio.pause();
    whisper_free(ctx);
    SDL_Quit();
    return 0;
}
