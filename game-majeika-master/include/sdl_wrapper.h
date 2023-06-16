#ifndef __SDL_WRAPPER_H__
#define __SDL_WRAPPER_H__

#include "color.h"
#include "list.h"
#include "scene.h"
#include "state.h"
#include "vector.h"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2_rotozoom.h>
#include <stdbool.h>

// Values passed to a key handler when the given arrow key is pressed
typedef enum {
  LEFT_ARROW = 1,
  UP_ARROW = 2,
  RIGHT_ARROW = 3,
  DOWN_ARROW = 4,
  SPACE = 5,
  W = 6,
  A = 7,
  S = 8,
  D = 9,
  Q = 10,
  E = 11,
  F = 12,
  T = 13,
  RET = 14,
  ONE = 15,
  TWO = 16,
  THREE = 17,
  FOUR = 18,
  FIVE = 19,
  SIX = 20,
  SEVEN = 21,
  EIGHT = 22,
  ESC = 23,
  RULES = 24
} arrow_key_t;

/**
 * The possible types of key events.
 * Enum types in C are much more primitive than in Java; this is equivalent to:
 * typedef unsigned int KeyEventType;
 * #define KEY_PRESSED 0
 * #define KEY_RELEASED 1
 */
typedef enum { KEY_PRESSED, KEY_RELEASED } key_event_type_t;

/**
 * A keypress handler.
 * When a key is pressed or released, the handler is passed its char value.
 * Most keys are passed as their char value, e.g. 'a', '1', or '\r'.
 * Arrow keys have the special values listed above.
 *
 * @param state pointer to the current state of the program
 * @param key a character indicating which key was pressed
 * @param type the type of key event (KEY_PRESSED or KEY_RELEASED)
 * @param held_time if a press event, the time the key has been held in seconds
 */
typedef void (*key_handler_t)(state_t *state, char key, key_event_type_t type,
                              double held_time);

/**
 * Initializes the SDL window and renderer.
 * Must be called once before any of the other SDL functions.
 *
 * @param min the x and y coordinates of the bottom left of the scene
 * @param max the x and y coordinates of the top right of the scene
 */
void sdl_init(vector_t min, vector_t max);

/**
 * Processes all SDL events and returns whether the window has been closed.
 * This function must be called in order to handle keypresses.
 *
 * @param state pointer to the current state of the program
 * @return true if the window was closed, false otherwise
 */
bool sdl_is_done(state_t *state);

/**
 * Clears the screen. Should be called before drawing polygons in each frame.
 */
void sdl_clear(void);

/**
 * Draws a polygon from the given list of vertices and a color.
 *
 * @param points the list of vertices of the polygon
 * @param color the color used to fill in the polygon
 */
void sdl_draw_polygon(list_t *points, rgb_color_t color);

/**
 * Displays the rendered frame on the SDL window.
 * Must be called after drawing the polygons in order to show them.
 */
void sdl_show(void);

/**
 * Draws all bodies in a scene.
 * This internally calls sdl_clear(), sdl_draw_polygon(), and sdl_show(),
 * so those functions should not be called directly.
 *
 * @param scene the scene to draw
 */
void sdl_render_scene(scene_t *scene);

/**
 * Registers a function to be called every time a key is pressed.
 * Overwrites any existing handler.
 *
 * @param handler the function to call with each key press
 */
void sdl_on_key(key_handler_t handler);

/**
 * Gets the mouse coordinates on the screen.
 * 
 * @return a vector containing the x and y position
*/
vector_t sdl_get_mouse_pos();

/**
 * Initializes and loads the image according to the inputted image file
 * and rotation
 * 
 * @param image_filename a pointer to the filepath of the image
*/
SDL_Texture *sdl_load_image(const char *image_filename);

/**
 * Creates a text object to be rendered.
 * 
 * @param text string representing the text to be rendered
 * @param text_color color of the text
 * @param font_filename filepath containing the font file
 * @param font_size size of the font
 * 
 * @return a texture containing the rendered text
*/
SDL_Texture *sdl_load_text(char *text, rgb_color_t text_color,
                           const char *font_filename, size_t font_size);

/**
 * Initializes and plays the sound effect with the inputted sound file.
 * 
 * @param sound_filename a pointer to the filepath
*/
void sdl_play_sound(const char *sound_filename);

/**
 * Initializes and starts playing the background music.
 * 
 * @param sound_filename a pointer to the filepath containing the music file
*/
void sdl_start_background_music(const char *sound_filename);

/**
 * Pauses the background music.
*/
void sdl_pause_background_music();

/**
 * Resumes the background music.
*/
void sdl_resume_background_music();

/**
 * Gets the amount of time that has passed since the last time
 * this function was called, in seconds.
 *
 * @return the number of seconds that have elapsed
 */
double time_since_last_tick(void);

#endif // #ifndef __SDL_WRAPPER_H__
