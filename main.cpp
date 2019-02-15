#include <stdio.h>
#include <complex>
#include <math.h>

#define SDL_MAIN_HANDLED
#include "SDL.h"

#define WIDTH 800
#define HEIGHT 600
#define MAX_ITERATION 255

#define LERP(x, min, max) (((max)-(min))*(x)+(min))

SDL_Renderer* renderer;
unsigned char pixels[WIDTH*HEIGHT];
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

Color palette[MAX_ITERATION];

Color map_color(float x) {
    Color result;
    result.v = (x) * 0xFFFFFF;
    return result;
}

void init_palette() {
    for (int i=0; i<MAX_ITERATION; i++) {
        palette[i].r = 0;
        palette[i].g = 255 - abs(300 - i);
        if (palette[i].g < 0) {
            palette[i].g = 0;
        }
        palette[i].b = 255 - abs(700 - i);
        if (palette[i].b < 0) {
            palette[i].b = 0;
        }
    }
}

#if 0
void render(double x_min, double y_min, double x_max, double y_max) {
    
    long start = SDL_GetTicks();
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 1);
    SDL_RenderClear(renderer);
    
    for (int py=0; py<HEIGHT; py++) {
        for (int px=0; px<WIDTH; px++) {
            double tx = (double)px/(double)WIDTH;
            double ty = (double)py/(double)HEIGHT;
            double x0 = LERP(tx, x_min, x_max);
            double y0 = LERP(ty, y_min, y_max);
            double x = 0.0;
            double y = 0.0;
            int iteration = 0;
            int max_iteration = 1000;
            while (x*x + y*y <= 256 && iteration < max_iteration) {
                double x_temp = x*x - y*y + x0;
                y = 2.0*x*y + y0;
                x = x_temp;
                iteration++;
            }
            
            double iter = iteration;
            if ( iteration < max_iteration ) {
                // sqrt of inner term removed using log simplification rules.
                double log_zn = log( x*x + y*y ) / 2.0;
                double log_2 = log(2.0);
                double nu = log( log_zn / log_2 ) / log_2;
                // Rearranging the potential function.
                // Dividing log_zn by log(2) instead of log(N = 1<<8)
                // because we want the entire palette to range from the
                // center to radius 2, NOT our bailout radius.
                iter = iter + 1.0 - nu;
            }
            Color color1 = palette[(int)floor(iter)];
            Color color2 = palette[(int)floor(iter) + 1];
            // iteration % 1 = fractional part of iteration.
            Color color;
            color.v = LERP(iter - floor(iter), color1.v, color2.v);
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 0xFF);
            SDL_RenderDrawPoint(renderer, px, py);
        }
    }
    SDL_RenderPresent(renderer);
    
    long end = SDL_GetTicks();
    printf("Time=%fs\n", (float)(end - start) / 1000.0f);
    
    /*
    For each pixel (Px, Py) on the screen, do:
    {
        x0 = scaled x coordinate of pixel (scaled to lie in the Mandelbrot X scale (-2.5, 1))
            y0 = scaled y coordinate of pixel (scaled to lie in the Mandelbrot Y scale (-1, 1))
            x = 0.0
            y = 0.0
            iteration = 0
            max_iteration = 1000
            // Here N=2^8 is chosen as a reasonable bailout radius.
            while ( x*x + y*y <= (1 << 16)  AND  iteration < max_iteration ) {
            xtemp = x*x - y*y + x0
                y = 2*x*y + y0
                x = xtemp
                iteration = iteration + 1
        }
        // Used to avoid floating point issues with points inside the set.
        if ( iteration < max_iteration ) {
            // sqrt of inner term removed using log simplification rules.
            log_zn = log( x*x + y*y ) / 2
                nu = log( log_zn / log(2) ) / log(2)
                // Rearranging the potential function.
                // Dividing log_zn by log(2) instead of log(N = 1<<8)
                // because we want the entire palette to range from the
                // center to radius 2, NOT our bailout radius.
                iteration = iteration + 1 - nu
        }
        color1 = palette[floor(iteration)]
            color2 = palette[floor(iteration) + 1]
            // iteration % 1 = fractional part of iteration.
            color = linear_interpolate(color1, color2, iteration % 1)
            plot(Px, Py, color)
    }
    */
}
#else
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
            
            Color color = map_color(hue);
            
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 0xFF);
            SDL_RenderDrawPoint(renderer, x, y);
            
            pixel++;
        }
    }
    
    SDL_RenderPresent(renderer);
    
    long end = SDL_GetTicks();
    printf("Time=%fs\n", (float)(end - start) / 1000.0f);
}
#endif

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
                    x_min = LERP(px_start, x_min, x_max);
                    x_max = LERP(px_end, x_min, x_max);
                    y_min = LERP(py_start, y_min, y_max);
                    y_max = LERP(py_end, y_min, y_max);
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
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_MOD);
            SDL_SetRenderDrawColor(renderer, 0, 0x7F, 0xFF, 0xFF);
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