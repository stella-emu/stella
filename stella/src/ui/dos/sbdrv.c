/*****************************************************************************/
/*                                                                           */
/* Module:    SBDRV                                                          */
/* Purpose:   Sound Blaster DAC DMA Driver V1.3                              */
/* Author(s): Ron Fries, Neil Bradley and Bradford Mott                      */
/*                                                                           */
/* 02/20/97 - Initial Release                                                */
/*                                                                           */
/* 08/19/97 - V1.1 - Corrected problem with the auto-detect of older SB      */
/*            cards and problem with DSP shutdown which left the auto-init   */
/*            mode active.  Required creating a function to reset the DSP.   */
/*            Also, added checks on the BLASTER settings to verify they      */
/*            are possible values for either SB or SB compatibles.           */
/*            Added several helpful information/error messages.  These can   */
/*            be disabled by removing the SBDRV_SHOW_ERR definition.         */
/*                                                                           */
/* 12/24/97 - V1.2 - Added support for DJGPP (by Bradford Mott).             */
/*                                                                           */
/* 02/04/99 - V1.3 - Cleaned up DJGPP support, locking code and data.        */
/*            Fixed a bug with the reading of the BLASTER= environment       */
/*            variable, which caused a segfault if one was not set.  Alst    */
/*            added a timeout to dsp_out() and some other minor              */
/*            modifications (Matthew Conte)                                  */
/*                                                                           */
/*****************************************************************************/
/*                                                                           */
/*                 License Information and Copyright Notice                  */
/*                 ========================================                  */
/*                                                                           */
/* SBDrv is Copyright(c) 1997-1999 by Ron Fries, Neil Bradley and            */
/*       Bradford Mott                                                       */
/*                                                                           */
/* This library is free software; you can redistribute it and/or modify it   */
/* under the terms of version 2 of the GNU Library General Public License    */
/* as published by the Free Software Foundation.                             */
/*                                                                           */
/* This library is distributed in the hope that it will be useful, but       */
/* WITHOUT ANY WARRANTY; without even the implied warranty of                */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library */
/* General Public License for more details.                                  */
/* To obtain a copy of the GNU Library General Public License, write to the  */
/* Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.   */
/*                                                                           */
/* Any permitted reproduction of these routines, in whole or in part, must   */
/* bear this legend.                                                         */
/*                                                                           */
/*****************************************************************************/

#ifdef DJGPP
  #include <go32.h>
  #include <dpmi.h>
  #include <sys/movedata.h>

  /* Handy macros (care of Allegro) to lock code / data */
  #define  END_OF_FUNCTION(x)   static void x##_end(void) {}
  #define  LOCK_VARIABLE(x)     _go32_dpmi_lock_data((void*)&x,sizeof(x))
  #define  LOCK_FUNCTION(x)     _go32_dpmi_lock_code(x,(long)x##_end-(long)x)
#endif

#include <dos.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <conio.h>
#include <time.h>
#include <stdio.h>
#include "sbdrv.h"

#define DSP_RESET         0x06
#define DSP_READ          0x0a
#define DSP_WRITE         0x0c
#define DSP_ACK           0x0e

#define DMA_BASE          0x00
#define DMA_COUNT         0x01
#define DMA_MASK          0x0a
#define DMA_MODE          0x0b
#define DMA_FF            0x0c

#define MASTER_VOLUME     0x22
#define LINE_VOLUME       0x2e
#define FM_VOLUME         0x26

/* declare local global variables */
#ifdef DJGPP
  static int theDOSBufferSegment;
  static int theDOSBufferSelector;
#endif
static uint8  *Sb_buffer;
static uint16 Sb_buf_size = 200;
static uint16 Sb_offset;
static uint16 Playback_freq;
static uint8  Sb_init = 0;
static uint8  Count_low;
static uint8  Count_high;

static uint16 IOaddr = 0x220;
static uint16 Irq = 7;
static uint16 Dma = 1;
static uint8  DMAmode = AUTO_DMA;
/*static*/ uint8  DMAcount;
static void   (*FillBuffer)(uint8 *buf, uint16 buf_size);

#ifdef DJGPP
  static _go32_dpmi_seginfo OldIntVectInfo;
  static _go32_dpmi_seginfo NewIntVectInfo;
#else 
  static void   (__interrupt *OldIntVect)(void);
#endif

/* function prototypes */
static void  setNewIntVect (uint16 irq);
static void  setOldIntVect (uint16 irq);
static void  dsp_out (uint16 port, uint8 val);
static uint8 hextodec (char c);
static void  logErr (char *st);
static uint8 getBlasterEnv (void);
static uint8 dsp_in (uint16 port);


/*****************************************************************************/
/*                                                                           */
/* Module:  newIntVect                                                       */
/* Purpose: The interrupt vector to handle the DAC DMAC completed interrupt  */
/*          Sends the next buffer to the SB and re-fills the current buffer. */
/* Author:  Ron Fries                                                        */
/* Date:    January 1, 1997                                                  */
/*                                                                           */
/*****************************************************************************/
#ifdef DJGPP
static void newIntVect(void)
#else
static void interrupt newIntVect (void)
#endif
{
   uint16 addr;

   if (DMAmode == STANDARD_DMA)
   {
      /* restart standard DMA transfer */
      dsp_out (IOaddr + DSP_WRITE, 0x14);
      dsp_out (IOaddr + DSP_WRITE, Count_low);
      dsp_out (IOaddr + DSP_WRITE, Count_high);
   }

   DMAcount++;

   /* acknowledge the DSP interrupt */
   inp (IOaddr + DSP_ACK);

   /* determine the current playback position */
   addr  = inp (DMA_BASE + (Dma << 1));         /* get low byte ptr */
   addr |= inp (DMA_BASE + (Dma << 1)) << 8;    /* and high byte ptr */

   addr -= Sb_offset;  /* subtract the offset */

   /* if we're currently playing the first half of the buffer */
   if (addr < Sb_buf_size)
   {
      /* reload the second half of the buffer */
      FillBuffer(Sb_buffer + Sb_buf_size, Sb_buf_size);

#ifdef DJGPP
      /* Copy data to DOS memory buffer */
      dosmemput(Sb_buffer + Sb_buf_size, Sb_buf_size,
          (theDOSBufferSegment << 4) + Sb_buf_size);
#endif
   }
   else
   {
      /* else reload the first half of the buffer */
      FillBuffer(Sb_buffer, Sb_buf_size);

#ifdef DJGPP
      /* Copy data to DOS memory buffer */
      dosmemput(Sb_buffer, Sb_buf_size, theDOSBufferSegment << 4);
#endif
   }

   /* indicate end of interrupt  */
   outp (0x20, 0x20);

   if (Irq > 7)
   {
      outp (0xa0, 0x20);
   }
}
#ifdef DJGPP
END_OF_FUNCTION(newIntVect);
#endif

/*****************************************************************************/
/*                                                                           */
/* Module:  setNewIntVect                                                    */
/* Purpose: To set the specified interrupt vector to the sound output        */
/*          processing interrupt.                                            */
/* Author:  Ron Fries                                                        */
/* Date:    January 1, 1997                                                  */
/*                                                                           */
/*****************************************************************************/

static void setNewIntVect (uint16 irq)
{
#ifdef DJGPP
   /* Lock code / data */
   LOCK_VARIABLE(DMAmode);
   LOCK_VARIABLE(IOaddr);
   LOCK_VARIABLE(Count_low);
   LOCK_VARIABLE(DMAcount);
   LOCK_VARIABLE(Dma);
   LOCK_VARIABLE(Sb_offset);
   LOCK_VARIABLE(Sb_buf_size);
   LOCK_VARIABLE(theDOSBufferSegment);
   LOCK_FUNCTION(newIntVect);
#endif

   if (irq > 7)
   {
#ifdef DJGPP
      _go32_dpmi_get_protected_mode_interrupt_vector(irq + 0x68,
          &OldIntVectInfo);
      NewIntVectInfo.pm_selector = _my_cs();
      NewIntVectInfo.pm_offset = (int)newIntVect;
      _go32_dpmi_allocate_iret_wrapper(&NewIntVectInfo);
      _go32_dpmi_set_protected_mode_interrupt_vector(irq + 0x68,
          &NewIntVectInfo);      
#else
      OldIntVect = _dos_getvect (irq + 0x68);
      _dos_setvect (irq + 0x68, newIntVect);
#endif
   }
   else
   {
#ifdef DJGPP
      _go32_dpmi_get_protected_mode_interrupt_vector(irq + 0x08,
          &OldIntVectInfo);
      NewIntVectInfo.pm_selector = _my_cs();
      NewIntVectInfo.pm_offset = (int)newIntVect;
      _go32_dpmi_allocate_iret_wrapper(&NewIntVectInfo);
      _go32_dpmi_set_protected_mode_interrupt_vector(irq + 0x08,
          &NewIntVectInfo);      
#else
      OldIntVect = _dos_getvect (irq + 0x08);
      _dos_setvect (irq + 0x08, newIntVect);
#endif
   }
}


/*****************************************************************************/
/*                                                                           */
/* Module:  setOldIntVect                                                    */
/* Purpose: To restore the original vector                                   */
/* Author:  Ron Fries                                                        */
/* Date:    January 1, 1997                                                  */
/*                                                                           */
/*****************************************************************************/

static void setOldIntVect (uint16 irq)
{
   if (irq > 7)
   {
#ifdef DJGPP
     _go32_dpmi_set_protected_mode_interrupt_vector(irq + 0x68,
         &OldIntVectInfo);
     _go32_dpmi_free_iret_wrapper(&NewIntVectInfo);
#else
      _dos_setvect (irq + 0x68, OldIntVect);
#endif
   }
   else
   {
#ifdef DJGPP
     _go32_dpmi_set_protected_mode_interrupt_vector(irq + 0x08,
         &OldIntVectInfo);
     _go32_dpmi_free_iret_wrapper(&NewIntVectInfo);
#else
      _dos_setvect (irq + 0x08, OldIntVect);
#endif
   }
}


/*****************************************************************************/
/*                                                                           */
/* Module:  dsp_out                                                          */
/* Purpose: To send a byte to the SB's DSP                                   */
/* Author:  Ron Fries                                                        */
/* Date:    September 10, 1996                                               */
/*                                                                           */
/*****************************************************************************/

static void dsp_out(uint16 port, uint8 val)
{
  uint32 timeout = 60000; /* set timeout */

  /* wait for buffer to be free */
  while((timeout--) && (inp(IOaddr + DSP_WRITE) & 0x80))
  {
     /* do nothing */
  }

  /* transmit the next byte */
  outp(port,val);
}


/*****************************************************************************/
/*                                                                           */
/* Module:  dsp_in                                                           */
/* Purpose: To read a byte from the SB's DSP                                 */
/* Author:  Ron Fries                                                        */
/* Date:    January 26, 1997                                                 */
/*                                                                           */
/*****************************************************************************/

static uint8 dsp_in(uint16 port)
{
  uint16 x=10000;    /* set timeout */

  /* wait for buffer to be free */
  while(((inp(IOaddr + 0x0E) & 0x80) == 0) && (x>0))
  {
     /* decrement the timeout */
     x--;
  }

  if (x>0)
  {
     /* read the data byte */
     return(inp(port));
  }
  else
  {
     return (0);
  }
}


/*****************************************************************************/
/*                                                                           */
/* Module:  hextodec                                                         */
/* Purpose: Convert the input character to hex                               */
/* Author:  Ron Fries                                                        */
/* Date:    September 10, 1996                                               */
/*                                                                           */
/*****************************************************************************/

uint8 hextodec (char c)
{
   uint8 retval = 0;

   c = toupper (c);

   if ((c>='0') && (c<='9'))
   {
      retval = c - '0';
   }
   else if ((c>='A') && (c<='F'))
   {
      retval = c - 'A' + 10;
   }

   return (retval);
}


/*****************************************************************************/
/*                                                                           */
/* Module:  logErr                                                           */
/* Purpose: Displays an error message.                                       */
/* Author:  Ron Fries                                                        */
/* Date:    September 24, 1996                                               */
/*                                                                           */
/*****************************************************************************/

static void logErr (char *st)
{
#ifdef SBDRV_SHOW_ERR
   printf ("%s",st);
#endif
}

/*****************************************************************************/
/*                                                                           */
/* Module:  getBlasterEnv                                                    */
/* Purpose: Read the BLASTER environment variable and set the local globals  */
/* Author:  Ron Fries                                                        */
/* Date:    September 10, 1996                                               */
/*                                                                           */
/*****************************************************************************/

static uint8 getBlasterEnv (void)
{
   char *env;
   char *ptr;
   uint16 count = 0;

   env = getenv("BLASTER");

   /* if the environment variable exists */
   if (env)
   {
      strupr(env);

      /* search for the address setting */
      ptr = strchr(env, 'A');
      if (ptr)
      {
         /* if valid, read and convert the IO address */
         IOaddr = (hextodec (ptr[1]) << 8) +
                  (hextodec (ptr[2]) << 4) +
                  (hextodec (ptr[3]));

         /* verify the IO address is one of the possible SB settings */
         switch (IOaddr)
         {
            case 0x210:
            case 0x220:
            case 0x230:
            case 0x240:
            case 0x250:
            case 0x260:
            case 0x280:
            case 0x2A0:
            case 0x2C0:
            case 0x2E0:
               /* IO address OK so indicate one more valid item found */
               count++;
               break;

            default:
               logErr ("Invalid Sound Blaster I/O address specified.\n");
               logErr ("Possible values are:  ");
               logErr ("210, 220, 230, 240, 250, 260, 280, 2A0, 2C0, 2E0.\n");
         }
      }
      else
      {
         logErr ("Unable to read Sound Blaster I/O address.\n");
      }

      /* search for the IRQ setting */
      ptr = strchr(env, 'I');
      if (ptr)
      {
         /* if valid, read and convert the IRQ */
         /* if the IRQ has two digits */
         if ((ptr[1] == '1') && ((ptr[2] >= '0') && (ptr[2] <='5')))
         {
            /* then convert accordingly (using decimal) */
            Irq = hextodec (ptr[1]) * 10 + hextodec (ptr[2]);
         }
         else
         {
            /* else convert as a single hex digit */
            Irq = hextodec (ptr[1]);
         }

         /* verify the IRQ setting is one of the possible SB settings */
         switch (Irq)
         {
            case 2:   /* two is actually the interrupt cascade for IRQs > 7 */
               /* IRQ nine is the cascase for 2 */
               Irq = 9;
               
               /* IRQ OK so indicate one more valid item found */
               count++;
               break;

            case 3:
            case 4:
            case 5:
            case 7:
            case 9:
            case 10:
            case 11:
            case 12:
            case 15:
      
               /* IRQ OK so indicate one more valid item found */
               count++;
               break;

            default:
               logErr ("Invalid Sound Blaster IRQ specified.\n");
               logErr ("Possible values are:  ");
               logErr ("2, 3, 4, 5, 7, 9, 10, 11, 12, 15.\n");
         }
      }
      else
      {
         logErr ("Unable to read Sound Blaster IRQ.\n");
      }

      /* search for the DMA setting */
      ptr = strchr(env, 'D');
      if (ptr)
      {
         /* if valid, read and convert the DMA */
         Dma = hextodec (ptr[1]);

         /* verify the DMA setting is one of the possible 8-bit SB settings */
         switch (Dma)
         {
            case 0:
            case 1:
            case 3:
               /* DMA OK so indicate one more valid item found */
               count++;
               break;

            default:
               logErr ("Invalid Sound Blaster 8-bit DMA specified.\n");
               logErr ("Possible values are:  ");
               logErr ("0, 1, 3.\n");
         }
      }
      else
      {
         logErr ("Unable to read Sound Blaster DMA setting.\n");
      }
   }
   else
   {  
      logErr ("BLASTER enviroment variable not configured.");
   }

   return (count != 3);
}


/*****************************************************************************/
/*                                                                           */
/* Module:  low_malloc                                                       */
/* Purpose: To allocate memory in the first 640K of memory                   */
/* Author:  Neil Bradley                                                     */
/* Date:    December 16, 1996                                                */
/*                                                                           */
/*****************************************************************************/

#ifdef __WATCOMC__
void dos_memalloc(unsigned short int para, unsigned short int *seg, unsigned short int *sel);
#pragma  aux dos_memalloc = \
  "push  ecx"               \
  "push  edx"               \
  "mov   ax, 0100h"         \
  "int   31h"               \
  "pop   ebx"               \
  "mov   [ebx], dx"         \
  "pop   ebx"               \
  "mov   [ebx], ax"         \
  parm   [bx] [ecx] [edx]   \
  modify [ax ebx ecx edx];

void dos_memfree(short int sel);
#pragma  aux dos_memfree =  \
  "mov   ax, 0101h"         \
  "int   31h"               \
  parm   [dx]               \
  modify [ax dx];

void *low_malloc(int size)
{
    unsigned short int seg;
    unsigned short int i=0;

    dos_memalloc((size >> 4) + 1, &seg, &i);
    return((char *)(seg << 4));
}
#endif


/*****************************************************************************/
/*                                                                           */
/* Module:  Set_master_volume                                                */
/* Purpose: To set the Sound Blaster's master volume                         */
/* Author:  Neil Bradley                                                     */
/* Date:    January 1, 1997                                                  */
/*                                                                           */
/*****************************************************************************/

void Set_master_volume(uint8 left, uint8 right)

{
   /* if the SB was initialized */
   if (Sb_init)
   {
      outp(IOaddr + 0x04, MASTER_VOLUME);
      outp(IOaddr + 0x05, ((left & 0xf) << 4) + (right & 0x0f));
   }
}


/*****************************************************************************/
/*                                                                           */
/* Module:  Set_line_volume                                                  */
/* Purpose: To set the Sound Blaster's line level volume                     */
/* Author:  Neil Bradley                                                     */
/* Date:    January 1, 1997                                                  */
/*                                                                           */
/*****************************************************************************/

void Set_line_volume(uint8 left, uint8 right)
{
   /* if the SB was initialized */
   if (Sb_init)
   {
      outp(IOaddr + 0x04, LINE_VOLUME);
      outp(IOaddr + 0x05, ((left & 0xf) << 4) + (right & 0x0f));
   }
}


/*****************************************************************************/
/*                                                                           */
/* Module:  Set_FM_volume                                                    */
/* Purpose: To set the Sound Blaster's FM volume                             */
/* Author:  Neil Bradley                                                     */
/* Date:    January 1, 1997                                                  */
/*                                                                           */
/*****************************************************************************/

void Set_FM_volume(uint8 left, uint8 right)

{
   /* if the SB was initialized */
   if (Sb_init)
   {
      outp(IOaddr + 0x04, FM_VOLUME);
      outp(IOaddr + 0x05, ((left & 0xf) << 4) + (right & 0x0f));
   }
}


/*****************************************************************************/
/*                                                                           */
/* Module:  ResetDSP                                                         */
/* Purpose: To reset the SB DSP.  Returns a value of zero if unsuccessful.   */
/*          This function requires as input the SB base port address.        */
/* Author:  Ron Fries                                                        */
/* Date:    August 5, 1997                                                   */
/*                                                                           */
/*****************************************************************************/

uint8 ResetDSP(uint16 ioaddr)
{
   uint8  x;
   uint16 y;

   /* assume the init was not successful */
   Sb_init = 0;

   /* send a DSP reset to the SB */
   outp (ioaddr + DSP_RESET, 1);

   /* wait a few microsec */
   x = inp(ioaddr + DSP_RESET);
   x = inp(ioaddr + DSP_RESET);
   x = inp(ioaddr + DSP_RESET);
   x = inp(ioaddr + DSP_RESET);

   /* clear the DSP reset */
   outp (ioaddr + DSP_RESET,0);

   /* wait a bit until the SB indicates good status */
   y = 0;

   do
   {
      x = inp (ioaddr + DSP_READ);
      y++;
   } while ((y < 1000) && (x != 0xaa));

   /* if we were able to successfully reset the SB */
   if (x == 0xaa)
   {
      /* turn on speaker */
      dsp_out (ioaddr + DSP_WRITE, 0xd1);

      /* read to make sure DSP register is clear */
      dsp_in (ioaddr + DSP_READ);

      /* set time constant */
      dsp_out (ioaddr + DSP_WRITE, 0x40);
      dsp_out (ioaddr + DSP_WRITE,
         (unsigned char)(256 - 1000000L/Playback_freq));

      /* indicate successful initialization */
      Sb_init = 1;
   }

   return (Sb_init);
}


/*****************************************************************************/
/*                                                                           */
/* Module:  OpenSB                                                           */
/* Purpose: To reset the SB and prepare all buffers and other global         */
/*          global variables for sound output.  Allows the user to select    */
/*          the playback frequency, number of buffers, and size of each      */
/*          buffer.  Returns a value of zero if unsuccessful.                */
/* Author:  Ron Fries                                                        */
/* Date:    January 1, 1997                                                  */
/*                                                                           */
/*****************************************************************************/

uint8 OpenSB(uint16 playback_freq, uint16 buffer_size)
{
   /* initialize local globals */
   if (buffer_size > 0)
   {
      Sb_buf_size = buffer_size;
   }

   Playback_freq = playback_freq;

   /* assume the init was not successful */
   Sb_init = 0;

   /* attempt to read the Blaster Environment Variable */
   if (getBlasterEnv() == 0)
   {
      /* if the DSP could be successfully reset */
      if (ResetDSP(IOaddr) != 0  )
      {
         /* setup the DSP interrupt service routine */
         setNewIntVect(Irq);

         /* Enable the interrupt used */
         if (Irq > 7)
         {
            outp (0xa1,inp(0xa1) & (~(1<<(Irq-8))));
         }
         else
         {
            outp (0x21,inp(0x21) & (~(1<<Irq)));
         }

         /* make sure interrupts are enabled */
         _enable();

         /* create a buffer to hold the data */
#ifdef __WATCOMC__
         Sb_buffer = low_malloc (Sb_buf_size*2);
#elif defined(DJGPP)
         Sb_buffer = (uint8 *)malloc(Sb_buf_size*2);
         theDOSBufferSegment = __dpmi_allocate_dos_memory((Sb_buf_size*2+15) >> 4,
             &theDOSBufferSelector);
#else
         Sb_buffer = malloc (Sb_buf_size*2);
#endif

         /* if we were unable to successfully allocate the buffer */
#ifdef DJGPP
         if ((Sb_buffer == 0) || (theDOSBufferSegment == -1))
#else
         if (Sb_buffer == 0)
#endif
         {
            logErr ("Unable to allocate buffer for audio output.\n");

            /* close the SB */
            CloseSB();
         }
      }
      else
      {
         logErr ("Unable to initialize the Sound Card.\n");
      }

   }

   return (Sb_init);
}


/*****************************************************************************/
/*                                                                           */
/* Module:  CloseSB                                                          */
/* Purpose: Closes the SB and disables the interrupts.                       */
/* Author:  Ron Fries                                                        */
/* Date:    January 1, 1997                                                  */
/*                                                                           */
/*****************************************************************************/

void CloseSB(void)
{
#ifdef __WATCOMC__
   uint32 addr;
#endif

   /* if the SB was initialized */
   if (Sb_init)
   {
      /* stop all DMA transfer */
      Stop_audio_output();
      ResetDSP(IOaddr);

      /* turn the speaker off */
      dsp_out (IOaddr + DSP_WRITE, 0xd3);

      /* indicate SB no longer active */
      Sb_init = 0;

      /* Disable the interrupt used */
      if (Irq > 7)
      {
         outp (0xa1,inp(0xa1) | (1<<(Irq-8)));
      }
      else
      {
         outp (0x21,inp(0x21) | (1<<Irq));
      }

      /* restore the original interrupt routine */
      setOldIntVect(Irq);

      /* free any memory that had been allocated */
      if (Sb_buffer != 0)
      {
#ifdef __WATCOMC__
         addr = (uint32) Sb_buffer;
         dos_memfree((uint16)(addr >> 4));
#elif defined(DJGPP)
         free(Sb_buffer);
         __dpmi_free_dos_memory(theDOSBufferSelector);
#else
         free (Sb_buffer);
#endif
      }
   }
}


/*****************************************************************************/
/*                                                                           */
/* Module:  Stop_audio_output                                                */
/* Purpose: Stops the SB's DMA transfer.                                     */
/* Author:  Ron Fries                                                        */
/* Date:    January 17, 1997                                                 */
/*                                                                           */
/*****************************************************************************/

void Stop_audio_output (void)
{
   /* stop any transfer that may be in progress */

   /* if the SB was initialized */
   if (Sb_init)
   {
      /* halt DMA */
      dsp_out (IOaddr + DSP_WRITE, 0xd0);

      /* exit DMA operation*/
      dsp_out (IOaddr + DSP_WRITE, 0xda);

      /* halt DMA */
      dsp_out (IOaddr + DSP_WRITE, 0xd0);
   }
}


/*****************************************************************************/
/*                                                                           */
/* Module:  Start_audio_output                                               */
/* Purpose: Fills all configured buffers and outputs the first.              */
/* Author:  Ron Fries                                                        */
/* Date:    February 20, 1997                                                */
/*                                                                           */
/*****************************************************************************/

uint8 Start_audio_output (uint8 dma_mode,
                          void (*fillBuffer)(uint8 *buf,uint16 n))
{
   uint8  ret_val = 1;
   static uint8 pagePort[8] = { 0x87, 0x83, 0x81, 0x82 };
   uint8  offset_low;
   uint8  offset_high;
   uint8  page_no;
   uint8  count_low;
   uint8  count_high;
   uint32 addr;
   clock_t start_time;

   /* if the SB initialized properly */
   if (Sb_init)
   {
      /* set the fill buffer routine */
      FillBuffer = fillBuffer;

      /* keep track of the DMA selection */
      DMAmode = dma_mode;

      /* stop any transfer that may be in progress */
      Stop_audio_output();

      /* fill the buffer */
      FillBuffer (Sb_buffer, Sb_buf_size*2);

#ifdef DJGPP
      /* Copy data to DOS memory buffer */
      dosmemput(Sb_buffer, Sb_buf_size * 2, theDOSBufferSegment << 4);
#endif

      /* calculate high, low and page addresses of buffer */
#ifdef __WATCOMC__
      addr = (uint32) Sb_buffer;
#elif defined(DJGPP)
      addr = ((uint32) theDOSBufferSegment) << 4;
#else
      addr = ((uint32)FP_SEG(Sb_buffer) << 4) +
              (uint32)FP_OFF(Sb_buffer);
#endif
      Sb_offset = (uint16)(addr & 0x0ffff);
      offset_low  = (uint8)(addr & 0x0ff);
      offset_high = (uint8)((addr >> 8) & 0x0ff);
      page_no     = (uint8)(addr >> 16);

      count_low = (uint8) ((Sb_buf_size*2)-1) & 0x0ff;
      count_high = (uint8) (((Sb_buf_size*2)-1) >> 8) & 0x0ff;

      /* program the DMAC for output transfer */
      outp (DMA_MASK              , 0x04 | Dma );
      outp (DMA_FF                , 0 );

      /* select auto-initialize DMA mode */
      outp (DMA_MODE              , 0x58 | Dma );
      outp (DMA_BASE + (Dma << 1) , offset_low );
      outp (DMA_BASE + (Dma << 1) , offset_high );
      outp (pagePort[Dma]         , page_no );
      outp (DMA_COUNT + (Dma << 1), count_low );
      outp (DMA_COUNT + (Dma << 1), count_high );
      outp (DMA_MASK              , Dma );

      /* calculate the high/low buffer size counts */
      Count_low = (uint8) (Sb_buf_size-1) & 0x0ff;
      Count_high = (uint8) ((Sb_buf_size-1) >> 8) & 0x0ff;

      if (DMAmode == STANDARD_DMA)
      {
         /* start the standard DMA transfer */
         dsp_out (IOaddr + DSP_WRITE, 0x14);
         dsp_out (IOaddr + DSP_WRITE, Count_low);
         dsp_out (IOaddr + DSP_WRITE, Count_high);
      }
      else
      {
         /* reset the DMA counter */
         DMAcount = 0;

         /* set the auto-initialize buffer size */
         dsp_out (IOaddr + DSP_WRITE, 0x48);
         dsp_out (IOaddr + DSP_WRITE, Count_low);
         dsp_out (IOaddr + DSP_WRITE, Count_high);

         /* and start the auto-initialize DMA transfer */
         dsp_out (IOaddr + DSP_WRITE, 0x1c);

         start_time = clock();

         /* Delay for a bit and wait for DMAcount to change. */
         /* Wait for the DMA to be called twice to make sure */
         /* auto-init mode is working properly. */
         while ((clock()-start_time < (int)(CLK_TCK/2)) && (DMAcount < 2))
         {
            /* This routine will wait for up to 1/2 second for DMAcount */
            /* to change.  The value in CLK_TCK is the number of times */
            /* the clock will tick in one second. */
         }

         /* if the auto-init DMA is not active */
         if (DMAcount < 2)
         {
            /* Reset the SB DSP */
            ResetDSP(IOaddr);

            /* then try again with STANDARD_DMA */
            Start_audio_output (STANDARD_DMA, fillBuffer);
         }
      }

      ret_val = 0;
   }

   return (ret_val);
}
