#include <SDL3/SDL.h>
#include <iostream>
#include <vector>
#include <cstring>

#include "VideoDecoder.h"
#include "VideoRenderer.h"
#include "AudioDecoder.h"
#include <mutex>
#include <condition_variable>

// Shared audio buffer and synchronization primitives
std::vector<uint8_t> audioData;
std::mutex audioMutex;
std::condition_variable audioCondVar;
size_t audioPos = 0;

// Audio callback function
void SDLCALL audioCallback(void* userdata, Uint8* stream, int len) {
    std::unique_lock<std::mutex> lock(audioMutex);

    // Clear output buffer first (avoid garbage sound)
    SDL_memset(stream, 0, len);

    // If no data, just silence
    if (audioPos >= audioData.size()) {
        return;
    }

    // Calculate how many bytes we can copy
    int bytesAvailable = (int)(audioData.size() - audioPos);
    int bytesToCopy = (len > bytesAvailable) ? bytesAvailable : len;

    // Copy audio data into stream buffer
    SDL_memcpy(stream, audioData.data() + audioPos, bytesToCopy);

    audioPos += bytesToCopy;

    // If we've consumed all data, reset buffer (or signal refill)
    if (audioPos >= audioData.size()) {
        audioData.clear();
        audioPos = 0;
        audioCondVar.notify_one(); // optional: notify main thread to decode more
    }
}

int main(int argc, char* argv[]) {
    const char* videoFile = "sample.mp4";

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "Failed to init SDL: " << SDL_GetError() << "\n";
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_Window* window = SDL_CreateWindow(
        "Video Player",
        1280, 720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    if (!window) {
        std::cerr << "Failed to create SDL window: " << SDL_GetError() << "\n";
        SDL_Quit();
        return -1;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "Failed to create OpenGL context: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        SDL_GL_DestroyContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Open video decoder
    VideoDecoder videoDecoder;
    if (!videoDecoder.openFile(videoFile)) {
        std::cerr << "Failed to open video file: " << videoFile << "\n";
        SDL_GL_DestroyContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    int videoWidth = videoDecoder.getWidth();
    int videoHeight = videoDecoder.getHeight();
    VideoRenderer renderer(videoWidth, videoHeight);

    // Open audio decoder
    AudioDecoder audioDecoder;
    if (!audioDecoder.openFile(videoFile)) {
        std::cerr << "Failed to open audio from file: " << videoFile << "\n";
        SDL_GL_DestroyContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    SDL_AudioSpec desiredSpec = {};
    desiredSpec.freq = audioDecoder.getSampleRate();
    desiredSpec.format = SDL_AUDIO_S16;
    desiredSpec.channels = static_cast<Uint8>(audioDecoder.getChannels());

    SDL_AudioSpec obtainedSpec = {};

    SDL_AudioDeviceID audioDevice = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &desiredSpec);
    if (audioDevice == 0) {
        std::cerr << "Failed to open audio device: " << SDL_GetError() << "\n";
        SDL_Quit();
        return -1;
    }

    SDL_ResumeAudioDevice(audioDevice);

    std::vector<uint8_t> audioBuffer;

    bool running = true;
    SDL_Event event;

    while (running) {
        Uint32 frameStart = SDL_GetTicks();

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        // Clear screen
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        // Video frame
        AVFrame* frame = videoDecoder.getRGBFrame();
        if (frame) {
            int width = videoDecoder.getWidth();
            int height = videoDecoder.getHeight();
            int linesize = frame->linesize[0];
            uint8_t* rgbData = frame->data[0];

            if (linesize == width * 3) {
                renderer.uploadFrame(rgbData);
            }
            else {
                std::vector<uint8_t> packedData(width * height * 3);
                for (int y = 0; y < height; ++y) {
                    memcpy(&packedData[y * width * 3], &rgbData[y * linesize], width * 3);
                }
                renderer.uploadFrame(packedData.data());
            }
        }

        // Audio decode
        if (audioDecoder.decodeNextFrame(audioBuffer)) {
            if (!audioBuffer.empty()) {
                // Lock and append decoded audio to shared buffer
                std::unique_lock<std::mutex> lock(audioMutex);
                audioData.insert(audioData.end(), audioBuffer.begin(), audioBuffer.end());
            }
        }

        renderer.render();
        SDL_GL_SwapWindow(window);

        Uint32 frameTime = SDL_GetTicks() - frameStart;
        int delayMs = static_cast<int>(videoDecoder.getFrameDelay() * 1000);
        if (frameTime < delayMs) {
            SDL_Delay(delayMs - frameTime);
        }
    }

    SDL_CloseAudioDevice(audioDevice);
    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}