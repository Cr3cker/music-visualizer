#include <raylib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "kiss_fft.h"
#include <math.h>


#define FFT_SIZE 1024
#define BAR_WIDTH 10
#define SCALE_FACTOR 1.0f
#define INTERPOLATION_SPEED 0.07f


void perform_fft(float *input, kiss_fft_cpx *output) {
    kiss_fft_cfg cfg;
    kiss_fft_cpx *buf_in = malloc(sizeof(kiss_fft_cpx) * FFT_SIZE);
    kiss_fft_cpx *buf_out = malloc(sizeof(kiss_fft_cpx) * FFT_SIZE);

    if (buf_in == NULL || buf_out == NULL) {
        printf("Failed to allocate memory for FFT\n");
        return;
    }

    for (int i = 0; i < FFT_SIZE; i++) {
        buf_in[i].r = input[i];
        buf_in[i].i = 0;
    }

    cfg = kiss_fft_alloc(FFT_SIZE, 0, NULL, NULL);
    kiss_fft(cfg, buf_in, buf_out);

    for (int i = 0; i < FFT_SIZE; i++) {
        output[i].r = buf_out[i].r;
        output[i].i = buf_out[i].i;
    }

    free(cfg);
    free(buf_in);
    free(buf_out);
}


int main(int argc, char *argv[]) {
    int width = 700;
    int height = 500;

    InitWindow(width, height, "Music Player");
    SetTargetFPS(60);
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    const char *text = "Drag'n'Drop music here";
    int fontSize = 30;

    InitAudioDevice();

    Sound music = {0};
    float *audioData = NULL;
    int sampleCount = 0;
    int currentSample = 0;

    float *currentHeights = malloc(sizeof(float) * FFT_SIZE);
    for (int i = 0; i < FFT_SIZE; i++) {
        currentHeights[i] = 0.0f;
    }

    while (!WindowShouldClose()) {
        BeginDrawing();
        int fps = GetFPS();
        char fpsText[20];
        sprintf(fpsText, "FPS: %d", fps);

        int textWidth = MeasureText(fpsText, 20);
        int margin = 10; 

        DrawText(fpsText, GetScreenWidth() - textWidth - margin, margin, 20, GREEN);
        ClearBackground(BLACK);

        textWidth = MeasureText(text, fontSize);
        int textX = width/2 - textWidth/2;
        int textY = height/2 - fontSize/2;

        if (!IsSoundPlaying(music)) DrawText("Drag'n'Drop music here", textX, textY, fontSize, WHITE);                  

        if (IsKeyPressed(KEY_SPACE) && IsSoundPlaying(music)) PauseSound(music);
        else if (IsKeyPressed(KEY_SPACE)) ResumeSound(music);

        if (IsSoundPlaying(music)) {
        
            kiss_fft_cpx *frequencyData = malloc(sizeof(kiss_fft_cpx) * FFT_SIZE);
            perform_fft(&audioData[currentSample], frequencyData);

            float maxAmplitude = 0.0f;

            for (int i = 0; i < FFT_SIZE; i++) {
                float amplitude = sqrtf(frequencyData[i].r * frequencyData[i].r + frequencyData[i].i * frequencyData[i].i);
                if (amplitude > maxAmplitude) {
                    maxAmplitude = amplitude;
                }
            }


            for (int i = 0; i < FFT_SIZE; i++) {
                float amplitude = sqrtf(frequencyData[i].r * frequencyData[i].r + frequencyData[i].i * frequencyData[i].i);
                float normalizedAmplitude = amplitude / maxAmplitude;
                float targetHeight = normalizedAmplitude * GetScreenHeight();

                currentHeights[i] += (targetHeight - currentHeights[i]) * INTERPOLATION_SPEED;

                DrawRectangle(i * BAR_WIDTH, GetScreenHeight() - currentHeights[i], BAR_WIDTH, currentHeights[i], RED);
            }

            free(frequencyData);

            currentSample += FFT_SIZE;
            if (currentSample + FFT_SIZE > sampleCount) {
                currentSample = 0;
            }
        } else if (IsFileDropped()) {
            FilePathList filesDropped = LoadDroppedFiles();
            int fileCount = filesDropped.count;

            for (int i = 0; i < fileCount; i++) {
                const char *extension = GetFileExtension(filesDropped.paths[i]);
                if (extension != NULL && 
                    (strcmp(extension, ".mp3") == 0 || strcmp(extension, ".wav") == 0 ||
                    strcmp(extension, ".ogg") == 0 || strcmp(extension, ".flac") == 0)) {
                    printf("Music file dropped: %s\n", filesDropped.paths[i]);
                    music = LoadSound(filesDropped.paths[i]);

                    Wave wave = LoadWave(filesDropped.paths[i]);

                    if (audioData != NULL) {
                        UnloadWaveSamples(audioData);
                    }

                    audioData = LoadWaveSamples(wave);
                    sampleCount = wave.frameCount * wave.channels;
                    currentSample = 0;

                    PlaySound(music);

                    UnloadWave(wave);
                }
            }

            UnloadDroppedFiles(filesDropped);
        }

        EndDrawing();
    }

    UnloadSound(music);
    if (audioData != NULL) {
         UnloadWaveSamples(audioData);
    }
    CloseWindow();
    CloseAudioDevice();

    return 0;
}