// NOLINTBEGIN (misc-use-anonymous-namespace)

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
#include "PaletteHandler.hxx"
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

// libretro UI settings
static int setting_ntsc, setting_pal;
static int setting_stereo;
static int setting_phosphor, setting_console, setting_phosphor_blend;
static int stella_paddle_joypad_sensitivity;
static int stella_paddle_analog_sensitivity;
static int stella_paddle_mouse_sensitivity;
static int stella_paddle_analog_deadzone;
static bool stella_paddle_analog_absolute;
static bool stella_lightgun_crosshair;
static int setting_crop_hoverscan, crop_left;
static int setting_crop_voverscan, crop_top;
static NTSCFilter::Preset setting_filter;
static const char* setting_palette;
static int setting_reload;

static bool system_reset;

static unsigned input_devices[4];
static int32_t input_crosshair[2];
static Controller::Type input_type[2];

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
  if (stella_paddle_analog_deadzone && abs(analog_x) < stella_paddle_analog_deadzone * 0x7fff / 100) \
    analog_x = 0; \
  if (mouse_r) \
    mouse_x *= 3; \

static void retro_analog_paddle(unsigned pad, int32_t *analog_axis, int32_t *input_bitmask)
{
  RETRO_ANALOG_COMMON();

  if (mouse_x)
    *analog_axis += mouse_x * stella_paddle_mouse_sensitivity;
  else if (!stella_paddle_analog_absolute)
    *analog_axis += analog_x / 50;
  else
    *analog_axis  = analog_x;
  *analog_axis = BSPF::clamp(*analog_axis, -0x7fff, 0x7fff);
}

static void retro_analog_wheel(unsigned pad, int32_t *analog_axis, int32_t *input_bitmask)
{
  RETRO_ANALOG_COMMON();

  if (mouse_x)
    *analog_axis = mouse_x * stella_paddle_mouse_sensitivity * 50;
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

      if (stella_paddle_analog_deadzone && analog_mag <= stella_paddle_analog_deadzone * 0x7fff / 100)
        analog_x = analog_y = 0;

      mouse_x += analog_x / (80000 / stella_paddle_analog_sensitivity);
      mouse_y += analog_y / (80000 / stella_paddle_analog_sensitivity);

      if (input_bitmask[pad] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT))
        mouse_x -= stella_paddle_joypad_sensitivity;
      else if (input_bitmask[pad] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT))
        mouse_x += stella_paddle_joypad_sensitivity;

      if (input_bitmask[pad] & (1 << RETRO_DEVICE_ID_JOYPAD_UP))
        mouse_y -= stella_paddle_joypad_sensitivity;
      else if (input_bitmask[pad] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN))
        mouse_y += stella_paddle_joypad_sensitivity;

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
  if (setting_reload)
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

    if(setting_filter != value)
    {
      stella.setVideoFilter(value);

      geometry_update = true;
      setting_filter = value;
    }
  }

  RETRO_GET("stella_crop_hoverscan")
  {
    setting_crop_hoverscan = !strcmp(var.value, "enabled");

    geometry_update = true;
  }

    RETRO_GET("stella_crop_voverscan")
  {
    setting_crop_voverscan = atoi(var.value);

    geometry_update = true;
  }

  RETRO_GET("stella_ntsc_aspect")
  {
    int value = 0;

    if(!strcmp(var.value, "par")) value = 0;
    else value = atoi(var.value);

    if(setting_ntsc != value)
    {
      stella.setVideoAspectNTSC(value);

      geometry_update = true;
      setting_ntsc = value;
    }
  }

  RETRO_GET("stella_pal_aspect")
  {
    int value = 0;

    if(!strcmp(var.value, "par")) value = 0;
    else value = atoi(var.value);

    if(setting_pal != value)
    {
      stella.setVideoAspectPAL(value);

      setting_pal = value;
      geometry_update = true;
    }
  }

  RETRO_GET("stella_palette")
  {
    if(setting_palette != var.value)
    {
      stella.setVideoPalette(var.value);

      setting_palette = var.value;
    }
  }

  RETRO_GET("stella_console")
  {
    int value = 0;

    if(!strcmp(var.value, "auto")) value = 0;
    else if(!strcmp(var.value, "ntsc")) value = 1;
    else if(!strcmp(var.value, "pal")) value = 2;
    else if(!strcmp(var.value, "secam")) value = 3;
    else if(!strcmp(var.value, "ntsc50")) value = 4;
    else if(!strcmp(var.value, "pal60")) value = 5;
    else if(!strcmp(var.value, "secam60")) value = 6;

    if(setting_console != value)
    {
      stella.setConsoleFormat(value);

      setting_console = value;
      system_reset = true;
    }
  }

  RETRO_GET("stella_stereo")
  {
    int value = 0;

    if(!strcmp(var.value, "auto")) value = 0;
    else if(!strcmp(var.value, "off")) value = 1;
    else if(!strcmp(var.value, "on")) value = 2;

    if(setting_stereo != value)
    {
      stella.setAudioStereo(value);

      setting_stereo = value;
    }
  }

  RETRO_GET("stella_phosphor")
  {
    int value = 0;

    if(!strcmp(var.value, "auto")) value = 0;
    else if(!strcmp(var.value, "off")) value = 1;
    else if(!strcmp(var.value, "on")) value = 2;

    if(setting_phosphor != value)
    {
      stella.setVideoPhosphor(value, setting_phosphor_blend);

      setting_phosphor = value;
    }
  }

  RETRO_GET("stella_phosphor_blend")
  {
    int value = 0;

    value = atoi(var.value);

    if(setting_phosphor_blend != value)
    {
      stella.setVideoPhosphor(setting_phosphor, value);

      setting_phosphor_blend = value;
    }
  }

  RETRO_GET("stella_paddle_joypad_sensitivity")
  {
    int value = 0;

    value = atoi(var.value);

    if(stella_paddle_joypad_sensitivity != value)
    {
      if(!init) stella.setPaddleJoypadSensitivity(value);

      stella_paddle_joypad_sensitivity = value;
    }
  }

  RETRO_GET("stella_paddle_analog_sensitivity")
  {
    int value = 0;

    value = atoi(var.value);

    if(stella_paddle_analog_sensitivity != value)
    {
      if(!init) stella.setPaddleAnalogSensitivity(value);

      stella_paddle_analog_sensitivity = value;
    }
  }

  RETRO_GET("stella_reload")
  {
    int value = 0;

    if(!strcmp(var.value, "off")) value = 0;
    else if(!strcmp(var.value, "on")) value = 1;

    setting_reload = value;
  }

  RETRO_GET("stella_paddle_mouse_sensitivity")
  {
    stella_paddle_mouse_sensitivity = atoi(var.value);
  }

  RETRO_GET("stella_paddle_analog_deadzone")
  {
    stella_paddle_analog_deadzone = atoi(var.value);
  }

  RETRO_GET("stella_paddle_analog_absolute")
  {
    stella_paddle_analog_absolute = false;

    if(!strcmp(var.value, "enabled"))
      stella_paddle_analog_absolute = true;
  }

  RETRO_GET("stella_lightgun_crosshair")
  {
    stella_lightgun_crosshair = false;

    if(!strcmp(var.value, "enabled"))
      stella_lightgun_crosshair = true;
  }

  if(!init && !system_reset)
  {
    crop_left = setting_crop_hoverscan ? (stella.getVideoZoom() == 2 ? 32 : 8) : 0;
    crop_top  = setting_crop_voverscan;

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
  if(!stella.create(log_cb ? true : false)) return false;

  // get auto-detect controllers
  input_type[0] = stella.getLeftControllerType();
  input_type[1] = stella.getRightControllerType();
  stella.setPaddleJoypadSensitivity(stella_paddle_joypad_sensitivity);
  stella.setPaddleAnalogSensitivity(stella_paddle_analog_sensitivity);

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
  info->library_version = STELLA_VERSION GIT_VERSION;
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

  info->geometry.aspect_ratio = stella.getVideoAspectPar() *
      (float)(160 - crop_width) * 2 / (float)(stella.getVideoHeight() - (crop_top * 2));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void retro_set_controller_port_device(unsigned port, unsigned device)
{
  if(port < 4)
  {
    switch(device)
    {
      case RETRO_DEVICE_NONE:
      case RETRO_DEVICE_JOYPAD:
        input_devices[port] = device;
        break;

      default:
        input_devices[port] = RETRO_DEVICE_JOYPAD;
        break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void retro_set_environment(retro_environment_t cb)
{
  environ_cb = cb;

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
        { "disabled", NULL },
        { "enabled", NULL },
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
        { "off", NULL },
        { "on", NULL },
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
        { "off", NULL },
        { "on", NULL },
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
        { "60", NULL },
        { "65", NULL },
        { "70", NULL },
        { "75", NULL },
        { "80", NULL },
        { "85", NULL },
        { "90", NULL },
        { "95", NULL },
        { "100", NULL },
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
        { "1", NULL },
        { "2", NULL },
        { "3", NULL },
        { "4", NULL },
        { "5", NULL },
        { "6", NULL },
        { "7", NULL },
        { "8", NULL },
        { "9", NULL },
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
        { "1", NULL },
        { "2", NULL },
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
        { "disabled", NULL },
        { "enabled", NULL },
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
        { "disabled", NULL },
        { "enabled", NULL },
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
        { "off", NULL },
        { "on", NULL },
        { NULL, NULL },
      },
      "off",
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
}

static const struct retro_controller_description controllers[] = {
    { "Automatic", RETRO_DEVICE_JOYPAD },
    { "None", RETRO_DEVICE_NONE },
    { NULL, 0 }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool retro_load_game(const struct retro_game_info *info)
{
  enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;

  static const struct retro_controller_info controller_info[] = {
    { controllers, sizeof(controllers) / sizeof(controllers[0]) },
    { controllers, sizeof(controllers) / sizeof(controllers[0]) },
    { controllers, sizeof(controllers) / sizeof(controllers[0]) },
    { controllers, sizeof(controllers) / sizeof(controllers[0]) },
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

  return reset_system();
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

  if(stella_lightgun_crosshair && input_crosshair[0] && input_crosshair[1])
    draw_crosshair(input_crosshair[0], input_crosshair[1], 0x0000ff);

  if(stella.getVideoResize())
    update_geometry();

  if(stella.getVideoReady())
    video_cb(reinterpret_cast<uInt32*>(stella.getVideoBuffer()) + crop_left + (crop_top * stella.getVideoWidthMax()),
        stella.getVideoWidth() - crop_left,
        stella.getVideoHeight() - crop_top * 2,
        stella.getVideoPitch());

  if(stella.getAudioReady())
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
