/*****************************************************************************/
/*                                                                           */
/*   LIBRARY Functions - Scan code definitions        Copyright K.W 1994     */
/*                                                                           */
/*****************************************************************************/
/*                                                                           */
/*   Title      : Keyboard Code Definitions                                  */
/*                                                                           */
/*   File Name  : SCANDEF.H                                                  */
/*                                                                           */
/*   Author     : Keith Wilkins                                              */
/*                                                                           */
/*   Version    : 0.01                                                       */
/*                                                                           */
/*   Desciption : This header file defines all of the scan code definitions  */
/*                                                                           */
/*   Functions  : None                                                       */
/*                                                                           */
/*****************************************************************************/
/*                                                                           */
/*   Revision History:                                                       */
/*                                                                           */
/*   Version    Date    Who  Description of changes                          */
/*   -------    ----    ---  ----------------------                          */
/*                                                                           */
/*    0.01    13/11/94  K.W  Creation                                        */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

#ifndef SCANDEF_H
    #define SCANDEF_H

#define  SCAN_BASE        (0x00)
#define  SCAN_PAUSE       (0x00)     // We'll use 0x00 for the pause code
#define  SCAN_ESC         (0x01)
#define  SCAN_1           (0x02)
#define  SCAN_2           (0x03)
#define  SCAN_3           (0x04)
#define  SCAN_4           (0x05)
#define  SCAN_5           (0x06)
#define  SCAN_6           (0x07)
#define  SCAN_7           (0x08)
#define  SCAN_8           (0x09)
#define  SCAN_9           (0x0a)
#define  SCAN_0           (0x0b)
#define  SCAN_MINUS       (0x0c)
#define  SCAN_EQUALS      (0x0d)
#define  SCAN_BSPACE      (0x0e)
#define  SCAN_TAB         (0x0f)

#define  SCAN_Q           (0x10)
#define  SCAN_W           (0x11)
#define  SCAN_E           (0x12)
#define  SCAN_R           (0x13)
#define  SCAN_T           (0x14)
#define  SCAN_Y           (0x15)
#define  SCAN_U           (0x16)
#define  SCAN_I           (0x17)
#define  SCAN_O           (0x18)
#define  SCAN_P           (0x19)
#define  SCAN_LSQRB       (0x1a)
#define  SCAN_RSQRB       (0x1b)
#define  SCAN_RETURN      (0x1c)
#define  SCAN_ENTER       (0x1c)
#define  SCAN_CTRL        (0x1d)
#define  SCAN_A           (0x1e)
#define  SCAN_S           (0x1f)

#define  SCAN_D           (0x20)
#define  SCAN_F           (0x21)
#define  SCAN_G           (0x22)
#define  SCAN_H           (0x23)
#define  SCAN_J           (0x24)
#define  SCAN_K           (0x25)
#define  SCAN_L           (0x26)
#define  SCAN_SCOLON      (0x27)
#define  SCAN_APSTPY      (0x28)     // Apostrophy '''''
#define  SCAN_TILDE       (0x29)
#define  SCAN_LSHIFT      (0x2a)
#define  SCAN_HASH        (0x2b)
#define  SCAN_Z           (0x2c)
#define  SCAN_X           (0x2d)
#define  SCAN_C           (0x2e)
#define  SCAN_V           (0x2f)

#define  SCAN_B           (0x30)
#define  SCAN_N           (0x31)
#define  SCAN_M           (0x32)
#define  SCAN_COMMA       (0x33)
#define  SCAN_STOP        (0x34)
#define  SCAN_FSLASH      (0x35)
#define  SCAN_RSHIFT      (0x36)
#define  SCAN_STAR        (0x37)
#define  SCAN_ALT         (0x38)
#define  SCAN_SPACE       (0x39)
#define  SCAN_CAPS        (0x3a)
#define  SCAN_F1          (0x3b)
#define  SCAN_F2          (0x3c)
#define  SCAN_F3          (0x3d)
#define  SCAN_F4          (0x3e)
#define  SCAN_F5          (0x3f)

#define  SCAN_F6          (0x40)
#define  SCAN_F7          (0x41)
#define  SCAN_F8          (0x42)
#define  SCAN_F9          (0x43)
#define  SCAN_F10         (0x44)
#define  SCAN_NUMLCK      (0x45)
#define  SCAN_SCRLCK      (0x46)
#define  SCAN_HOME        (0x47)
#define  SCAN_UP          (0x48)
#define  SCAN_PGUP        (0x49)
#define  SCAN_DASH        (0x4a)     // Number pad minus
#define  SCAN_LEFT        (0x4b)
#define  SCAN_CENTRE      (0x4c)     // Number pad centre
#define  SCAN_RIGHT       (0x4d)
#define  SCAN_PLUS        (0x4e)     // Number pad plus
#define  SCAN_END         (0x4f)

#define  SCAN_DOWN        (0x50)
#define  SCAN_PGDN        (0x51)
#define  SCAN_INS         (0x52)
#define  SCAN_DEL         (0x53)

#define  SCAN_BSLASH      (0x56)
#define  SCAN_F11         (0x57)
#define  SCAN_F12         (0x58)

#endif

/* END OF SCANDEF.H */
