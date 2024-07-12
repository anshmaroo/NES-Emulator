#include "controller.h"
#include <SDL2/SDL.h>

/**
 * @brief initalizes controller object
 * 
 * @return Controller* 
 */
Controller *InitController() {
    Controller *controller = (Controller *) malloc(sizeof(controller));
    controller->left = false;
    controller->right = false;
    controller->up = false;
    controller->down = false;

    controller->a = false;
    controller->b = false;
    
    controller->select = false;
    controller->start = false;
    controller->bus = NULL;

    reset_controller(controller);

    return controller;
}

/**
 * @brief reset controller state
 * 
 * @param controller 
 */
void reset_controller(Controller *controller) {
    controller->left = false;
    controller->right = false;
    controller->up = false;
    controller->down = false;

    controller->a = false;
    controller->b = false;
    
    controller->select = false;
    controller->start = false;

}

/**
 * @brief sets controller state
 * 
 * @param controller 
 * @param pressed_keys 
 */
void set_controller(Controller *controller, uint8_t *pressed_keys) {
    reset_controller(controller);

    if(pressed_keys[SDL_SCANCODE_A]) 
        controller->left = 1;
    if(pressed_keys[SDL_SCANCODE_D]) 
        controller->right = 1;
    if(pressed_keys[SDL_SCANCODE_W]) 
        controller->up = 1;
    if(pressed_keys[SDL_SCANCODE_S]) 
        controller->down = 1;
    if(pressed_keys[SDL_SCANCODE_L]) 
        controller->b = 1;
    if(pressed_keys[SDL_SCANCODE_SEMICOLON]) 
        controller->a = 1;
    if(pressed_keys[SDL_SCANCODE_MINUS]) 
        controller->select = 1;
    if(pressed_keys[SDL_SCANCODE_RETURN]) {
        // printf("START PRESSED!\n");
        controller->start = 1;
    }
}

/**
 * @brief get specified bit of controller
 * 
 * @param controller 
 * @param bit 
 * @return uint8_t 
 */
uint8_t read_from_controller(Controller *controller, uint8_t bit) {
    switch(bit) {
        case 0:
            if(controller->a)
                // printf("\n\tA READ!\n");
            return controller->a;
            break;
        case 1:
            if(controller->b)
                // printf("\n\tB READ!\n");
            return controller->b;
            break;
        case 2:
            if(controller->select)
                // printf("\n\tSELECT READ!\n");
            return controller->select;
            break;
        case 3:
            if(controller->start)
                // printf("\n\tSTART READ!\n");
            return controller->start;
            break;
        case 4:
            if(controller->up)
                // printf("\n\tUP READ!\n");
            return controller->up;
            break;
        case 5:
            if(controller->down)
                // printf("\n\tDOWN READ!\n");
            return controller->down;
            break;
        case 6:
            if(controller->left)
                // printf("\n\tLEFT READ!\n");
            return controller->left;
            break;
        case 7:
            if(controller->right)
                // printf("\n\tRIGHT READ!\n");
            return controller->right;
            break;

    }
    return 0;
}
