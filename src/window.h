#include <stdbool.h>
#include <stdint.h>
#include "../include/SDL2/SDL.h"

/**
 * @brief initializes video
 * 
 * @return true 
 * @return false 
 */
bool init_SDL(void);

/**
 * @brief Create a window object
 * 
 * @param title 
 * @param width 
 * @param height 
 * @param scale 
 * @return SDL_Window* 
 */
SDL_Window *create_window(char *title, int width, int height);

/**
 * @brief Set the pixel object
 * 
 * @param window 
 * @param x 
 * @param y 
 * @param pix 
 */
void set_pixel(SDL_Window *window, int x, int y, uint32_t pix);

/**
 * @brief clear SDL resources
 * 
 */
void quit_sdl(void);