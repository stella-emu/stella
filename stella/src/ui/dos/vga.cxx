//============================================================================
//
//   SSSS    tt          lll  lll       
//  SS  SS   tt           ll   ll        
//  SS     tttttt  eeee   ll   ll   aaaa 
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2003 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// This code is based on the vga256fb frame buffer device driver for
// Linux by Salvatore Sanfilippo <antirez@invece.org>.  The vga256fb
// code can be found at http://www.kyuzz.org/antirez/vga256fb.htm.  It
// was released under the GNU General Public License.
//
// $Id: vga.cxx,v 1.1 2003-02-17 05:17:42 bwmott Exp $
//============================================================================

#include <sys/farptr.h>
#include <dos.h>

#include "vga.hxx"

// Structure for holding VGA mode information and register settings
struct VgaModeInfo
{
  unsigned int xres;             // X resolution
  unsigned int yres;             // Y resolution
  bool chained;                  // Chained flag
  unsigned char crt[0x19];       // 24 CRT sub-registers
  unsigned char attrib[0x15];    // 21 attribute sub-registers
  unsigned char graphic[0x09];   // 9 graphic sub-registers
  unsigned char sequencer[0x05]; // 5 sequencer sub-registers
  unsigned char misc;            // misc register
};

// VGA mode information for 320x200x256 colors at 60Hz
static VgaModeInfo Vga320x200x60Hz = {
  xres:	320,
  yres:	200,
  chained: true,
  crt: {
      0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0x0A, 0x3E,
      0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0xC2, 0x84, 0x8F, 0x28, 0x40, 0x90, 0x08, 0xA3 },
  attrib: {
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
      0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
      0x41, 0x00, 0x0F, 0x00, 0x00, },
  graphic: {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F, 0xFF },
  sequencer: {
      0x03, 0x01, 0x0F, 0x00, 0x0E },
  misc:	0x63
};

// VGA mode information for 320x200x256 colors at 70Hz (standard BIOS 13h mode)
static VgaModeInfo Vga320x200x70Hz = {
  xres:	320,
  yres:	200,
  chained: true,
  crt: {
      0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
      0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x9C, 0x8E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3 },
  attrib: {
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
      0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
      0x41, 0x00, 0x0F, 0x00, 0x00 },
  graphic: {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F, 0xFF },
  sequencer: {
      0x03, 0x01, 0x0F, 0x00, 0x0E },
  misc: 0x63
};

// VGA mode information for 320x240x256 colors at 60Hz (square pixels)
static VgaModeInfo Vga320x240x60Hz = {
xres: 320,
yres: 240,
chained: false,
  crt: {
      0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0x0D, 0x3E,
      0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0xEA, 0xAC, 0xDF, 0x28, 0x00, 0xE7, 0x06, 0xE3 },
  attrib: {
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
      0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
      0x41, 0x00, 0x0F, 0x00, 0x00, },
  graphic: {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F, 0xFF },
  sequencer: {
      0x03, 0x01, 0x0F, 0x00, 0x06 },
  misc: 0xE3
};

#define V_CRT_INDEX 0x3D4    // CRT address (index) register
#define V_CRT_RW    0x3D5    // CRT data register
#define V_ISTAT1_R  0x3DA    // Input status register #1
#define V_FEATURE_W 0x3DA    // Feature control register, (write)
#define V_SEQ_INDEX 0x3C4    // Sequencer address (index) register
#define V_SEQ_RW    0x3C5    // Sequencer data register
#define V_GR_INDEX  0x3CE    // VGA address (index) register
#define V_GR_RW	    0x3CF    // VGA data register
#define V_MISC_R    0x3CC    // VGA misc register (read)
#define V_MISC_W    0x3C2    // VGA misc register (write)
#define V_ATTR_IW   0x3C0    // Attribute index and data register (write)
#define V_ATTR_R    0x3C1    // Attribute data register (read)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
static inline void put_sequence(int i, unsigned char b)
{
  outportb(V_SEQ_INDEX, i); 
  outportb(V_SEQ_RW, b); 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
static inline void put_graph(int i, unsigned char b)
{
  outportb(V_GR_INDEX, i);
  outportb(V_GR_RW, b);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
static inline void put_misc(unsigned char b)
{
  outportb(V_MISC_W, b);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
static inline void put_attr(int i, unsigned char b)
{
  // Warning: as you can see the 0x20 bit will be set to zero
  // so the video output will be disabled, if you want read or write
  // (since some VGA cards allows you to write without setting the PAS
  // bit) without put the video off OR the index with 0x20 */

  // reset the flip/flop 
  inportb(V_ISTAT1_R);

  // set the index 
  outportb(V_ATTR_IW, i);

  // write data
  outportb(V_ATTR_IW, b);

  // reset the flip/flop 
  inportb(V_ISTAT1_R);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
static inline void put_crt(int i, unsigned char b)
{
  outportb(V_CRT_INDEX, i);
  outportb(V_CRT_RW, b);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
static inline unsigned char get_crt(int i)
{
  outportb(V_CRT_INDEX, i);
  return inportb(V_CRT_RW);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
static void disable_video(void)
{
  // Get the current value
  volatile unsigned char t = inportb(V_ATTR_IW);

  // Reset the flip/flop
  inportb(V_ISTAT1_R);

  // Clear the PAS bit
  t &= 0xDF;

  // Set the port
  outportb(V_ATTR_IW, t);

  // Reset the flip/flop
  inportb(V_ISTAT1_R);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
static void enable_video(void)
{
  // Get the current value
  volatile unsigned char t = inportb(V_ATTR_IW);

  // Reset the flip/flop
  inportb(V_ISTAT1_R);

  // Set the PAS bit
  t |= 0x20;

  // set the port
  outportb(V_ATTR_IW, t);

  // Reset the flip/flop
  inportb(V_ISTAT1_R);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
static void unlock_crt_registers(void)
{
  volatile unsigned char aux = get_crt(0x11);
  aux &= 0x7f;
  put_crt(0x11, aux);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
static void load_regs(VgaModeInfo *regs)
{
  int j;

  disable_video();
  disable();

  // Set misc register
  put_misc(regs->misc);

  // Sequencer sync reset on
  put_sequence(0x00, 0x01);

  // Sequencer registers
  for(j = 0; j <= 0x04; j++)
  {
    put_sequence(j, regs->sequencer[j]);
  }

  // Sequencer reset off
  put_sequence(0x00, 0x03);

  unlock_crt_registers();
  // crt registers
  for(j = 0; j <= 0x18; j++)
  {
    put_crt(j, regs->crt[j]);
  }

  // Graphic registers
  for(j = 0; j <= 0x08; j++)
  {
    put_graph(j, regs->graphic[j]);
  }

  // Attrib registers
  for(j = 0; j <= 0x14; j++)
  {
    put_attr(j, regs->attrib[j]);
  }

  enable();
  enable_video();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
bool VgaSetMode(int mode)
{
  VgaModeInfo* info = 0;

  if(mode == VGA_320_200_60HZ)
  {
    info = &Vga320x200x60Hz;
  }
  else if(mode == VGA_320_200_70HZ)
  {
    info = &Vga320x200x70Hz;
  }
  else if(mode == VGA_320_240_60HZ)
  {
    info = &Vga320x240x60Hz;
  }
  else
  {
    assert(false);
  }

  load_regs(info);
  return info->chained;
}

