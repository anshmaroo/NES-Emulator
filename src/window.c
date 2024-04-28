#include "window.h"
#include <stdio.h>
/**
 * @brief initializes video
 * 
 * @return true 
 * @return false 
 */
bool init_SDL()
{
    /* Initialize SDL */
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());
        exit(0);
        return false;
    }

    // if (SDL_Init(SDL_INIT_AUDIO) < 0)
    // {
    //     fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());
    //     exit(0);
    //     return false;
    // }

    return true;
}

/**
 * @brief create a window object
 * 
 * @return SDL_Window* 
 */
SDL_Window *create_window(char *title, int width, int height) {
    int window_width = width;
    int window_height = height;

    SDL_Window *new_window = SDL_CreateWindow(
        title, 
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        window_width,
        window_height,
        SDL_WINDOW_RESIZABLE
    );

    if (!new_window)
    {
        fprintf(stderr, "Could not create SDL window: %s\n", SDL_GetError());
        return NULL;
    }

    return new_window;
}

/**
 * @brief Set the pixel object
 * 
 * @param window 
 * @param x 
 * @param y 
 * @param pix 
 */
void set_pixel(SDL_Window *window, int x, int y, uint32_t pix) {
    SDL_Surface *surface = SDL_GetWindowSurface(window);
    uint32_t *pixels = (uint32_t *)surface->pixels;
    pixels[(y * surface->w) + x] = pix;
}


/**
 * @brief clear SDL resources
 * 
 */
void quit_sdl(void) {
    SDL_Quit();
}