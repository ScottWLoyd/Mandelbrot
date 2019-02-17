# Mandelbrot
A Mandelbrot set renderer

Depends on SDL library.  The batch file builds with a static version of SDL, but you could run it with the DLL version just as easily.

Renders the Mandelbrot set in a color palette and supports zoom in via click-and-drag.  Renders using 24 threads which was the fastest render time tested on my machine.

Roadmap:
- Add support for runtime-configurable color palettes
- Add zoom out feature
- Add save image feature
- Add runtime-configurable level of detail (max iterations, histogram for color mapping, etc)
