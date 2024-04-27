#include <stdint.h>
#include <stdbool.h>

typedef struct Controller {
    // BUS
    struct Bus *bus;

    bool left;
    bool right;
    bool up;
    bool down;
    bool a;
    bool b;
    bool select;
    bool start;

} Controller;

/**
 * @brief initalizes controller object
 * 
 * @return Controller* 
 */
Controller *InitController();

/**
 * @brief reset controller state
 * 
 * @param controller 
 */
void reset_controller(Controller *controller);

/**
 * @brief sets controller state
 * 
 * @param controller 
 * @param pressed_keys 
 */
void set_controller(Controller *controller, uint8_t *pressed_keys);

/**
 * @brief get specified bit of controller
 * 
 * @param controller 
 * @param bit 
 * @return uint8_t 
 */
uint8_t read_from_controller(Controller *controller, uint8_t bit);

