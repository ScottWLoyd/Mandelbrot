#include <stdio.h>
#include <complex>

#define SDL_MAIN_HANDLED
#include "SDL.h"

#define WIDTH 800
#define HEIGHT 600
#define MAX_ITERATION 255

#define LERP(x, min, max) (((max)-(min))*(x)+(min))

unsigned char pixels[WIDTH*HEIGHT];
unsigned char histogram[MAX_ITERATION];

unsigned int map_color(float x) {
    unsigned int result = (1.0f - x) * 0xFFFFFF;
    return result;
}

int main(int argc, char** argv) {
    
    SDL_Init(SDL_INIT_VIDEO);
    
    SDL_Window* window = SDL_CreateWindow("Mandelbrot",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          WIDTH, HEIGHT,
                                          SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer =  SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 1);
    SDL_RenderClear(renderer);
    
    double zoom_ratio = 0.8;
    double target_x = -0.74967;
    double target_y = -0.07;
    double x_min = -2.5;
    double x_max = 1.0;
    double y_min = -1.1;
    double y_max = 1.1;
    
    bool running = true;
    while(running) {
        long start = SDL_GetTicks();
        
        for (int i=0; i<MAX_ITERATION; i++) {
            histogram[i] = 0;
        }
        
        printf("Calculating x=(%f, %f), y=(%f, %f)\n", 
               x_min, x_max, y_min, y_max);
        unsigned char* pixel = pixels;
        for (int y=0; y<HEIGHT; y++) {
            for (int x=0; x<WIDTH; x++) {
                double px = (double)x / (double)WIDTH;
                double py = (double)y / (double)HEIGHT;
                std::complex<double> c(LERP(px, x_min, x_max), LERP(py, y_min, y_max));
                std::complex<double> z(0.0, 0.0);
                
                int iteration = 0;
                while(iteration < MAX_ITERATION) {
                    z = z * z + c;
                    if (abs(z) > 2) {
                        break;
                    }
                    iteration++;
                }
                
                histogram[iteration]++;
                *pixel++ = iteration;
            }
        }
        
#if 1
        // Compute normalized colors
        int total = 0;
        for (int i=0; i<MAX_ITERATION; i++) {
            total += histogram[i];
        }
        
        pixel = pixels;
        for (int y=0; y<HEIGHT; y++) {
            for (int x=0; x<WIDTH; x++) {
                float hue = 0.0f;
                for (int i=0; i<*pixel; i++) {
                    hue += (float)histogram[i] / (float)total;
                }
                
                unsigned int mapped = map_color(hue);
                
                unsigned char r = (mapped >> 16) & 0xFF;
                unsigned char g = (mapped >>  8) & 0xFF;
                unsigned char b = (mapped >>  0) & 0xFF;
                SDL_SetRenderDrawColor(renderer, r, g, b, 0xFF);
                SDL_RenderDrawPoint(renderer, x, y);
                
                pixel++;
            }
        }
#endif
        SDL_RenderPresent(renderer);
        
        SDL_PumpEvents();
        int mouse_x, mouse_y;
        SDL_GetMouseState(&mouse_x, &mouse_y);
        double mapped_x = (double)mouse_x / (double)WIDTH;
        double mapped_y = (double)mouse_y / (double)HEIGHT;
        printf("x=%f, y=%f\n", LERP(mapped_x, x_min, x_max), 
               LERP(mapped_y, y_min, y_max));
        
        x_min = LERP(zoom_ratio, x_min, target_x);
        x_max = LERP(zoom_ratio, x_max, target_x);
        y_min = LERP(zoom_ratio, y_min, target_y);
        y_max = LERP(zoom_ratio, y_max, target_y);
        
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT: {
                    running = false;
                } break;
            }
        }
        
        long end = SDL_GetTicks();
        printf("Time=%fs\n", (float)(end - start) / 1000.0f);
        SDL_Delay(1);
    }
    return 0;
}