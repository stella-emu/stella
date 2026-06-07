// NOLINTBEGIN: Not under Stella control

#ifndef _MSC_VER
#include <sched.h>
#endif
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <stdarg.h>

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#include "libretro.h"

#include "StellaLIBRETRO.hxx"
#include "Event.hxx"
#include "NTSCFilter.hxx"
#include "Version.hxx"


static StellaLIBRETRO stella;

static retro_log_printf_t log_cb;
static retro_video_refresh_t video_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static retro_environment_t environ_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static bool libretro_supports_bitmasks;
static unsigned message_interface_version;

// Exposed to FSNodeLIBRETRO for VFS-based file operations
retro_vfs_interface* libretro_vfs = nullptr;
string libretro_save_dir;
string libretro_system_dir;
string libretro_rom_path;

// libretro UI settings
static SettingsLIBRETRO stella_settings;
static int crop_left;
static int crop_top;

static bool system_reset;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void post_message(const char* msg, retro_log_level level, unsigned duration_ms = 3000)
{
  if(log_cb) log_cb(level, "%s\n", msg);

  if(message_interface_version >= 1)
  {
    const retro_message_ext ext = {
      msg, duration_ms,
      level == RETRO_LOG_ERROR ? 3u : 1u,
      level, RETRO_MESSAGE_TARGET_ALL, RETRO_MESSAGE_TYPE_NOTIFICATION, -1
    };
    environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE_EXT, (void*)&ext);
  }
  else
  {
    const retro_message legacy = { msg, duration_ms * 60 / 1000 };
    environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, (void*)&legacy);
  }
}

void libretro_show_message(const char* msg)
{
  post_message(msg, RETRO_LOG_INFO);
}

static unsigned input_devices[4];
static int32_t input_crosshair[2];
static Controller::Type input_type[2];

static bool key_state[RETROK_LAST];

#define STELLA_DEVICE_JOYSTICK    RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0)
#define STELLA_DEVICE_BOOSTERGRIP RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 1)
#define STELLA_DEVICE_GENESIS     RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 2)
#define STELLA_DEVICE_JOY2BPLUS   RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 3)
#define STELLA_DEVICE_PADDLES     RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 4)
#define STELLA_DEVICE_DRIVING     RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 5)
#define STELLA_DEVICE_KEYBOARD    RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 6)
#define STELLA_DEVICE_TRAKBALL    RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 7)
#define STELLA_DEVICE_AMIGAMOUSE  RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 8)
#define STELLA_DEVICE_ATARIMOUSE  RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 9)
#define STELLA_DEVICE_LIGHTGUN    RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 10)
#define STELLA_DEVICE_QUADTARI    RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 11)
#define STELLA_DEVICE_MINDLINK    RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 12)
#define STELLA_DEVICE_ATARIVOX    RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 13)
#define STELLA_DEVICE_SAVEKEY     RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 14)
#define STELLA_DEVICE_KIDVID      RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 15)

// Unknown = use auto-detect from ROM database
static Controller::Type input_override[2];

static Controller::Type device_to_type(unsigned device)
{
  using enum Controller::Type;
  switch(device)
  {
    case STELLA_DEVICE_JOYSTICK:    return Joystick;
    case STELLA_DEVICE_BOOSTERGRIP: return BoosterGrip;
    case STELLA_DEVICE_GENESIS:     return Genesis;
    case STELLA_DEVICE_JOY2BPLUS:   return Joy2BPlus;
    case STELLA_DEVICE_PADDLES:     return Paddles;
    case STELLA_DEVICE_DRIVING:     return Driving;
    case STELLA_DEVICE_KEYBOARD:    return Keyboard;
    case STELLA_DEVICE_TRAKBALL:    return TrakBall;
    case STELLA_DEVICE_AMIGAMOUSE:  return AmigaMouse;
    case STELLA_DEVICE_ATARIMOUSE:  return AtariMouse;
    case STELLA_DEVICE_LIGHTGUN:    return Lightgun;
    case STELLA_DEVICE_QUADTARI:    return QuadTari;
    case STELLA_DEVICE_MINDLINK:    return MindLink;
    case STELLA_DEVICE_ATARIVOX:    return AtariVox;
    case STELLA_DEVICE_SAVEKEY:     return SaveKey;
    case STELLA_DEVICE_KIDVID:      return KidVid;
    default:                        return Unknown;
  }
}

// Tracks raw keyboard state for keyboard controller input.
// Note: retro_keyboard_event_t returns void so we cannot suppress RetroArch
// hotkeys here. Users should set a Hotkey Enable button in RetroArch settings
// (Settings -> Input -> Hotkeys -> Hotkey Enable) to prevent conflicts when
// playing games with keyboard controllers.
static void keyboard_event_cb(bool down, unsigned keycode,
                               uint32_t /*character*/, uint16_t /*mod*/)
{
  if(keycode < RETROK_LAST)
    key_state[keycode] = down;
}

void libretro_logger(int log_level, const char *source)
{
  retro_log_level log_mode = RETRO_LOG_INFO;
  char *string = strdup(source);
  char *token  = strtok(string, "\n");

  switch (log_level)
  {
    case 2: log_mode = RETRO_LOG_DEBUG; break;
    case 0: log_mode = RETRO_LOG_ERROR; break;
  }

  while (token != NULL)
  {
    log_cb(log_mode, "%s\n", token);
    token = strtok(NULL, "\n");
  }
  free(string);
  string = NULL;
}

// TODO input:
// https://github.com/libretro/blueMSX-libretro/blob/master/libretro.c
// https://github.com/libretro/libretro-o2em/blob/master/libretro.c

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint32_t libretro_read_rom(void* data)
{
  memcpy(data, stella.getROM(), stella.getROMSize());

  return stella.getROMSize();
}

uint32_t libretro_get_rom_size(void)
{
  return stella.getROMSize();
}

#define RETRO_ANALOG_COMMON() \
  bool mouse_l     = input_state_cb(pad, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT); \
  bool mouse_r     = input_state_cb(pad, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT); \
  int32_t mouse_x  = input_state_cb(pad, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X); \
  int32_t analog_x = input_state_cb(pad, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X); \
  *input_bitmask  |= mouse_l << RETRO_DEVICE_ID_JOYPAD_B; \
  if (stella_settings.paddle_analog_deadzone && abs(analog_x) < stella_settings.paddle_analog_deadzone * 0x7fff / 100) \
    analog_x = 0; \
  if (mouse_r) \
    mouse_x *= 3; \

static void retro_analog_paddle(unsigned pad, int32_t *analog_axis, int32_t *input_bitmask)
{
  RETRO_ANALOG_COMMON();

  if (mouse_x)
    *analog_axis += mouse_x * stella_settings.paddle_mouse_sensitivity;
  else if (!stella_settings.paddle_analog_absolute)
    *analog_axis += analog_x / 50;
  else
    *analog_axis  = analog_x;
  *analog_axis = BSPF::clamp(*analog_axis, -0x7fff, 0x7fff);
}

static void retro_analog_wheel(unsigned pad, int32_t *analog_axis, int32_t *input_bitmask)
{
  RETRO_ANALOG_COMMON();

  if (mouse_x)
    *analog_axis = mouse_x * stella_settings.paddle_mouse_sensitivity * 50;
  else
    *analog_axis = analog_x;

  *analog_axis = BSPF::clamp(*analog_axis, -0x7fff, 0x7fff);
}

static void draw_crosshair(int16_t x, int16_t y, uint16_t color)
{
   int i;
   int size       = 3;
   int width      = stella.getVideoWidthMax();
   int viewport_w = stella.getVideoWidth();
   int viewport_h = stella.getVideoHeight();
   uint8_t zoom   = stella.getVideoZoom();

   /* crosshair center position */
   uint32_t *ptr = (uint32_t *)stella.getVideoBuffer() + (y * width) + x;

   /* default crosshair dimension */
   int x_start = x - size * zoom;
   int x_end   = x + size * zoom;
   int y_start = y - size;
   int y_end   = y + size;

   if (zoom > 1)
     x_end++;

   /* off-screen */
   if (x <= 0 || y <= 0)
      return;

   /* framebuffer limits */
   if (x_start < 0) x_start = 0;
   if (x_end > viewport_w) x_end = viewport_w;
   if (y_start < 0) y_start = 0;
   if (y_end > viewport_h) y_end = viewport_h;

   /* draw crosshair */
   for (i = (x_start - x); i <= (x_end - x); i++)
   {
      ptr[i] = (i & zoom) ? color : 0xffffff;
   }
   for (i = (y_start - y); i <= (y_end - y); i++)
   {
      ptr[i * width] = (i & 1) ? color : 0xffffff;
      if (zoom > 1)
        ptr[(i * width) + 1] = (i & 1) ? color : 0xffffff;
   }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void update_input()
{
  if(!input_poll_cb) return;
  input_poll_cb();

#define EVENT stella.setInputEvent
  int32_t input_bitmask[4];
#define GET_BITMASK(pad) \
    if (libretro_supports_bitmasks) \
      input_bitmask[(pad)] = input_state_cb((pad), RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK); \
    else \
    { \
        input_bitmask[(pad)] = 0; \
        for (int i = 0; i <= RETRO_DEVICE_ID_JOYPAD_R3; i++) \
          input_bitmask[(pad)] |= input_state_cb((pad), RETRO_DEVICE_JOYPAD, 0, i) ? (1 << i) : 0; \
    }
#define MASK_EVENT(evt, pad, id) stella.setInputEvent((evt), (input_bitmask[(pad)] & (1 << (id))) ? 1 : 0)

  input_crosshair[0] = input_crosshair[1] = 0;

  int pad = 0;
  GET_BITMASK(pad)
  switch(input_type[0])
  {
    using enum Controller::Type;
    case Driving:
    {
      int32_t wheel = 0;

      retro_analog_wheel(pad, &wheel, &input_bitmask[pad]);
      EVENT(Event::LeftDrivingAnalog, wheel);
      MASK_EVENT(Event::LeftDrivingCCW,  pad, RETRO_DEVICE_ID_JOYPAD_LEFT);
      MASK_EVENT(Event::LeftDrivingCW,   pad, RETRO_DEVICE_ID_JOYPAD_RIGHT);
      MASK_EVENT(Event::LeftDrivingFire, pad, RETRO_DEVICE_ID_JOYPAD_B);
      break;
    }

    case PaddlesIAxis:
    case PaddlesIAxDr:
    case Paddles:
    {
      static int32_t paddle_a = 0;
      static int32_t paddle_b = 0;

      retro_analog_paddle(pad, &paddle_a, &input_bitmask[pad]);
      EVENT(Event::LeftPaddleAAnalog, paddle_a);
      MASK_EVENT(Event::LeftPaddleAIncrease, pad, RETRO_DEVICE_ID_JOYPAD_LEFT);
      MASK_EVENT(Event::LeftPaddleADecrease, pad, RETRO_DEVICE_ID_JOYPAD_RIGHT);
      MASK_EVENT(Event::LeftPaddleAFire,     pad, RETRO_DEVICE_ID_JOYPAD_B);

      pad++;
      GET_BITMASK(pad)

      retro_analog_paddle(pad, &paddle_b, &input_bitmask[pad]);
      EVENT(Event::LeftPaddleBAnalog, paddle_b);
      MASK_EVENT(Event::LeftPaddleBIncrease, pad, RETRO_DEVICE_ID_JOYPAD_LEFT);
      MASK_EVENT(Event::LeftPaddleBDecrease, pad, RETRO_DEVICE_ID_JOYPAD_RIGHT);
      MASK_EVENT(Event::LeftPaddleBFire,     pad, RETRO_DEVICE_ID_JOYPAD_B);
      break;
    }

    case Keyboard:
      EVENT(Event::LeftKeyboard1,     key_state[RETROK_1]);
      EVENT(Event::LeftKeyboard2,     key_state[RETROK_2]);
      EVENT(Event::LeftKeyboard3,     key_state[RETROK_3]);
      EVENT(Event::LeftKeyboard4,     key_state[RETROK_q]);
      EVENT(Event::LeftKeyboard5,     key_state[RETROK_w]);
      EVENT(Event::LeftKeyboard6,     key_state[RETROK_e]);
      EVENT(Event::LeftKeyboard7,     key_state[RETROK_a]);
      EVENT(Event::LeftKeyboard8,     key_state[RETROK_s]);
      EVENT(Event::LeftKeyboard9,     key_state[RETROK_d]);
      EVENT(Event::LeftKeyboardStar,  key_state[RETROK_z]);
      EVENT(Event::LeftKeyboard0,     key_state[RETROK_x]);
      EVENT(Event::LeftKeyboardPound, key_state[RETROK_c]);
      break;

    case QuadTari:
    {
      MASK_EVENT(Event::LeftJoystickLeft,  pad, RETRO_DEVICE_ID_JOYPAD_LEFT);
      MASK_EVENT(Event::LeftJoystickRight, pad, RETRO_DEVICE_ID_JOYPAD_RIGHT);
      MASK_EVENT(Event::LeftJoystickUp,    pad, RETRO_DEVICE_ID_JOYPAD_UP);
      MASK_EVENT(Event::LeftJoystickDown,  pad, RETRO_DEVICE_ID_JOYPAD_DOWN);
      MASK_EVENT(Event::LeftJoystickFire,  pad, RETRO_DEVICE_ID_JOYPAD_B);
      GET_BITMASK(2)
      MASK_EVENT(Event::QTJoystickThreeUp,    2, RETRO_DEVICE_ID_JOYPAD_UP);
      MASK_EVENT(Event::QTJoystickThreeDown,  2, RETRO_DEVICE_ID_JOYPAD_DOWN);
      MASK_EVENT(Event::QTJoystickThreeLeft,  2, RETRO_DEVICE_ID_JOYPAD_LEFT);
      MASK_EVENT(Event::QTJoystickThreeRight, 2, RETRO_DEVICE_ID_JOYPAD_RIGHT);
      MASK_EVENT(Event::QTJoystickThreeFire,  2, RETRO_DEVICE_ID_JOYPAD_B);
      break;
    }

    case Lightgun:
    {
      // scale from -0x8000..0x7fff to image rect
      const Common::Rect& rect = stella.getImageRect();
      const int32_t x = (input_state_cb(pad, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_SCREEN_X) + 0x7fff) * rect.w() / 0xffff;
      const int32_t y = (input_state_cb(pad, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_SCREEN_Y) + 0x7fff) * rect.h() / 0xffff;

      input_crosshair[0] = x > 0 && x < 0x7fff ? x * stella.getVideoWidth() / rect.w() : 0;
      input_crosshair[1] = y > 0 && y < 0x7fff ? y * stella.getVideoHeight() / rect.h() : 0;

      EVENT(Event::MouseAxisXValue, x);
      EVENT(Event::MouseAxisYValue, y);
      EVENT(Event::MouseButtonLeftValue, input_state_cb(pad, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_TRIGGER));
      EVENT(Event::MouseButtonRightValue, input_state_cb(pad, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_TRIGGER));
      break;
    }

    case Controller::Type::AmigaMouse:
    case Controller::Type::AtariMouse:
    case Controller::Type::TrakBall:
    {
      bool mouse_l     = input_state_cb(pad, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);
      bool mouse_r     = input_state_cb(pad, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT);
      int32_t mouse_x  = input_state_cb(pad, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
      int32_t mouse_y  = input_state_cb(pad, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
      int32_t analog_x = input_state_cb(pad, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
      int32_t analog_y = input_state_cb(pad, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);
      float analog_mag = sqrt((analog_x * analog_x) + (analog_y * analog_y));

      if (stella_settings.paddle_analog_deadzone && analog_mag <= stella_settings.paddle_analog_deadzone * 0x7fff / 100)
        analog_x = analog_y = 0;

      mouse_x += analog_x / (80000 / stella_settings.paddle_analog_sensitivity);
      mouse_y += analog_y / (80000 / stella_settings.paddle_analog_sensitivity);

      if (input_bitmask[pad] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT))
        mouse_x -= stella_settings.paddle_joypad_sensitivity;
      else if (input_bitmask[pad] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT))
        mouse_x += stella_settings.paddle_joypad_sensitivity;

      if (input_bitmask[pad] & (1 << RETRO_DEVICE_ID_JOYPAD_UP))
        mouse_y -= stella_settings.paddle_joypad_sensitivity;
      else if (input_bitmask[pad] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN))
        mouse_y += stella_settings.paddle_joypad_sensitivity;

      if (input_bitmask[pad] & (1 << RETRO_DEVICE_ID_JOYPAD_B))
        mouse_l = true;
      if (input_bitmask[pad] & (1 << RETRO_DEVICE_ID_JOYPAD_A))
        mouse_r = true;

      EVENT(Event::MouseAxisXMove, mouse_x);
      EVENT(Event::MouseAxisYMove, mouse_y);
      EVENT(Event::MouseButtonLeftValue,  mouse_l);
      EVENT(Event::MouseButtonRightValue, mouse_r);
      break;
    }

    case MindLink:
    {
      int32_t mouse_x = input_state_cb(pad, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
      int32_t analog_x = input_state_cb(pad, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);

      if (stella_settings.paddle_analog_deadzone && std::abs(analog_x) <= stella_settings.paddle_analog_deadzone * 0x7fff / 100)
        analog_x = 0;

      mouse_x += analog_x / (80000 / stella_settings.paddle_analog_sensitivity);

      if (input_bitmask[pad] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT))
        mouse_x -= stella_settings.paddle_joypad_sensitivity;
      else if (input_bitmask[pad] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT))
        mouse_x += stella_settings.paddle_joypad_sensitivity;

      EVENT(Event::MouseAxisXMove, mouse_x);
      EVENT(Event::MouseButtonLeftValue,  input_state_cb(pad, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT)
                                        | (bool)(input_bitmask[pad] & (1 << RETRO_DEVICE_ID_JOYPAD_B)));
      EVENT(Event::MouseButtonRightValue, input_state_cb(pad, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT)
                                        | (bool)(input_bitmask[pad] & (1 << RETRO_DEVICE_ID_JOYPAD_A)));
      break;
    }

    case Joy2BPlus:
    case BoosterGrip:
      MASK_EVENT(Event::LeftJoystickFire9, pad, RETRO_DEVICE_ID_JOYPAD_Y);
      [[fallthrough]];
    case Genesis:
      MASK_EVENT(Event::LeftJoystickFire5, pad, RETRO_DEVICE_ID_JOYPAD_A);
      [[fallthrough]];
    case Joystick:
    default:
      MASK_EVENT(Event::LeftJoystickLeft, pad, RETRO_DEVICE_ID_JOYPAD_LEFT);
      MASK_EVENT(Event::LeftJoystickRight, pad, RETRO_DEVICE_ID_JOYPAD_RIGHT);
      MASK_EVENT(Event::LeftJoystickUp, pad, RETRO_DEVICE_ID_JOYPAD_UP);
      MASK_EVENT(Event::LeftJoystickDown, pad, RETRO_DEVICE_ID_JOYPAD_DOWN);
      MASK_EVENT(Event::LeftJoystickFire, pad, RETRO_DEVICE_ID_JOYPAD_B);
      break;
  }
  pad++;
  GET_BITMASK(pad)

  switch(input_type[1])
  {
    using enum Controller::Type;
    case Driving:
    {
      int32_t wheel = 0;

      retro_analog_wheel(pad, &wheel, &input_bitmask[pad]);
      EVENT(Event::RightDrivingAnalog, wheel);
      MASK_EVENT(Event::RightDrivingCCW,  pad, RETRO_DEVICE_ID_JOYPAD_LEFT);
      MASK_EVENT(Event::RightDrivingCW,   pad, RETRO_DEVICE_ID_JOYPAD_RIGHT);
      MASK_EVENT(Event::RightDrivingFire, pad, RETRO_DEVICE_ID_JOYPAD_B);
      break;
    }

    case PaddlesIAxis:
    case PaddlesIAxDr:
    case Paddles:
    {
      static int32_t paddle_a = 0;
      static int32_t paddle_b = 0;

      retro_analog_paddle(pad, &paddle_a, &input_bitmask[pad]);
      EVENT(Event::RightPaddleAAnalog, paddle_a);
      MASK_EVENT(Event::RightPaddleAIncrease, pad, RETRO_DEVICE_ID_JOYPAD_LEFT);
      MASK_EVENT(Event::RightPaddleADecrease, pad, RETRO_DEVICE_ID_JOYPAD_RIGHT);
      MASK_EVENT(Event::RightPaddleAFire, pad, RETRO_DEVICE_ID_JOYPAD_B);

      pad++;
      GET_BITMASK(pad)

      retro_analog_paddle(pad, &paddle_b, &input_bitmask[pad]);
      EVENT(Event::RightPaddleBAnalog, paddle_b);
      MASK_EVENT(Event::RightPaddleBIncrease, pad, RETRO_DEVICE_ID_JOYPAD_LEFT);
      MASK_EVENT(Event::RightPaddleBDecrease, pad, RETRO_DEVICE_ID_JOYPAD_RIGHT);
      MASK_EVENT(Event::RightPaddleBFire, pad, RETRO_DEVICE_ID_JOYPAD_B);
      break;
    }

    case KidVid:
      // Tape selection: keyboard keys 8/9/0 or joypad B/A/Y
      EVENT(Event::RightKeyboard1, key_state[RETROK_8] | (bool)(input_bitmask[pad] & (1 << RETRO_DEVICE_ID_JOYPAD_B)));
      EVENT(Event::RightKeyboard2, key_state[RETROK_9] | (bool)(input_bitmask[pad] & (1 << RETRO_DEVICE_ID_JOYPAD_A)));
      EVENT(Event::RightKeyboard3, key_state[RETROK_0] | (bool)(input_bitmask[pad] & (1 << RETRO_DEVICE_ID_JOYPAD_Y)));
      // Skip/stop song: keyboard key O or joypad X
      EVENT(Event::RightKeyboard6, key_state[RETROK_o] | (bool)(input_bitmask[pad] & (1 << RETRO_DEVICE_ID_JOYPAD_X)));
      break;

    case Keyboard:
      EVENT(Event::RightKeyboard1,     key_state[RETROK_8]);
      EVENT(Event::RightKeyboard2,     key_state[RETROK_9]);
      EVENT(Event::RightKeyboard3,     key_state[RETROK_0]);
      EVENT(Event::RightKeyboard4,     key_state[RETROK_i]);
      EVENT(Event::RightKeyboard5,     key_state[RETROK_o]);
      EVENT(Event::RightKeyboard6,     key_state[RETROK_p]);
      EVENT(Event::RightKeyboard7,     key_state[RETROK_k]);
      EVENT(Event::RightKeyboard8,     key_state[RETROK_l]);
      EVENT(Event::RightKeyboard9,     key_state[RETROK_SEMICOLON]);
      EVENT(Event::RightKeyboardStar,  key_state[RETROK_COMMA]);
      EVENT(Event::RightKeyboard0,     key_state[RETROK_PERIOD]);
      EVENT(Event::RightKeyboardPound, key_state[RETROK_SLASH]);
      break;

    case QuadTari:
    {
      MASK_EVENT(Event::RightJoystickLeft,  pad, RETRO_DEVICE_ID_JOYPAD_LEFT);
      MASK_EVENT(Event::RightJoystickRight, pad, RETRO_DEVICE_ID_JOYPAD_RIGHT);
      MASK_EVENT(Event::RightJoystickUp,    pad, RETRO_DEVICE_ID_JOYPAD_UP);
      MASK_EVENT(Event::RightJoystickDown,  pad, RETRO_DEVICE_ID_JOYPAD_DOWN);
      MASK_EVENT(Event::RightJoystickFire,  pad, RETRO_DEVICE_ID_JOYPAD_B);
      GET_BITMASK(3)
      MASK_EVENT(Event::QTJoystickFourUp,    3, RETRO_DEVICE_ID_JOYPAD_UP);
      MASK_EVENT(Event::QTJoystickFourDown,  3, RETRO_DEVICE_ID_JOYPAD_DOWN);
      MASK_EVENT(Event::QTJoystickFourLeft,  3, RETRO_DEVICE_ID_JOYPAD_LEFT);
      MASK_EVENT(Event::QTJoystickFourRight, 3, RETRO_DEVICE_ID_JOYPAD_RIGHT);
      MASK_EVENT(Event::QTJoystickFourFire,  3, RETRO_DEVICE_ID_JOYPAD_B);
      break;
    }

    case MindLink:
    {
      int32_t mouse_x = input_state_cb(pad, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
      int32_t analog_x = input_state_cb(pad, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);

      if (stella_settings.paddle_analog_deadzone && std::abs(analog_x) <= stella_settings.paddle_analog_deadzone * 0x7fff / 100)
        analog_x = 0;

      mouse_x += analog_x / (80000 / stella_settings.paddle_analog_sensitivity);

      if (input_bitmask[pad] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT))
        mouse_x -= stella_settings.paddle_joypad_sensitivity;
      else if (input_bitmask[pad] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT))
        mouse_x += stella_settings.paddle_joypad_sensitivity;

      EVENT(Event::MouseAxisXMove, mouse_x);
      EVENT(Event::MouseButtonLeftValue,  input_state_cb(pad, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT)
                                        | (bool)(input_bitmask[pad] & (1 << RETRO_DEVICE_ID_JOYPAD_B)));
      EVENT(Event::MouseButtonRightValue, input_state_cb(pad, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT)
                                        | (bool)(input_bitmask[pad] & (1 << RETRO_DEVICE_ID_JOYPAD_A)));
      break;
    }

    case Joy2BPlus:
    case BoosterGrip:
      MASK_EVENT(Event::RightJoystickFire9, pad, RETRO_DEVICE_ID_JOYPAD_Y);
      [[fallthrough]];
    case Genesis:
      MASK_EVENT(Event::RightJoystickFire5, pad, RETRO_DEVICE_ID_JOYPAD_A);
      [[fallthrough]];
    case Joystick:
    default:
      MASK_EVENT(Event::RightJoystickLeft, pad, RETRO_DEVICE_ID_JOYPAD_LEFT);
      MASK_EVENT(Event::RightJoystickRight, pad, RETRO_DEVICE_ID_JOYPAD_RIGHT);
      MASK_EVENT(Event::RightJoystickUp, pad, RETRO_DEVICE_ID_JOYPAD_UP);
      MASK_EVENT(Event::RightJoystickDown, pad, RETRO_DEVICE_ID_JOYPAD_DOWN);
      MASK_EVENT(Event::RightJoystickFire, pad, RETRO_DEVICE_ID_JOYPAD_B);
      break;
  }

  // Notes:
  // - Each event can only be assigned once, in case of conflicts ususally the latest assignment will be active
  // - The follwing events can also be used by analog devices
  MASK_EVENT(Event::ConsoleLeftDiffA,  0, RETRO_DEVICE_ID_JOYPAD_L);
  MASK_EVENT(Event::ConsoleLeftDiffB,  0, RETRO_DEVICE_ID_JOYPAD_L2);
  MASK_EVENT(Event::ConsoleColor,      0, RETRO_DEVICE_ID_JOYPAD_L3);
  MASK_EVENT(Event::ConsoleRightDiffA, 0, RETRO_DEVICE_ID_JOYPAD_R);
  MASK_EVENT(Event::ConsoleRightDiffB, 0, RETRO_DEVICE_ID_JOYPAD_R2);
  MASK_EVENT(Event::ConsoleBlackWhite, 0, RETRO_DEVICE_ID_JOYPAD_R3);
  MASK_EVENT(Event::ConsoleSelect,     0, RETRO_DEVICE_ID_JOYPAD_SELECT);
  MASK_EVENT(Event::ConsoleReset,      0, RETRO_DEVICE_ID_JOYPAD_START);
  if (stella_settings.reload)
    MASK_EVENT(Event::ReloadConsole,   0, RETRO_DEVICE_ID_JOYPAD_X);

#undef EVENT
#undef MASK_EVENT
#undef GET_BITMASK
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void update_geometry()
{
  struct retro_system_av_info av_info;

  retro_get_system_av_info(&av_info);

  environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &av_info);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void update_system_av()
{
  struct retro_system_av_info av_info;

  retro_get_system_av_info(&av_info);

  environ_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &av_info);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void update_variables(bool init = false)
{
  bool geometry_update = false;

  struct retro_variable var;

#define RETRO_GET(x) \
  var.key = x; \
  var.value = NULL; \
  if(environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)

  RETRO_GET("stella_filter")
  {
    NTSCFilter::Preset value = NTSCFilter::Preset::OFF;

    if(!strcmp(var.value, "disabled"))            value = NTSCFilter::Preset::OFF;
    else if(!strcmp(var.value, "composite"))      value = NTSCFilter::Preset::COMPOSITE;
    else if(!strcmp(var.value, "s-video"))        value = NTSCFilter::Preset::SVIDEO;
    else if(!strcmp(var.value, "rgb"))            value = NTSCFilter::Preset::RGB;
    else if(!strcmp(var.value, "badly adjusted")) value = NTSCFilter::Preset::BAD;

    if(stella_settings.video_filter != value)
    {
      stella_settings.video_filter = value;
      stella.setVideoFilter(value);
      geometry_update = true;
    }
  }

  RETRO_GET("stella_crop_hoverscan")
  {
    stella_settings.crop_hoverscan = !strcmp(var.value, "enabled");

    geometry_update = true;
  }

  RETRO_GET("stella_crop_voverscan")
  {
    stella_settings.crop_voverscan = atoi(var.value);

    geometry_update = true;
  }

  RETRO_GET("stella_ntsc_aspect")
  {
    uInt32 value = strcmp(var.value, "par") ? static_cast<uInt32>(atoi(var.value)) : 0;

    if(stella_settings.video_aspect_ntsc != value)
    {
      stella_settings.video_aspect_ntsc = value;
      geometry_update = true;
    }
  }

  RETRO_GET("stella_pal_aspect")
  {
    uInt32 value = strcmp(var.value, "par") ? static_cast<uInt32>(atoi(var.value)) : 0;

    if(stella_settings.video_aspect_pal != value)
    {
      stella_settings.video_aspect_pal = value;
      geometry_update = true;
    }
  }

  RETRO_GET("stella_palette")
  {
    if(stella_settings.video_palette != var.value)
    {
      stella_settings.video_palette = var.value;
      stella.setVideoPalette(stella_settings.video_palette);
    }
  }

  RETRO_GET("stella_console")
  {
    const char* fmt = "AUTO";

    if(!strcmp(var.value, "ntsc"))     fmt = "NTSC";
    else if(!strcmp(var.value, "pal")) fmt = "PAL";
    else if(!strcmp(var.value, "secam"))   fmt = "SECAM";
    else if(!strcmp(var.value, "ntsc50")) fmt = "NTSC50";
    else if(!strcmp(var.value, "pal60"))   fmt = "PAL60";
    else if(!strcmp(var.value, "secam60")) fmt = "SECAM60";

    if(stella_settings.console_format != fmt)
    {
      stella_settings.console_format = fmt;
      system_reset = true;
    }
  }

  RETRO_GET("stella_stereo")
  {
    const char* mode = "byrom";

    if(!strcmp(var.value, "off")) mode = "mono";
    else if(!strcmp(var.value, "on")) mode = "stereo";

    if(stella_settings.audio_mode != mode)
    {
      stella_settings.audio_mode = mode;
      stella.setAudioStereo(stella_settings.audio_mode);
    }
  }

  RETRO_GET("stella_phosphor")
  {
    const char* phosphor = "byrom";

    if(!strcmp(var.value, "off")) phosphor = "never";
    else if(!strcmp(var.value, "on")) phosphor = "always";

    if(stella_settings.video_phosphor != phosphor)
    {
      stella_settings.video_phosphor = phosphor;
      stella.setVideoPhosphor(stella_settings.video_phosphor, stella_settings.video_phosphor_blend);
    }
  }

  RETRO_GET("stella_phosphor_blend")
  {
    uInt32 value = atoi(var.value);

    if(stella_settings.video_phosphor_blend != value)
    {
      stella_settings.video_phosphor_blend = value;
      stella.setVideoPhosphor(stella_settings.video_phosphor, stella_settings.video_phosphor_blend);
    }
  }

  RETRO_GET("stella_paddle_joypad_sensitivity")
  {
    int value = atoi(var.value);

    if(stella_settings.paddle_joypad_sensitivity != value)
    {
      if(!init) stella.setPaddleJoypadSensitivity(value);

      stella_settings.paddle_joypad_sensitivity = value;
    }
  }

  RETRO_GET("stella_paddle_analog_sensitivity")
  {
    int value = atoi(var.value);

    if(stella_settings.paddle_analog_sensitivity != value)
    {
      if(!init) stella.setPaddleAnalogSensitivity(value);

      stella_settings.paddle_analog_sensitivity = value;
    }
  }

  RETRO_GET("stella_reload")
  {
    stella_settings.reload = !strcmp(var.value, "on");
  }

  RETRO_GET("stella_messages")
  {
    bool value = !strcmp(var.value, "enabled");

    if(stella_settings.info_messages != value)
    {
      stella_settings.info_messages = value;
      stella.setMessages(value);
    }
  }

  RETRO_GET("stella_detect_pal60")
  {
    bool value = !strcmp(var.value, "enabled");

    if(stella_settings.detect_pal60 != value)
    {
      stella_settings.detect_pal60 = value;
      system_reset = true;
    }
  }

  RETRO_GET("stella_detect_ntsc50")
  {
    bool value = !strcmp(var.value, "enabled");

    if(stella_settings.detect_ntsc50 != value)
    {
      stella_settings.detect_ntsc50 = value;
      system_reset = true;
    }
  }

  bool pal_changed = false;

  RETRO_GET("stella_pal_contrast")
  {
    float value = atoi(var.value) * 0.1F;
    if(stella_settings.pal_contrast != value) { stella_settings.pal_contrast = value; pal_changed = true; }
  }

  RETRO_GET("stella_pal_brightness")
  {
    float value = atoi(var.value) * 0.1F;
    if(stella_settings.pal_brightness != value) { stella_settings.pal_brightness = value; pal_changed = true; }
  }

  RETRO_GET("stella_pal_hue")
  {
    float value = atoi(var.value) * 0.1F;
    if(stella_settings.pal_hue != value) { stella_settings.pal_hue = value; pal_changed = true; }
  }

  RETRO_GET("stella_pal_saturation")
  {
    float value = atoi(var.value) * 0.1F;
    if(stella_settings.pal_saturation != value) { stella_settings.pal_saturation = value; pal_changed = true; }
  }

  RETRO_GET("stella_pal_gamma")
  {
    float value = atoi(var.value) * 0.1F;
    if(stella_settings.pal_gamma != value) { stella_settings.pal_gamma = value; pal_changed = true; }
  }

  if(pal_changed)
    stella.setPaletteAdjust(stella_settings.pal_contrast, stella_settings.pal_brightness,
                            stella_settings.pal_hue, stella_settings.pal_saturation, stella_settings.pal_gamma);

  RETRO_GET("stella_dpc_pitch")
  {
    uInt32 value = atoi(var.value);

    if(stella_settings.dpc_pitch != value)
    {
      stella_settings.dpc_pitch = value;
      stella.setDpcPitch(value);
    }
  }

  RETRO_GET("stella_paddle_mouse_sensitivity")
  {
    stella_settings.paddle_mouse_sensitivity = atoi(var.value);
  }

  RETRO_GET("stella_paddle_analog_deadzone")
  {
    stella_settings.paddle_analog_deadzone = atoi(var.value);
  }

  RETRO_GET("stella_paddle_analog_absolute")
  {
    stella_settings.paddle_analog_absolute = !strcmp(var.value, "enabled");
  }

  RETRO_GET("stella_lightgun_crosshair")
  {
    stella_settings.lightgun_crosshair = !strcmp(var.value, "enabled");
  }

  if(!init && !system_reset)
  {
    crop_left = stella_settings.crop_hoverscan ? (stella.getVideoZoom() == 2 ? 32 : 8) : 0;
    crop_top  = stella_settings.crop_voverscan;

    if(geometry_update) update_geometry();
  }

#undef RETRO_GET
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static bool reset_system()
{
  // clean restart
  stella.destroy();

  // apply pre-boot settings first
  update_variables(true);

  // start system
  if(!stella.create(stella_settings, log_cb ? true : false)) return false;

  // expose RIOT RAM at its native 6502 address for RetroAchievements
  const retro_memory_descriptor mem_desc = {
    RETRO_MEMDESC_SYSTEM_RAM, stella.getRAM(), 0, 0x80, 0, 0, stella.getRAMSize(), nullptr
  };
  const retro_memory_map mem_map = { &mem_desc, 1 };
  environ_cb(RETRO_ENVIRONMENT_SET_MEMORY_MAPS, (void*)&mem_map);

  // get auto-detect controllers, then apply any user override
  input_type[0] = stella.getLeftControllerType();
  input_type[1] = stella.getRightControllerType();
  if(input_override[0] != Controller::Type::Unknown) input_type[0] = input_override[0];
  if(input_override[1] != Controller::Type::Unknown) input_type[1] = input_override[1];

  stella.setPaddleJoypadSensitivity(stella_settings.paddle_joypad_sensitivity);
  stella.setPaddleAnalogSensitivity(stella_settings.paddle_analog_sensitivity);

  system_reset = false;

  // reset libretro window, apply post-boot settings
  update_variables(false);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unsigned retro_api_version()
{
  return RETRO_API_VERSION;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unsigned retro_get_region()
{
  return stella.getVideoNTSC() ? RETRO_REGION_NTSC : RETRO_REGION_PAL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
void retro_set_audio_sample(retro_audio_sample_t cb) { audio_cb = cb; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }
void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void retro_get_system_info(struct retro_system_info *info)
{
  *info = retro_system_info{};  // reset to defaults


  info->library_name = stella.getCoreName();
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
  static char library_version[100]{};
  snprintf(library_version, 99, "%s%s", STELLA_VERSION.data(), GIT_VERSION);
  info->library_version = library_version;
  info->valid_extensions = stella.getROMExtensions();
  info->need_fullpath = false;
  info->block_extract = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void retro_get_system_av_info(struct retro_system_av_info *info)
{
  *info = retro_system_av_info{};  // reset to defaults
  unsigned crop_width = crop_left ? 8 : 0;

  info->timing.fps            = stella.getVideoRate();
  info->timing.sample_rate    = stella.getAudioRate();

  info->geometry.base_width   = stella.getRenderWidth();
  info->geometry.base_height  = stella.getRenderHeight();

  info->geometry.max_width    = stella.getVideoWidthMax();
  info->geometry.max_height   = stella.getVideoHeightMax();

  info->geometry.aspect_ratio = stella.getVideoAspectPar(stella_settings.video_aspect_ntsc, stella_settings.video_aspect_pal) *
      (float)(160 - crop_width) * 2 / (float)(stella.getVideoHeight() - (crop_top * 2));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void retro_set_controller_port_device(unsigned port, unsigned device)
{
  if(port >= 4) return;

  input_devices[port] = device;

  if(port >= 2) return;

  input_override[port] = device_to_type(device);

  if(stella.isSystemReady())
  {
    const auto autoType = (port == 0) ? stella.getLeftControllerType()
                                      : stella.getRightControllerType();
    input_type[port] = (input_override[port] != Controller::Type::Unknown)
                       ? input_override[port] : autoType;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void retro_set_environment(retro_environment_t cb)
{
  environ_cb = cb;

  // Request VFS v3 interface (stat/mkdir/opendir support)
  struct retro_vfs_interface_info vfs_info = { 3, nullptr };
  if(environ_cb(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, &vfs_info) && vfs_info.iface)
    libretro_vfs = vfs_info.iface;

  static struct retro_core_option_v2_category option_categories[] = {
    { "system", "System", NULL },
    { "video", "Video", NULL },
    { "audio", "Audio", NULL },
    { "input", "Input", NULL },
    { NULL, NULL, NULL },
  };

  static struct retro_core_option_v2_definition option_defs[] = {
    {
      "stella_console",
      "Console display",
      NULL,
      NULL,
      NULL,
      "video",
      {
        { "auto", NULL },
        { "ntsc", NULL },
        { "pal", NULL },
        { "secam", NULL },
        { "ntsc50", NULL },
        { "pal60", NULL },
        { "secam60", NULL },
        { NULL, NULL },
      },
      "auto",
    },
    {
      "stella_palette",
      "Palette colors",
      NULL,
      NULL,
      NULL,
      "video",
      {
        { "standard", NULL },
        { "z26", NULL },
        { "user", NULL },
        { "custom", NULL },
        { NULL, NULL },
      },
      "standard",
    },
    {
      "stella_filter",
      "TV effects",
      NULL,
      NULL,
      NULL,
      "video",
      {
        { "disabled", NULL },
        { "composite", NULL },
        { "s-video", NULL },
        { "rgb", NULL },
        { "badly adjusted", NULL },
        { NULL, NULL },
      },
      "disabled",
    },
    {
      "stella_crop_hoverscan",
      "Crop horizontal overscan",
      NULL,
      NULL,
      NULL,
      "video",
      {
        { "enabled", NULL },
        { "disabled", NULL },
        { NULL, NULL },
      },
      "disabled",
    },
    {
      "stella_crop_voverscan",
      "Crop vertical overscan",
      NULL,
      NULL,
      NULL,
      "video",
      {
        { "0", NULL },
        { "1", NULL },
        { "2", NULL },
        { "3", NULL },
        { "4", NULL },
        { "5", NULL },
        { "6", NULL },
        { "7", NULL },
        { "8", NULL },
        { "9", NULL },
        { "10", NULL },
        { "11", NULL },
        { "12", NULL },
        { "13", NULL },
        { "14", NULL },
        { "15", NULL },
        { "16", NULL },
        { "17", NULL },
        { "18", NULL },
        { "19", NULL },
        { "20", NULL },
        { "21", NULL },
        { "22", NULL },
        { "23", NULL },
        { "24", NULL },
        { NULL, NULL },
      },
      "0",
    },
    {
      "stella_ntsc_aspect",
      "NTSC aspect %",
      NULL,
      NULL,
      NULL,
      "video",
      {
        { "par", NULL },
        { "75", NULL },
        { "76", NULL },
        { "77", NULL },
        { "78", NULL },
        { "79", NULL },
        { "80", NULL },
        { "81", NULL },
        { "82", NULL },
        { "83", NULL },
        { "84", NULL },
        { "85", NULL },
        { "86", NULL },
        { "87", NULL },
        { "88", NULL },
        { "89", NULL },
        { "90", NULL },
        { "91", NULL },
        { "92", NULL },
        { "93", NULL },
        { "94", NULL },
        { "95", NULL },
        { "96", NULL },
        { "97", NULL },
        { "98", NULL },
        { "99", NULL },
        { "100", NULL },
        { "101", NULL },
        { "102", NULL },
        { "103", NULL },
        { "104", NULL },
        { "105", NULL },
        { "106", NULL },
        { "107", NULL },
        { "108", NULL },
        { "109", NULL },
        { "110", NULL },
        { "111", NULL },
        { "112", NULL },
        { "113", NULL },
        { "114", NULL },
        { "115", NULL },
        { "116", NULL },
        { "117", NULL },
        { "118", NULL },
        { "119", NULL },
        { "120", NULL },
        { "121", NULL },
        { "122", NULL },
        { "123", NULL },
        { "124", NULL },
        { "125", NULL },
        { NULL, NULL },
      },
      "par",
    },
    {
      "stella_pal_aspect",
      "PAL aspect %",
      NULL,
      NULL,
      NULL,
      "video",
      {
        { "par", NULL },
        { "75", NULL },
        { "76", NULL },
        { "77", NULL },
        { "78", NULL },
        { "79", NULL },
        { "80", NULL },
        { "81", NULL },
        { "82", NULL },
        { "83", NULL },
        { "84", NULL },
        { "85", NULL },
        { "86", NULL },
        { "87", NULL },
        { "88", NULL },
        { "89", NULL },
        { "90", NULL },
        { "91", NULL },
        { "92", NULL },
        { "93", NULL },
        { "94", NULL },
        { "95", NULL },
        { "96", NULL },
        { "97", NULL },
        { "98", NULL },
        { "99", NULL },
        { "100", NULL },
        { "101", NULL },
        { "102", NULL },
        { "103", NULL },
        { "104", NULL },
        { "105", NULL },
        { "106", NULL },
        { "107", NULL },
        { "108", NULL },
        { "109", NULL },
        { "110", NULL },
        { "111", NULL },
        { "112", NULL },
        { "113", NULL },
        { "114", NULL },
        { "115", NULL },
        { "116", NULL },
        { "117", NULL },
        { "118", NULL },
        { "119", NULL },
        { "120", NULL },
        { "121", NULL },
        { "122", NULL },
        { "123", NULL },
        { "124", NULL },
        { "125", NULL },
        { NULL, NULL },
      },
      "par",
    },
    {
      "stella_stereo",
      "Stereo sound",
      NULL,
      NULL,
      NULL,
      "audio",
      {
        { "auto", NULL },
        { "on", NULL },
        { "off", NULL },
        { NULL, NULL },
      },
      "auto",
    },
    {
      "stella_phosphor",
      "Phosphor mode",
      NULL,
      NULL,
      NULL,
      "video",
      {
        { "auto", NULL },
        { "on", NULL },
        { "off", NULL },
        { NULL, NULL },
      },
      "auto",
    },
    {
      "stella_phosphor_blend",
      "Phosphor blend %",
      NULL,
      NULL,
      NULL,
      "video",
      {
        { "0", NULL },
        { "5", NULL },
        { "10", NULL },
        { "15", NULL },
        { "20", NULL },
        { "25", NULL },
        { "30", NULL },
        { "35", NULL },
        { "40", NULL },
        { "45", NULL },
        { "50", NULL },
        { "55", NULL },
        { "60", NULL },
        { "65", NULL },
        { "70", NULL },
        { "75", NULL },
        { "80", NULL },
        { "85", NULL },
        { "90", NULL },
        { "95", NULL },
        { "100", NULL },
        { NULL, NULL },
      },
      "60",
    },
    {
      "stella_paddle_mouse_sensitivity",
      "Paddle mouse sensitivity",
      NULL,
      NULL,
      NULL,
      "input",
      {
        { "1", NULL },
        { "2", NULL },
        { "3", NULL },
        { "4", NULL },
        { "5", NULL },
        { "6", NULL },
        { "7", NULL },
        { "8", NULL },
        { "9", NULL },
        { "10", NULL },
        { "11", NULL },
        { "12", NULL },
        { "13", NULL },
        { "14", NULL },
        { "15", NULL },
        { "16", NULL },
        { "17", NULL },
        { "18", NULL },
        { "19", NULL },
        { "20", NULL },
        { "21", NULL },
        { "22", NULL },
        { "23", NULL },
        { "24", NULL },
        { "25", NULL },
        { "26", NULL },
        { "27", NULL },
        { "28", NULL },
        { "29", NULL },
        { "30", NULL },
        { NULL, NULL },
      },
      "10",
    },
    {
      "stella_paddle_joypad_sensitivity",
      "Paddle joypad sensitivity",
      NULL,
      NULL,
      NULL,
      "input",
      {
        { "1", NULL },
        { "2", NULL },
        { "3", NULL },
        { "4", NULL },
        { "5", NULL },
        { "6", NULL },
        { "7", NULL },
        { "8", NULL },
        { "9", NULL },
        { "10", NULL },
        { "11", NULL },
        { "12", NULL },
        { "13", NULL },
        { "14", NULL },
        { "15", NULL },
        { "16", NULL },
        { "17", NULL },
        { "18", NULL },
        { "19", NULL },
        { "20", NULL },
        { NULL, NULL },
      },
      "3",
    },
    {
      "stella_paddle_analog_sensitivity",
      "Paddle analog sensitivity",
      NULL,
      NULL,
      NULL,
      "input",
      {
        { "0", NULL },
        { "1", NULL },
        { "2", NULL },
        { "3", NULL },
        { "4", NULL },
        { "5", NULL },
        { "6", NULL },
        { "7", NULL },
        { "8", NULL },
        { "9", NULL },
        { "10", NULL },
        { "11", NULL },
        { "12", NULL },
        { "13", NULL },
        { "14", NULL },
        { "15", NULL },
        { "16", NULL },
        { "17", NULL },
        { "18", NULL },
        { "19", NULL },
        { "20", NULL },
        { "21", NULL },
        { "22", NULL },
        { "23", NULL },
        { "24", NULL },
        { "25", NULL },
        { "26", NULL },
        { "27", NULL },
        { "28", NULL },
        { "29", NULL },
        { "30", NULL },
        { NULL, NULL },
      },
      "20",
    },
    {
      "stella_paddle_analog_deadzone",
      "Paddle analog deadzone",
      NULL,
      NULL,
      NULL,
      "input",
      {
        { "0", NULL },
        { "1", NULL },
        { "2", NULL },
        { "3", NULL },
        { "4", NULL },
        { "5", NULL },
        { "6", NULL },
        { "7", NULL },
        { "8", NULL },
        { "9", NULL },
        { "10", NULL },
        { "11", NULL },
        { "12", NULL },
        { "13", NULL },
        { "14", NULL },
        { "15", NULL },
        { "16", NULL },
        { "17", NULL },
        { "18", NULL },
        { "19", NULL },
        { "20", NULL },
        { "21", NULL },
        { "22", NULL },
        { "23", NULL },
        { "24", NULL },
        { "25", NULL },
        { "26", NULL },
        { "27", NULL },
        { "28", NULL },
        { "29", NULL },
        { "30", NULL },
        { NULL, NULL },
      },
      "15",
    },
    {
      "stella_paddle_analog_absolute",
      "Paddle analog absolute",
      NULL,
      NULL,
      NULL,
      "input",
      {
        { "enabled", NULL },
        { "disabled", NULL },
        { NULL, NULL },
      },
      "disabled",
    },
    {
      "stella_lightgun_crosshair",
      "Lightgun crosshair",
      NULL,
      NULL,
      NULL,
      "input",
      {
        { "enabled", NULL },
        { "disabled", NULL },
        { NULL, NULL },
      },
      "disabled",
    },
    {
      "stella_reload",
      "Enable reload/next game",
      NULL,
      NULL,
      NULL,
      "system",
      {
        { "on", NULL },
        { "off", NULL },
        { NULL, NULL },
      },
      "off",
    },
    {
      "stella_detect_pal60",
      "Auto-detect PAL-60",
      NULL, NULL, NULL,
      "video",
      {
        { "disabled", NULL },
        { "enabled",  NULL },
        { NULL, NULL },
      },
      "disabled",
    },
    {
      "stella_detect_ntsc50",
      "Auto-detect NTSC-50",
      NULL, NULL, NULL,
      "video",
      {
        { "disabled", NULL },
        { "enabled",  NULL },
        { NULL, NULL },
      },
      "disabled",
    },
    {
      "stella_pal_contrast",
      "Palette contrast",
      NULL, NULL, NULL,
      "video",
      {
        { "0", NULL },
        { "-10", NULL }, { "-9", NULL }, { "-8", NULL }, { "-7", NULL }, { "-6", NULL },
        { "-5", NULL }, { "-4", NULL }, { "-3", NULL }, { "-2", NULL }, { "-1", NULL },
        { "1", NULL }, { "2", NULL }, { "3", NULL }, { "4", NULL }, { "5", NULL },
        { "6", NULL }, { "7", NULL }, { "8", NULL }, { "9", NULL }, { "10", NULL },
        { NULL, NULL },
      },
      "0",
    },
    {
      "stella_pal_brightness",
      "Palette brightness",
      NULL, NULL, NULL,
      "video",
      {
        { "0", NULL },
        { "-10", NULL }, { "-9", NULL }, { "-8", NULL }, { "-7", NULL }, { "-6", NULL },
        { "-5", NULL }, { "-4", NULL }, { "-3", NULL }, { "-2", NULL }, { "-1", NULL },
        { "1", NULL }, { "2", NULL }, { "3", NULL }, { "4", NULL }, { "5", NULL },
        { "6", NULL }, { "7", NULL }, { "8", NULL }, { "9", NULL }, { "10", NULL },
        { NULL, NULL },
      },
      "0",
    },
    {
      "stella_pal_hue",
      "Palette hue",
      NULL, NULL, NULL,
      "video",
      {
        { "0", NULL },
        { "-10", NULL }, { "-9", NULL }, { "-8", NULL }, { "-7", NULL }, { "-6", NULL },
        { "-5", NULL }, { "-4", NULL }, { "-3", NULL }, { "-2", NULL }, { "-1", NULL },
        { "1", NULL }, { "2", NULL }, { "3", NULL }, { "4", NULL }, { "5", NULL },
        { "6", NULL }, { "7", NULL }, { "8", NULL }, { "9", NULL }, { "10", NULL },
        { NULL, NULL },
      },
      "0",
    },
    {
      "stella_pal_saturation",
      "Palette saturation",
      NULL, NULL, NULL,
      "video",
      {
        { "0", NULL },
        { "-10", NULL }, { "-9", NULL }, { "-8", NULL }, { "-7", NULL }, { "-6", NULL },
        { "-5", NULL }, { "-4", NULL }, { "-3", NULL }, { "-2", NULL }, { "-1", NULL },
        { "1", NULL }, { "2", NULL }, { "3", NULL }, { "4", NULL }, { "5", NULL },
        { "6", NULL }, { "7", NULL }, { "8", NULL }, { "9", NULL }, { "10", NULL },
        { NULL, NULL },
      },
      "0",
    },
    {
      "stella_pal_gamma",
      "Palette gamma",
      NULL, NULL, NULL,
      "video",
      {
        { "0", NULL },
        { "-10", NULL }, { "-9", NULL }, { "-8", NULL }, { "-7", NULL }, { "-6", NULL },
        { "-5", NULL }, { "-4", NULL }, { "-3", NULL }, { "-2", NULL }, { "-1", NULL },
        { "1", NULL }, { "2", NULL }, { "3", NULL }, { "4", NULL }, { "5", NULL },
        { "6", NULL }, { "7", NULL }, { "8", NULL }, { "9", NULL }, { "10", NULL },
        { NULL, NULL },
      },
      "0",
    },
    {
      "stella_dpc_pitch",
      "Pitfall II music pitch",
      NULL, NULL, NULL,
      "audio",
      {
        { "20000", NULL }, { "10000", NULL }, { "11000", NULL }, { "12000", NULL },
        { "13000", NULL }, { "14000", NULL }, { "15000", NULL }, { "16000", NULL },
        { "17000", NULL }, { "18000", NULL }, { "19000", NULL }, { "21000", NULL },
        { "22000", NULL }, { "23000", NULL }, { "24000", NULL }, { "25000", NULL },
        { "26000", NULL }, { "27000", NULL }, { "28000", NULL }, { "29000", NULL },
        { "30000", NULL },
        { NULL, NULL },
      },
      "20000",
    },
    {
      "stella_messages",
      "Info messages",
      NULL,
      NULL,
      NULL,
      "system",
      {
        { "disabled", NULL },
        { "enabled", NULL },
        { NULL, NULL },
      },
      "disabled",
    },
    { NULL, NULL, NULL, NULL, NULL, NULL, { { NULL, NULL } }, NULL },
  };

  static struct retro_core_options_v2 core_options = {
    option_categories,
    option_defs
  };

  static struct retro_variable variables[] = {
    // Adding more variables and rearranging them is safe.
    { "stella_console", "Console display; auto|ntsc|pal|secam|ntsc50|pal60|secam60" },
    { "stella_palette", "Palette colors; standard|z26|user|custom" },
    { "stella_filter", "TV effects; disabled|composite|s-video|rgb|badly adjusted" },
    { "stella_crop_hoverscan", "Crop horizontal overscan; disabled|enabled" },
    { "stella_crop_voverscan", "Crop vertical overscan; 0|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17|18|19|20|21|22|23|24" },
    { "stella_ntsc_aspect", "NTSC aspect %; par|100|101|102|103|104|105|106|107|108|109|110|111|112|113|114|115|116|117|118|119|120|121|122|123|124|125|75|76|77|78|79|80|81|82|83|84|85|86|87|88|89|90|91|92|93|94|95|96|97|98|99" },
    { "stella_pal_aspect", "PAL aspect %; par|100|101|102|103|104|105|106|107|108|109|110|111|112|113|114|115|116|117|118|119|120|121|122|123|124|125|75|76|77|78|79|80|81|82|83|84|85|86|87|88|89|90|91|92|93|94|95|96|97|98|99" },
    { "stella_stereo", "Stereo sound; auto|off|on" },
    { "stella_phosphor", "Phosphor mode; auto|off|on" },
    { "stella_phosphor_blend", "Phosphor blend %; 60|65|70|75|80|85|90|95|100|0|5|10|15|20|25|30|35|40|45|50|55" },
    { "stella_paddle_mouse_sensitivity", "Paddle mouse sensitivity; 10|11|12|13|14|15|16|17|18|19|20|21|22|23|24|25|26|27|28|29|30|1|2|3|4|5|6|7|8|9" },
    { "stella_paddle_joypad_sensitivity", "Paddle joypad sensitivity; 3|4|5|6|7|8|9|10|11|12|13|14|15|16|17|18|19|20|1|2" },
    { "stella_paddle_analog_sensitivity", "Paddle analog sensitivity; 20|21|22|23|24|25|26|27|28|29|30|0|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17|18|19" },
    { "stella_paddle_analog_deadzone", "Paddle analog deadzone; 15|16|17|18|19|20|21|22|23|24|25|26|27|28|29|30|0|1|2|3|4|5|6|7|8|9|10|11|12|13|14" },
    { "stella_paddle_analog_absolute", "Paddle analog absolute; disabled|enabled" },
    { "stella_lightgun_crosshair", "Lightgun crosshair; disabled|enabled" },
    { "stella_reload", "Enable reload/next game; off|on" },
    { "stella_detect_pal60",    "Auto-detect PAL-60; disabled|enabled" },
    { "stella_detect_ntsc50",   "Auto-detect NTSC-50; disabled|enabled" },
    { "stella_pal_contrast",    "Palette contrast; 0|-1|-2|-3|-4|-5|-6|-7|-8|-9|-10|1|2|3|4|5|6|7|8|9|10" },
    { "stella_pal_brightness",  "Palette brightness; 0|-1|-2|-3|-4|-5|-6|-7|-8|-9|-10|1|2|3|4|5|6|7|8|9|10" },
    { "stella_pal_hue",         "Palette hue; 0|-1|-2|-3|-4|-5|-6|-7|-8|-9|-10|1|2|3|4|5|6|7|8|9|10" },
    { "stella_pal_saturation",  "Palette saturation; 0|-1|-2|-3|-4|-5|-6|-7|-8|-9|-10|1|2|3|4|5|6|7|8|9|10" },
    { "stella_pal_gamma",       "Palette gamma; 0|-1|-2|-3|-4|-5|-6|-7|-8|-9|-10|1|2|3|4|5|6|7|8|9|10" },
    { "stella_dpc_pitch",       "Pitfall II music pitch; 20000|10000|11000|12000|13000|14000|15000|16000|17000|18000|19000|21000|22000|23000|24000|25000|26000|27000|28000|29000|30000" },
    { "stella_messages",        "Info messages; disabled|enabled" },
    { NULL, NULL },
  };

  unsigned options_version = 0;
  if(environ_cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &options_version) && options_version >= 2)
    environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, &core_options);
  else
    environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void fallback_log(enum retro_log_level level, const char *fmt, ...)
{
  (void)level;
  va_list va;
  va_start(va, fmt);
  vfprintf(stderr, fmt, va);
  va_end(va);
}

void retro_init()
{
  struct retro_log_callback log;
  unsigned level = 4;

  log_cb = fallback_log;
  if(environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
    log_cb = log.log;

  environ_cb(RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL, &level);
  libretro_supports_bitmasks = environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL);
  environ_cb(RETRO_ENVIRONMENT_GET_MESSAGE_INTERFACE_VERSION, &message_interface_version);

  uint64_t quirks = RETRO_SERIALIZATION_QUIRK_CORE_VARIABLE_SIZE;
  environ_cb(RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS, &quirks);

  struct retro_keyboard_callback kb_cb = { keyboard_event_cb };
  environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &kb_cb);

  // Get save directory for nvram, palette, and other persistent files
  const char* save_dir_c = nullptr;
  if(environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &save_dir_c) && save_dir_c)
  {
    libretro_save_dir = save_dir_c;
    if(!libretro_save_dir.empty() && libretro_save_dir.back() != FSNode::PATH_SEPARATOR)
      libretro_save_dir += FSNode::PATH_SEPARATOR;
    if(log_cb) log_cb(RETRO_LOG_INFO, "Stella save directory: %s\n", libretro_save_dir.c_str());
  }

  // Get system directory for supplemental content (KidVid WAV files, etc.)
  const char* system_dir_c = nullptr;
  if(environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &system_dir_c) && system_dir_c)
  {
    libretro_system_dir = system_dir_c;
    if(!libretro_system_dir.empty() && libretro_system_dir.back() != FSNode::PATH_SEPARATOR)
      libretro_system_dir += FSNode::PATH_SEPARATOR;
    if(log_cb) log_cb(RETRO_LOG_INFO, "Stella system directory: %s\n", libretro_system_dir.c_str());
  }
}

static const struct retro_controller_description controllers[] = {
    { "Automatic (from ROM database)", RETRO_DEVICE_JOYPAD },
    { "Joystick",    STELLA_DEVICE_JOYSTICK },
    { "BoosterGrip", STELLA_DEVICE_BOOSTERGRIP },
    { "Genesis",     STELLA_DEVICE_GENESIS },
    { "Joy 2B+",     STELLA_DEVICE_JOY2BPLUS },
    { "Paddles",     STELLA_DEVICE_PADDLES },
    { "Driving",     STELLA_DEVICE_DRIVING },
    { "Keyboard",    STELLA_DEVICE_KEYBOARD },
    { "TrakBall",    STELLA_DEVICE_TRAKBALL },
    { "Amiga Mouse", STELLA_DEVICE_AMIGAMOUSE },
    { "Atari Mouse", STELLA_DEVICE_ATARIMOUSE },
    { "Lightgun",    STELLA_DEVICE_LIGHTGUN },
    { "QuadTari",    STELLA_DEVICE_QUADTARI },
    { "MindLink",    STELLA_DEVICE_MINDLINK },
    { "AtariVox",    STELLA_DEVICE_ATARIVOX },
    { "SaveKey",     STELLA_DEVICE_SAVEKEY },
    { "KidVid",      STELLA_DEVICE_KIDVID },
    { "None",        RETRO_DEVICE_NONE },
    { NULL, 0 }
};

static const struct retro_controller_description simple_controllers[] = {
    { "Automatic", RETRO_DEVICE_JOYPAD },
    { "None",      RETRO_DEVICE_NONE },
    { NULL, 0 }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool retro_load_game(const struct retro_game_info *info)
{
  enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;

  static const struct retro_controller_info controller_info[] = {
    { controllers,        sizeof(controllers)        / sizeof(controllers[0]) },
    { controllers,        sizeof(controllers)        / sizeof(controllers[0]) },
    { simple_controllers, sizeof(simple_controllers) / sizeof(simple_controllers[0]) },
    { simple_controllers, sizeof(simple_controllers) / sizeof(simple_controllers[0]) },
    { NULL, 0 }
  };

  #define RETRO_DESCRIPTOR_BLOCK(_user) \
  { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Up" }, \
  { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Down" }, \
  { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Left" }, \
  { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Right" }, \
  { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "Fire" }, \
  { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "Trigger" }, \
  { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Booster" }, \
  { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" }, \
  { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Reset" }, \
  { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "Left Difficulty A" }, \
  { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "Right Difficulty A" }, \
  { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,     "Left Difficulty B" }, \
  { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,     "Right Difficulty B" }, \
  { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3,     "Color" }, \
  { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3,     "Black/White" }, \
  { _user, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,   RETRO_DEVICE_ID_ANALOG_X, "Axis" } \

  #define RETRO_DESCRIPTOR_EXTRA_BLOCK(_user) \
  { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Left" }, \
  { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Right" }, \
  { _user, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "Fire" }, \
  { _user, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,   RETRO_DEVICE_ID_ANALOG_X, "Axis" } \

  static struct retro_input_descriptor input_descriptors[] =
  {
    RETRO_DESCRIPTOR_BLOCK(0),
    RETRO_DESCRIPTOR_BLOCK(1),
    RETRO_DESCRIPTOR_EXTRA_BLOCK(2),
    RETRO_DESCRIPTOR_EXTRA_BLOCK(3),
    {0, 0, 0, 0, NULL},
  };
  #undef RETRO_DESCRIPTOR_BLOCK
  #undef RETRO_DESCRIPTOR_EXTRA_BLOCK

  if(!info || info->size > stella.getROMMax()) return false;

  // Send controller infos to libretro
  environ_cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)controller_info);
  // Send controller input descriptions to libretro
  environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, (void*)input_descriptors);

  if(!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
  {
    if(log_cb) log_cb(RETRO_LOG_INFO, "XRGB8888 is not supported.\n");
    return false;
  }

  stella.setROM(info->path, info->data, info->size);
  libretro_rom_path = info->path ? info->path : "";

  bool supports_achievements = true;
  environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS, &supports_achievements);

  if(!reset_system())
  {
    post_message("Stella: failed to initialize system", RETRO_LOG_ERROR, 5000);
    return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info)
{
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void retro_reset()
{
  stella.reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void retro_run()
{
  bool updated = false;

  if(environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
    update_variables();

  if(system_reset)
  {
    reset_system();
    update_system_av();
    return;
  }

  update_input();

  stella.runFrame();

  if(stella_settings.lightgun_crosshair && input_crosshair[0] && input_crosshair[1])
    draw_crosshair(input_crosshair[0], input_crosshair[1], 0x0000ff);

  if(stella.getVideoResize())
    update_geometry();

  if(stella.getVideoReady())
    video_cb(reinterpret_cast<uInt32*>(stella.getVideoBuffer()) + crop_left + (crop_top * stella.getVideoWidthMax()),
        stella.getVideoWidth() - crop_left,
        stella.getVideoHeight() - crop_top * 2,
        stella.getVideoPitch());

  bool fast_forward = false;
  environ_cb(RETRO_ENVIRONMENT_GET_FASTFORWARDING, &fast_forward);

  if(stella.getAudioReady() && !fast_forward)
    audio_batch_cb(stella.getAudioBuffer(), stella.getAudioSize());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void retro_unload_game()
{
  stella.destroy();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void retro_deinit()
{
  stella.destroy();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t retro_serialize_size()
{
  int runahead = -1;
  if(environ_cb(RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE, &runahead))
  {
    // maximum state size possible
    if(runahead & 4)
      return 0x100000;
  }

  return stella.getStateSize();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool retro_serialize(void *data, size_t size)
{
  return stella.saveState(data, size);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool retro_unserialize(const void *data, size_t size)
{
  return stella.loadState(data, size);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void *retro_get_memory_data(unsigned id)
{
  switch (id)
  {
    case RETRO_MEMORY_SYSTEM_RAM:
      return stella.getRAM();

    default:
      return NULL;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t retro_get_memory_size(unsigned id)
{
  switch (id)
  {
    case RETRO_MEMORY_SYSTEM_RAM:
      return stella.getRAMSize();

    default:
      return 0;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void retro_cheat_reset()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
}

// NOLINTEND
