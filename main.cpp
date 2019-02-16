#include <stdio.h>
#include <complex>
#include <math.h>

#define SDL_MAIN_HANDLED
#include "SDL.h"

#define WIDTH 800
#define HEIGHT 600
#define MAX_ITERATION 1000

#define LERP(x, min, max) (((max)-(min))*(x)+(min))

SDL_Renderer* renderer;
unsigned short pixels[WIDTH*HEIGHT];
unsigned char histogram[MAX_ITERATION];

union Color {
    struct {
        unsigned char a;
        unsigned char r;
        unsigned char g;
        unsigned char b;
    };
    unsigned int v;
};

Color palette[] = {
    {255,  0,   7, 100},
    {255, 32, 107, 203},
    {255,237, 255, 255},
    {255,255, 170,   0},
    {255,  0,   2,   0},
    {255,  0,   7,  99}};

Color map_color(double x) {
    
    Color result;
    
    Color c1;
    Color c2;
    if (0 <= x && x < 0.16) {
        c1 = palette[0];
        c2 = palette[1];
    } else if (0.16 <= x && x < 0.42) {
        c1 = palette[1];
        c2 = palette[2];
    } else if (0.42 <= x && x < 0.6425) {
        c1 = palette[2];
        c2 = palette[3];
    } else if (0.6425 <= x && x < 0.8575) {
        c1 = palette[3];
        c2 = palette[4];
    } else {
        c1 = palette[4];
        c2 = palette[5];
    }
    
    result.r = LERP(x, c1.r, c2.r);
    result.g = LERP(x, c1.g, c2.g);
    result.b = LERP(x, c1.b, c2.b);
    result.a = 255;
    
    return result;
}


void render(double x_min, double y_min, double x_max, double y_max) {
    
    long start = SDL_GetTicks();
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 1);
    SDL_RenderClear(renderer);
    
    for (int i=0; i<MAX_ITERATION; i++) {
        histogram[i] = 0;
    }
    
    printf("Calculating x=(%f, %f), y=(%f, %f)\n", 
           x_min, x_max, y_min, y_max);
    unsigned short* pixel = pixels;
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
    
    // Compute normalized colors
    int total = 0;
    for (int i=0; i<MAX_ITERATION; i++) {
        total += histogram[i];
    }
    
    pixel = pixels;
    for (int y=0; y<HEIGHT; y++) {
        for (int x=0; x<WIDTH; x++) {
            Color color;
            if (*pixel == MAX_ITERATION) {
                color.v = 0x000000FF;
            } else {
                
                float hue = 0.0f;
                for (int i=0; i<*pixel; i++) {
                    hue += (float)histogram[i] / (float)total;
                }
                
                color = map_color(hue);
            }
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 0xFF);
            SDL_RenderDrawPoint(renderer, x, y);
            
            pixel++;
        }
    }
    
    SDL_RenderPresent(renderer);
    
    long end = SDL_GetTicks();
    printf("Time=%fs\n", (float)(end - start) / 1000.0f);
}

int main(int argc, char** argv) {
    
    SDL_Init(SDL_INIT_VIDEO);
    
    SDL_Window* window = SDL_CreateWindow("Mandelbrot",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          WIDTH, HEIGHT,
                                          SDL_WINDOW_SHOWN);
    renderer =  SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    
    
    // double zoom_ratio = 0.8;
    // double target_x = -0.74967;
    // double target_y = -0.07;
    double x_min = -2.5;
    double x_max = 1.0;
    double y_min = -1.1;
    double y_max = 1.1;
    
    render(x_min, y_min, x_max, y_max);
    
    bool zooming = false;
    int zoom_start_x;
    int zoom_start_y;
    int zoom_end_x;
    int zoom_end_y;
    
    bool running = true;
    while(running) {
        SDL_PumpEvents();
        
        /*
        x_min = LERP(zoom_ratio, x_min, target_x);
        x_max = LERP(zoom_ratio, x_max, target_x);
        y_min = LERP(zoom_ratio, y_min, target_y);
        y_max = LERP(zoom_ratio, y_max, target_y);
        */
        
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            switch (event.type) {
                
                case SDL_MOUSEBUTTONDOWN: {
                    zooming = true;
                    zoom_start_x = event.button.x;
                    zoom_start_y = event.button.y;
                    
                    zoom_end_x = zoom_start_x;
                    zoom_end_y = zoom_start_y;
                    
                    printf("start={%d,%d}\n", zoom_start_x, zoom_start_y);
                } break;
                
                case SDL_MOUSEBUTTONUP: {
                    zoom_end_x = event.button.x;
                    zoom_end_y = event.button.y;
                    
                    printf("end={%d,%d}\n", zoom_end_x, zoom_end_y);
                    
                    double px_start = (double)zoom_start_x/(double)WIDTH;
                    double px_end = (double)zoom_end_x/(double)WIDTH;
                    double py_start = (double)zoom_start_y/(double)HEIGHT;
                    double py_end = (double)zoom_end_y/(double)HEIGHT;
                    double temp_x_min = LERP(px_start, x_min, x_max);
                    double temp_x_max = LERP(px_end, x_min, x_max);
                    double temp_y_min = LERP(py_start, y_min, y_max);
                    double temp_y_max = LERP(py_end, y_min, y_max);
                    
                    x_min = temp_x_min;
                    x_max = temp_x_max;
                    y_min = temp_y_min;
                    y_max = temp_y_max;
                    
                    zooming = false;
                    
                    render(x_min, y_min, x_max, y_max);
                } break;
                
                case SDL_MOUSEMOTION: {
                    int mouse_x, mouse_y;
                    SDL_GetMouseState(&mouse_x, &mouse_y);
                    double mapped_x = (double)mouse_x / (double)WIDTH;
                    double mapped_y = (double)mouse_y / (double)HEIGHT;
                    printf("x=%f, y=%f\n", LERP(mapped_x, x_min, x_max), 
                           LERP(mapped_y, y_min, y_max));
                    
                    if (zooming) {
                        zoom_end_x = mouse_x;
                        zoom_end_y = mouse_y;
                    }
                } break;
                
                case SDL_QUIT: {
                    running = false;
                } break;
            }
        }
        
        if (zooming) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            SDL_SetRenderDrawColor(renderer, 0, 0x7F, 0xFF, 0x7F);
            SDL_Rect rect;
            rect.x = (int)zoom_start_x;
            rect.y = (int)zoom_start_y;
            rect.w = (int)(zoom_end_x-zoom_start_x);
            rect.h = (int)(zoom_end_y-zoom_start_y);
            SDL_RenderFillRect(renderer, &rect);
            SDL_RenderPresent(renderer);
        }
        
    }
    return 0;
}