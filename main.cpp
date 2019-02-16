#include <stdio.h>
#include <complex>
#include <math.h>
#include <assert.h>

#define SDL_MAIN_HANDLED
#include "SDL.h"

#define WIDTH 800
#define HEIGHT 800
#define MAX_ITERATION 256

#define LERP(x, min, max) (((max)-(min))*(x)+(min))
#define CLAMP(min, x, max) ((x)<(min)?(x)=(min):((x)>(max)?((x)=(max)):0))

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

#define RENDER_PALETTE 0
#define NUM_GRADIENTS 6
double breaks[NUM_GRADIENTS] = {
    //0, 0.16, 0.42, 0.6425, 0.8575, 1,
    0, 0.26, 0.42, 0.5425, 0.8575, 1,
};

Color gradient[NUM_GRADIENTS] = {
    {255,  0,   7,  50},
    {255, 32, 107, 203},
    {255,237, 255, 255},
    {255,255, 170,   0},
    {255,  0,   2,   0},
    {255,  0,   7,  99}};

#define NUM_COLORS 2048
Color palette[NUM_COLORS];

void init_palette() {
    for (int i=0; i<NUM_COLORS; i++) {
        double v = (double)i/(double)NUM_COLORS;
        Color* c = palette + i;
        
        Color c1;
        Color c2;
        double v1;
        for (int i=0; i<NUM_GRADIENTS-1; i++) {
            if (breaks[i] <= v && v < breaks[i+1]) {
                v1 = (v-breaks[i])/(breaks[i+1]-breaks[i]);
                c1 = gradient[i];
                c2 = gradient[i+1];
            }
        }
        
        c->r = LERP(v1, c1.r, c2.r);
        c->g = LERP(v1, c1.g, c2.g);
        c->b = LERP(v1, c1.b, c2.b);
        c->a = 255;
    }
}

void render(double x_min, double y_min, double x_max, double y_max) {
    
    static int scale = 256;
    static int shift = 0;
    static double ONE_OVER_LOG2 = 1.0 / log(2.0);
    
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
                CLAMP(0, hue, 1);
                color = palette[(int)(hue*NUM_COLORS)];
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

void assure_aspect_ratio(double ratio, double* x_min, double* y_min,
                         double* x_max, double* y_max) {
    double w = *x_max - *x_min;
    double h = *y_max - *y_min;
    double curr = w/h;
    double perc_diff = abs(curr-ratio)/ratio;
    if (ratio > curr) {
        double delta = w*perc_diff*0.5;
        *x_min -= delta;
        *x_max += delta;
    } else if (ratio < curr) {
        double delta = h*perc_diff*0.5;
        *y_min -= delta;
        *y_max += delta;
    }
}

int main(int argc, char** argv) {
    
    SDL_Init(SDL_INIT_VIDEO);
    
    init_palette();
    
    SDL_Window* window = SDL_CreateWindow("Mandelbrot",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          WIDTH, HEIGHT,
                                          SDL_WINDOW_SHOWN);
    renderer =  SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    
    double x_min = -2.5;
    double x_max = 1.0;
    double y_min = -1.1;
    double y_max = 1.1;
    
    double aspect = (double)WIDTH/(double)HEIGHT;
    assure_aspect_ratio(aspect, &x_min, &y_min, &x_max, &y_max);
    
    render(x_min, y_min, x_max, y_max);
    
    bool zooming = false;
    int zoom_start_x;
    int zoom_start_y;
    int zoom_end_x;
    int zoom_end_y;
    
    bool running = true;
    while(running) {
        
        SDL_PumpEvents();
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
                    
                    double aspect = (double)WIDTH/(double)HEIGHT;
                    assure_aspect_ratio(aspect, &x_min, &y_min, &x_max, &y_max);
                    
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
        
#if RENDER_PALETTE
        Color* p = palette;
        int x_reset=0;
        int y_offset=0;
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        for (int i=0; i<NUM_COLORS; i++) {
            SDL_SetRenderDrawColor(renderer, p->r, p->g, p->b, 0xFF);
            if (i % WIDTH == 0) {
                y_offset += 50;
            }
            SDL_RenderDrawLine(renderer, i%WIDTH, y_offset, i%WIDTH, y_offset+50);
            p++;
        }
        SDL_RenderPresent(renderer);
#endif
        
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