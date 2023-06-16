#include "../include/sdl_wrapper.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

const char WINDOW_TITLE[] = "CS 3";
const int WINDOW_WIDTH = 1024;
const int WINDOW_HEIGHT = 704;
const double MS_PER_S = 1e3;

SDL_Texture *texture = NULL;

Mix_Music *background_music = NULL;

TTF_Font *font = NULL;

/**
 * The coordinate at the center of the screen.
 */
vector_t center;
/**
 * The coordinate difference from the center to the top right corner.
 */
vector_t max_diff;
/**
 * The SDL window where the scene is rendered.
 */
SDL_Window *window;
/**
 * The renderer used to draw the scene.
 */
SDL_Renderer *renderer;
/**
 * The keypress handler, or NULL if none has been configured.
 */
key_handler_t key_handler = NULL;
/**
 * SDL's timestamp when a key was last pressed or released.
 * Used to mesasure how long a key has been held.
 */
uint32_t key_start_timestamp;
/**
 * The value of clock() when time_since_last_tick() was last called.
 * Initially 0.
 */
clock_t last_clock = 0;

/** Computes the center of the window in pixel coordinates */
vector_t get_window_center(void) {
  int *width = malloc(sizeof(*width)), *height = malloc(sizeof(*height));
  assert(width != NULL);
  assert(height != NULL);
  SDL_GetWindowSize(window, width, height);
  vector_t dimensions = {.x = *width, .y = *height};
  free(width);
  free(height);
  return vec_multiply(0.5, dimensions);
}

/**
 * Computes the scaling factor between scene coordinates and pixel coordinates.
 * The scene is scaled by the same factor in the x and y dimensions,
 * chosen to maximize the size of the scene while keeping it in the window.
 */
double get_scene_scale(vector_t window_center) {
  // Scale scene so it fits entirely in the window
  double x_scale = window_center.x / max_diff.x,
         y_scale = window_center.y / max_diff.y;
  return x_scale < y_scale ? x_scale : y_scale;
}

/** Maps a scene coordinate to a window coordinate */
vector_t get_window_position(vector_t scene_pos, vector_t window_center) {
  // Scale scene coordinates by the scaling factor
  // and map the center of the scene to the center of the window
  vector_t scene_center_offset = vec_subtract(scene_pos, center);
  double scale = get_scene_scale(window_center);
  vector_t pixel_center_offset = vec_multiply(scale, scene_center_offset);
  vector_t pixel = {.x = round(window_center.x + pixel_center_offset.x),
                    // Flip y axis since positive y is down on the screen
                    .y = round(window_center.y - pixel_center_offset.y)};
  return pixel;
}

/**
 * Converts an SDL key code to a char.
 * 7-bit ASCII characters are just returned
 * and arrow keys are given special character codes.
 */
char get_keycode(SDL_Keycode key) {
  switch (key) {
  case SDLK_LEFT:
    return LEFT_ARROW;
  case SDLK_UP:
    return UP_ARROW;
  case SDLK_RIGHT:
    return RIGHT_ARROW;
  case SDLK_DOWN:
    return DOWN_ARROW;
  case SDLK_SPACE:
    return SPACE;
  case SDLK_w:
    return W;
  case SDLK_a:
    return A;
  case SDLK_s:
    return S;
  case SDLK_d:
    return D;
  case SDLK_q:
    return Q;
  case SDLK_e:
    return E;
  case SDLK_f:
    return F;
  case SDLK_t:
    return T;
  case SDLK_RETURN:
    return RET;
  case SDLK_1:
    return ONE;
  case SDLK_2:
    return TWO;
  case SDLK_3:
    return THREE;
  case SDLK_4:
    return FOUR;
  case SDLK_5:
    return FIVE;
  case SDLK_6:
    return SIX;
  case SDLK_7:
    return SEVEN;
  case SDLK_8:
    return EIGHT;
  case SDLK_ESCAPE:
    return ESC;
  case SDLK_r:
    return RULES;
  default:
    // Only process 7-bit ASCII characters
    return key == (SDL_Keycode)(char)key ? key : '\0';
  }
}

void sdl_init(vector_t min, vector_t max) {
  // Check parameters
  assert(min.x < max.x);
  assert(min.y < max.y);

  center = vec_multiply(0.5, vec_add(min, max));
  max_diff = vec_subtract(max, center);
  SDL_Init(SDL_INIT_EVERYTHING);
  window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT,
                            SDL_WINDOW_RESIZABLE);
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
}

bool sdl_is_done(state_t *state) {
  SDL_Event *event = malloc(sizeof(*event));
  assert(event != NULL);
  while (SDL_PollEvent(event)) {
    switch (event->type) {
    case SDL_QUIT:
      free(event);
      return true;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
      // Skip the keypress if no handler is configured
      // or an unrecognized key was pressed
      if (key_handler == NULL)
        break;
      char key = get_keycode(event->key.keysym.sym);
      if (key == '\0')
        break;

      uint32_t timestamp = event->key.timestamp;
      if (!event->key.repeat) {
        key_start_timestamp = timestamp;
      }
      key_event_type_t type =
          event->type == SDL_KEYDOWN ? KEY_PRESSED : KEY_RELEASED;
      double held_time = (timestamp - key_start_timestamp) / MS_PER_S;
      key_handler(state, key, type, held_time);
      break;
    }
  }
  free(event);
  return false;
}

void sdl_clear(void) {
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderClear(renderer);
}

void sdl_draw_polygon(list_t *points, rgb_color_t color) {
  // Check parameters
  size_t n = list_size(points);
  assert(n >= 3);
  assert(0 <= color.r && color.r <= 1);
  assert(0 <= color.g && color.g <= 1);
  assert(0 <= color.b && color.b <= 1);

  vector_t window_center = get_window_center();

  // Convert each vertex to a point on screen
  int16_t *x_points = malloc(sizeof(*x_points) * n),
          *y_points = malloc(sizeof(*y_points) * n);
  assert(x_points != NULL);
  assert(y_points != NULL);
  for (size_t i = 0; i < n; i++) {
    vector_t *vertex = list_get(points, i);
    vector_t pixel = get_window_position(*vertex, window_center);
    x_points[i] = pixel.x;
    y_points[i] = pixel.y;
  }

  // Draw polygon with the given color
  filledPolygonRGBA(renderer, x_points, y_points, n, color.r * 255,
                    color.g * 255, color.b * 255, 255);
  free(x_points);
  free(y_points);
}

void sdl_show(void) {
  // Draw boundary lines
  vector_t window_center = get_window_center();
  vector_t max = vec_add(center, max_diff),
           min = vec_subtract(center, max_diff);
  vector_t max_pixel = get_window_position(max, window_center),
           min_pixel = get_window_position(min, window_center);
  SDL_Rect *boundary = malloc(sizeof(*boundary));
  boundary->x = min_pixel.x;
  boundary->y = max_pixel.y;
  boundary->w = max_pixel.x - min_pixel.x;
  boundary->h = min_pixel.y - max_pixel.y;
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderDrawRect(renderer, boundary);
  free(boundary);

  SDL_RenderPresent(renderer);
}

SDL_Rect *get_dest_rect(list_t *shape) {
  vector_t window_center = get_window_center();
  SDL_Rect *rect = malloc(sizeof(SDL_Rect));
  vector_t *p = list_get(shape, 0);
  vector_t xy = {p->x, p->y};
  vector_t wh = {p->x, p->y};
  for (size_t i = 1; i < list_size(shape); i++) {
    p = list_get(shape, i);
    if (p->x < xy.x) {
      xy.x = p->x;
    }
    if (p->y > xy.y) {
      xy.y = p->y;
    }
    if (p->x > wh.x) {
      wh.x = p->x;
    }
    if (p->y < wh.y) {
      wh.y = p->y;
    }
  }
  xy = get_window_position(xy, window_center);
  wh = get_window_position(wh, window_center);
  rect->x = xy.x;
  rect->y = xy.y;
  rect->w = wh.x - xy.x;
  rect->h = wh.y - xy.y;
  return rect;
}

void sdl_render_scene(scene_t *scene) {
  sdl_clear();
  size_t body_count = scene_bodies(scene);
  // for (size_t i = 0; i < body_count; i++) {
  //   body_t *body = scene_get_body(scene, i);
  //   list_t *shape = body_get_shape(body);
  //   sdl_draw_polygon(shape, body_get_color(body));
  //   list_free(shape);
  // }

  for (size_t i = 0; i < body_count; i++) {
    body_t *body = scene_get_body(scene, i);
    list_t *shape = body_get_shape(body);
    SDL_Texture *image = (SDL_Texture *)body_get_image(body);
    SDL_Texture *text = (SDL_Texture *)body_get_text(body);
    SDL_Rect *dest_rect = get_dest_rect(shape);
    if (image != NULL) {
      SDL_RenderCopy(renderer, image, NULL, dest_rect);
    } else if (body_get_is_visible(body)) {
      sdl_draw_polygon(shape, body_get_color(body));
    }
    if (text != NULL) {
      double shift_factor = 0.25;
      double scale_factor = 0.5;
      dest_rect->x = dest_rect->x + shift_factor * dest_rect->w;
      dest_rect->y = dest_rect->y + shift_factor * dest_rect->h;
      dest_rect->w = dest_rect->w * scale_factor;
      dest_rect->h = dest_rect->h * scale_factor;
      SDL_RenderCopy(renderer, text, NULL, dest_rect);
    }
    list_free(shape);
    free(dest_rect);
  }
  sdl_show();
}

void sdl_on_key(key_handler_t handler) { key_handler = handler; }

vector_t sdl_get_mouse_pos() {
  int x, y;
  SDL_GetMouseState(&x, &y);

  return (vector_t){x, WINDOW_HEIGHT - y};
}

SDL_Texture *sdl_load_image(const char *image_filename) {
  // printf("rotation: %f\n", rotation);
  SDL_Surface *img_surface = IMG_Load(image_filename);
  // img_surface = rotozoomSurface(img_surface, rotation, 0, 0);
  SDL_Texture *img_texture =
      SDL_CreateTextureFromSurface(renderer, img_surface);

  // Free surface
  SDL_FreeSurface(img_surface);

  return img_texture;
}

SDL_Texture *sdl_load_text(char *text, rgb_color_t text_color,
                           const char *font_filename, size_t font_size) {
  // Convert rgb_color_t to SDL_Color
  SDL_Color color = {text_color.r * 255, text_color.g * 255, text_color.b * 255,
                     0};

  SDL_Texture *texture = NULL;

  // Render text surface
  TTF_Font *font = TTF_OpenFont(font_filename, font_size);
  // TTF_Font *font = TTF_OpenFont("assets/fonts/Arial.ttf", 25);
  if (!font) {
    printf("Unable to render font! SDL_ttf Error: %s\n", TTF_GetError());
  }

  SDL_Surface *text_surface = TTF_RenderText_Solid(font, text, color);
  if (!text_surface) {
    printf("Unable to render text surface! SDL_ttf Error: %s\n",
           TTF_GetError());
  } else {
    // Create texture from surface pixels
    texture = SDL_CreateTextureFromSurface(renderer, text_surface);
    if (!texture) {
      printf("Unable to create texture from rendered text! SDL Error: %s\n",
             SDL_GetError());
    }

    // Free surface
    SDL_FreeSurface(text_surface);
  }

  // Return success
  TTF_CloseFont(font);
  return texture;
}

Mix_Chunk *sdl_load_sound(const char *sound_filename) {
  // Checking to make sure mixer was initialized
  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
    printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n",
           Mix_GetError());
  }

  Mix_Chunk *sound = Mix_LoadWAV(sound_filename);
  assert(sound);
  return sound;
}

void sdl_play_sound(const char *sound_filename) {
  Mix_Chunk *sound_effect = sdl_load_sound(sound_filename);
  Mix_PlayChannel(-1, sound_effect, 0);
}

void sdl_start_background_music(const char *sound_filename) {
  // Checking to make sure mixer was initialized
  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
    printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n",
           Mix_GetError());
  }

  background_music = Mix_LoadMUS(sound_filename);
  assert(background_music);
  Mix_PlayMusic(background_music, -1);
}

void sdl_pause_background_music() { Mix_PauseMusic(); }

void sdl_resume_background_music() { Mix_ResumeMusic(); }

double time_since_last_tick(void) {
  clock_t now = clock();
  double difference = last_clock
                          ? (double)(now - last_clock) / CLOCKS_PER_SEC
                          : 0.0; // return 0 the first time this is called
  last_clock = now;
  return difference;
}
