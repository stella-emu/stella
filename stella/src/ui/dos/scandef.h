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

#define  SCAN_BASE_OFFSET  0x0000
#define  SCAN_SHIFT_OFFSET 0x0080
#define  SCAN_CTRL_OFFSET  0x0100
#define  SCAN_ALT_OFFSET   0x0180


#define  SCAN_BASE        (SCAN_BASE_OFFSET+0x00)
#define  SCAN_ESC         (SCAN_BASE_OFFSET+0x01)
#define  SCAN_1           (SCAN_BASE_OFFSET+0x02)
#define  SCAN_2           (SCAN_BASE_OFFSET+0x03)
#define  SCAN_3           (SCAN_BASE_OFFSET+0x04)
#define  SCAN_4           (SCAN_BASE_OFFSET+0x05)
#define  SCAN_5           (SCAN_BASE_OFFSET+0x06)
#define  SCAN_6           (SCAN_BASE_OFFSET+0x07)
#define  SCAN_7           (SCAN_BASE_OFFSET+0x08)
#define  SCAN_8           (SCAN_BASE_OFFSET+0x09)
#define  SCAN_9           (SCAN_BASE_OFFSET+0x0a)
#define  SCAN_0           (SCAN_BASE_OFFSET+0x0b)
#define  SCAN_MINUS       (SCAN_BASE_OFFSET+0x0c)
#define  SCAN_EQUALS      (SCAN_BASE_OFFSET+0x0d)
#define  SCAN_BSPACE      (SCAN_BASE_OFFSET+0x0e)
#define  SCAN_TAB         (SCAN_BASE_OFFSET+0x0f)

#define  SCAN_Q           (SCAN_BASE_OFFSET+0x10)
#define  SCAN_W           (SCAN_BASE_OFFSET+0x11)
#define  SCAN_E           (SCAN_BASE_OFFSET+0x12)
#define  SCAN_R           (SCAN_BASE_OFFSET+0x13)
#define  SCAN_T           (SCAN_BASE_OFFSET+0x14)
#define  SCAN_Y           (SCAN_BASE_OFFSET+0x15)
#define  SCAN_U           (SCAN_BASE_OFFSET+0x16)
#define  SCAN_I           (SCAN_BASE_OFFSET+0x17)
#define  SCAN_O           (SCAN_BASE_OFFSET+0x18)
#define  SCAN_P           (SCAN_BASE_OFFSET+0x19)
#define  SCAN_LSQRB       (SCAN_BASE_OFFSET+0x1a)
#define  SCAN_RSQRB       (SCAN_BASE_OFFSET+0x1b)
#define  SCAN_RETURN      (SCAN_BASE_OFFSET+0x1c)
#define  SCAN_ENTER       (SCAN_BASE_OFFSET+0x1c)
#define  SCAN_SPACE       (SCAN_BASE_OFFSET+0x39)
#define  SCAN_BREAK       (SCAN_BASE_OFFSET+0x1d)     // Printed as 0x9d ???
#define  SCAN_CTRL        (SCAN_BASE_OFFSET+0x1d)
#define  SCAN_A           (SCAN_BASE_OFFSET+0x1e)
#define  SCAN_S           (SCAN_BASE_OFFSET+0x1f)

#define  SCAN_D           (SCAN_BASE_OFFSET+0x20)
#define  SCAN_F           (SCAN_BASE_OFFSET+0x21)
#define  SCAN_G           (SCAN_BASE_OFFSET+0x22)
#define  SCAN_H           (SCAN_BASE_OFFSET+0x23)
#define  SCAN_J           (SCAN_BASE_OFFSET+0x24)
#define  SCAN_K           (SCAN_BASE_OFFSET+0x25)
#define  SCAN_L           (SCAN_BASE_OFFSET+0x26)
#define  SCAN_SCOLON      (SCAN_BASE_OFFSET+0x27)
#define  SCAN_APSTPY      (SCAN_BASE_OFFSET+0x28)     // Apostrophy '''''
#define  SCAN_TILDE       (SCAN_BASE_OFFSET+0x29)
#define  SCAN_LSHIFT      (SCAN_BASE_OFFSET+0x2a)
#define  SCAN_HASH        (SCAN_BASE_OFFSET+0x2b)
#define  SCAN_Z           (SCAN_BASE_OFFSET+0x2c)
#define  SCAN_X           (SCAN_BASE_OFFSET+0x2d)
#define  SCAN_C           (SCAN_BASE_OFFSET+0x2e)
#define  SCAN_V           (SCAN_BASE_OFFSET+0x2f)

#define  SCAN_B           (SCAN_BASE_OFFSET+0x30)
#define  SCAN_N           (SCAN_BASE_OFFSET+0x31)
#define  SCAN_M           (SCAN_BASE_OFFSET+0x32)
#define  SCAN_COMMA       (SCAN_BASE_OFFSET+0x33)
#define  SCAN_STOP        (SCAN_BASE_OFFSET+0x34)
#define  SCAN_FSLASH      (SCAN_BASE_OFFSET+0x35)
#define  SCAN_RSHIFT      (SCAN_BASE_OFFSET+0x36)
#define  SCAN_STAR        (SCAN_BASE_OFFSET+0x37)
#define  SCAN_ALT         (SCAN_BASE_OFFSET+0x38)

#define  SCAN_CAPS        (SCAN_BASE_OFFSET+0x3a)
#define  SCAN_F1          (SCAN_BASE_OFFSET+0x3b)
#define  SCAN_F2          (SCAN_BASE_OFFSET+0x3c)
#define  SCAN_F3          (SCAN_BASE_OFFSET+0x3d)
#define  SCAN_F4          (SCAN_BASE_OFFSET+0x3e)
#define  SCAN_F5          (SCAN_BASE_OFFSET+0x3f)

#define  SCAN_F6          (SCAN_BASE_OFFSET+0x40)
#define  SCAN_F7          (SCAN_BASE_OFFSET+0x41)
#define  SCAN_F8          (SCAN_BASE_OFFSET+0x42)
#define  SCAN_F9          (SCAN_BASE_OFFSET+0x43)
#define  SCAN_F10         (SCAN_BASE_OFFSET+0x44)
#define  SCAN_NUMLCK      (SCAN_BASE_OFFSET+0x45)
#define  SCAN_SCRLCK      (SCAN_BASE_OFFSET+0x46)
#define  SCAN_HOME        (SCAN_BASE_OFFSET+0x47)
#define  SCAN_UP          (SCAN_BASE_OFFSET+0x48)
#define  SCAN_PGUP        (SCAN_BASE_OFFSET+0x49)
#define  SCAN_DASH        (SCAN_BASE_OFFSET+0x4a)     // Number pad minus

#define  SCAN_LEFT        (SCAN_BASE_OFFSET+0x4b)
#define  SCAN_CENTRE      (SCAN_BASE_OFFSET+0x4c)     // Number pad centre

#define  SCAN_RIGHT       (SCAN_BASE_OFFSET+0x4d)
#define  SCAN_PLUS        (SCAN_BASE_OFFSET+0x4e)     // Number pad plus

#define  SCAN_END         (SCAN_BASE_OFFSET+0x4f)

#define  SCAN_DOWN        (SCAN_BASE_OFFSET+0x50)
#define  SCAN_PGDN        (SCAN_BASE_OFFSET+0x51)
#define  SCAN_INS         (SCAN_BASE_OFFSET+0x52)
#define  SCAN_DEL         (SCAN_BASE_OFFSET+0x53)

#define  SCAN_BSLASH      (SCAN_BASE_OFFSET+0x56)
#define  SCAN_F11         (SCAN_BASE_OFFSET+0x57)
#define  SCAN_F12         (SCAN_BASE_OFFSET+0x58)


//
// Scancodes with shift held
//

#define  SCAN_SHFT_BASE   (SCAN_SHIFT_OFFSET+0x00)
#define  SCAN_SHFT_ESC    (SCAN_SHIFT_OFFSET+0x01)
#define  SCAN_SHFT_1      (SCAN_SHIFT_OFFSET+0x02)
#define  SCAN_SHFT_2      (SCAN_SHIFT_OFFSET+0x03)
#define  SCAN_SHFT_3      (SCAN_SHIFT_OFFSET+0x04)
#define  SCAN_SHFT_4      (SCAN_SHIFT_OFFSET+0x05)
#define  SCAN_SHFT_5      (SCAN_SHIFT_OFFSET+0x06)
#define  SCAN_SHFT_6      (SCAN_SHIFT_OFFSET+0x07)
#define  SCAN_SHFT_7      (SCAN_SHIFT_OFFSET+0x08)
#define  SCAN_SHFT_8      (SCAN_SHIFT_OFFSET+0x09)
#define  SCAN_SHFT_9      (SCAN_SHIFT_OFFSET+0x0a)
#define  SCAN_SHFT_0      (SCAN_SHIFT_OFFSET+0x0b)
#define  SCAN_SHFT_MINUS  (SCAN_SHIFT_OFFSET+0x0c)
#define  SCAN_SHFT_EQUALS (SCAN_SHIFT_OFFSET+0x0d)
#define  SCAN_SHFT_BSPACE (SCAN_SHIFT_OFFSET+0x0e)
#define  SCAN_SHFT_TAB    (SCAN_SHIFT_OFFSET+0x0f)

#define  SCAN_SHFT_Q      (SCAN_SHIFT_OFFSET+0x10)
#define  SCAN_SHFT_W      (SCAN_SHIFT_OFFSET+0x11)
#define  SCAN_SHFT_E      (SCAN_SHIFT_OFFSET+0x12)
#define  SCAN_SHFT_R      (SCAN_SHIFT_OFFSET+0x13)
#define  SCAN_SHFT_T      (SCAN_SHIFT_OFFSET+0x14)
#define  SCAN_SHFT_Y      (SCAN_SHIFT_OFFSET+0x15)
#define  SCAN_SHFT_U      (SCAN_SHIFT_OFFSET+0x16)
#define  SCAN_SHFT_I      (SCAN_SHIFT_OFFSET+0x17)
#define  SCAN_SHFT_O      (SCAN_SHIFT_OFFSET+0x18)
#define  SCAN_SHFT_P      (SCAN_SHIFT_OFFSET+0x19)
#define  SCAN_SHFT_LSQRB  (SCAN_SHIFT_OFFSET+0x1a)
#define  SCAN_SHFT_RSQRB  (SCAN_SHIFT_OFFSET+0x1b)
#define  SCAN_SHFT_RETURN (SCAN_SHIFT_OFFSET+0x1c)
#define  SCAN_SHFT_ENTER  (SCAN_SHIFT_OFFSET+0x1c)
#define  SCAN_SHFT_BREAK  (SCAN_SHIFT_OFFSET+0x1d)     // Printed as 0x9d ???
#define  SCAN_SHFT_CTRL   (SCAN_SHIFT_OFFSET+0x1d)
#define  SCAN_SHFT_A      (SCAN_SHIFT_OFFSET+0x1e)
#define  SCAN_SHFT_S      (SCAN_SHIFT_OFFSET+0x1f)

#define  SCAN_SHFT_D      (SCAN_SHIFT_OFFSET+0x20)
#define  SCAN_SHFT_F      (SCAN_SHIFT_OFFSET+0x21)
#define  SCAN_SHFT_G      (SCAN_SHIFT_OFFSET+0x22)
#define  SCAN_SHFT_H      (SCAN_SHIFT_OFFSET+0x23)
#define  SCAN_SHFT_J      (SCAN_SHIFT_OFFSET+0x24)
#define  SCAN_SHFT_K      (SCAN_SHIFT_OFFSET+0x25)
#define  SCAN_SHFT_L      (SCAN_SHIFT_OFFSET+0x26)
#define  SCAN_SHFT_SCOLON (SCAN_SHIFT_OFFSET+0x27)
#define  SCAN_SHFT_APSTPY (SCAN_SHIFT_OFFSET+0x28)     // Apostrophy '''''
#define  SCAN_SHFT_SQGL   (SCAN_SHIFT_OFFSET+0x29)     // ªªªªªªªª
#define  SCAN_SHFT_LSHIFT (SCAN_SHIFT_OFFSET+0x2a)
#define  SCAN_SHFT_HASH   (SCAN_SHIFT_OFFSET+0x2b)
#define  SCAN_SHFT_Z      (SCAN_SHIFT_OFFSET+0x2c)
#define  SCAN_SHFT_X      (SCAN_SHIFT_OFFSET+0x2d)
#define  SCAN_SHFT_C      (SCAN_SHIFT_OFFSET+0x2e)
#define  SCAN_SHFT_V      (SCAN_SHIFT_OFFSET+0x2f)

#define  SCAN_SHFT_B      (SCAN_SHIFT_OFFSET+0x30)
#define  SCAN_SHFT_N      (SCAN_SHIFT_OFFSET+0x31)
#define  SCAN_SHFT_M      (SCAN_SHIFT_OFFSET+0x32)
#define  SCAN_SHFT_COMMA  (SCAN_SHIFT_OFFSET+0x33)
#define  SCAN_SHFT_STOP   (SCAN_SHIFT_OFFSET+0x34)
#define  SCAN_SHFT_FSLASH (SCAN_SHIFT_OFFSET+0x35)
#define  SCAN_SHFT_RSHIFT (SCAN_SHIFT_OFFSET+0x36)
#define  SCAN_SHFT_STAR   (SCAN_SHIFT_OFFSET+0x37)
#define  SCAN_SHFT_ALT    (SCAN_SHIFT_OFFSET+0x38)

#define  SCAN_SHFT_CAPS   (SCAN_SHIFT_OFFSET+0x3a)
#define  SCAN_SHFT_F1     (SCAN_SHIFT_OFFSET+0x3b)
#define  SCAN_SHFT_F2     (SCAN_SHIFT_OFFSET+0x3c)
#define  SCAN_SHFT_F3     (SCAN_SHIFT_OFFSET+0x3d)
#define  SCAN_SHFT_F4     (SCAN_SHIFT_OFFSET+0x3e)
#define  SCAN_SHFT_F5     (SCAN_SHIFT_OFFSET+0x3f)

#define  SCAN_SHFT_F6     (SCAN_SHIFT_OFFSET+0x40)
#define  SCAN_SHFT_F7     (SCAN_SHIFT_OFFSET+0x41)
#define  SCAN_SHFT_F8     (SCAN_SHIFT_OFFSET+0x42)
#define  SCAN_SHFT_F9     (SCAN_SHIFT_OFFSET+0x43)
#define  SCAN_SHFT_F10    (SCAN_SHIFT_OFFSET+0x44)
#define  SCAN_SHFT_NUMLCK (SCAN_SHIFT_OFFSET+0x45)
#define  SCAN_SHFT_SCRLCK (SCAN_SHIFT_OFFSET+0x46)
#define  SCAN_SHFT_HOME   (SCAN_SHIFT_OFFSET+0x47)
#define  SCAN_SHFT_UP     (SCAN_SHIFT_OFFSET+0x48)
#define  SCAN_SHFT_PGUP   (SCAN_SHIFT_OFFSET+0x49)
#define  SCAN_SHFT_DASH   (SCAN_SHIFT_OFFSET+0x4a)     // Number pad minus

#define  SCAN_SHFT_LEFT   (SCAN_SHIFT_OFFSET+0x4b)
#define  SCAN_SHFT_CENTRE (SCAN_SHIFT_OFFSET+0x4c)     // Number pad centre

#define  SCAN_SHFT_RIGHT  (SCAN_SHIFT_OFFSET+0x4d)
#define  SCAN_SHFT_PLUS   (SCAN_SHIFT_OFFSET+0x4e)     // Number pad plus

#define  SCAN_SHFT_END    (SCAN_SHIFT_OFFSET+0x4f)

#define  SCAN_SHFT_DOWN   (SCAN_SHIFT_OFFSET+0x50)
#define  SCAN_SHFT_PGDN   (SCAN_SHIFT_OFFSET+0x51)
#define  SCAN_SHFT_INS    (SCAN_SHIFT_OFFSET+0x52)
#define  SCAN_SHFT_DEL    (SCAN_SHIFT_OFFSET+0x53)

#define  SCAN_SHFT_BSLASH (SCAN_SHIFT_OFFSET+0x56)
#define  SCAN_SHFT_F11    (SCAN_SHIFT_OFFSET+0x57)
#define  SCAN_SHFT_F12    (SCAN_SHIFT_OFFSET+0x58)



//
// Scancodes with ctrl held
//

#define  SCAN_CTRL_BASE   (SCAN_CTRL_OFFSET+0x00)
#define  SCAN_CTRL_ESC    (SCAN_CTRL_OFFSET+0x01)
#define  SCAN_CTRL_1      (SCAN_CTRL_OFFSET+0x02)
#define  SCAN_CTRL_2      (SCAN_CTRL_OFFSET+0x03)
#define  SCAN_CTRL_3      (SCAN_CTRL_OFFSET+0x04)
#define  SCAN_CTRL_4      (SCAN_CTRL_OFFSET+0x05)
#define  SCAN_CTRL_5      (SCAN_CTRL_OFFSET+0x06)
#define  SCAN_CTRL_6      (SCAN_CTRL_OFFSET+0x07)
#define  SCAN_CTRL_7      (SCAN_CTRL_OFFSET+0x08)
#define  SCAN_CTRL_8      (SCAN_CTRL_OFFSET+0x09)
#define  SCAN_CTRL_9      (SCAN_CTRL_OFFSET+0x0a)
#define  SCAN_CTRL_0      (SCAN_CTRL_OFFSET+0x0b)
#define  SCAN_CTRL_MINUS  (SCAN_CTRL_OFFSET+0x0c)
#define  SCAN_CTRL_EQUALS (SCAN_CTRL_OFFSET+0x0d)
#define  SCAN_CTRL_BSPACE (SCAN_CTRL_OFFSET+0x0e)
#define  SCAN_CTRL_TAB    (SCAN_CTRL_OFFSET+0x0f)

#define  SCAN_CTRL_Q      (SCAN_CTRL_OFFSET+0x10)
#define  SCAN_CTRL_W      (SCAN_CTRL_OFFSET+0x11)
#define  SCAN_CTRL_E      (SCAN_CTRL_OFFSET+0x12)
#define  SCAN_CTRL_R      (SCAN_CTRL_OFFSET+0x13)
#define  SCAN_CTRL_T      (SCAN_CTRL_OFFSET+0x14)
#define  SCAN_CTRL_Y      (SCAN_CTRL_OFFSET+0x15)
#define  SCAN_CTRL_U      (SCAN_CTRL_OFFSET+0x16)
#define  SCAN_CTRL_I      (SCAN_CTRL_OFFSET+0x17)
#define  SCAN_CTRL_O      (SCAN_CTRL_OFFSET+0x18)
#define  SCAN_CTRL_P      (SCAN_CTRL_OFFSET+0x19)
#define  SCAN_CTRL_LSQRB  (SCAN_CTRL_OFFSET+0x1a)
#define  SCAN_CTRL_RSQRB  (SCAN_CTRL_OFFSET+0x1b)
#define  SCAN_CTRL_RETURN (SCAN_CTRL_OFFSET+0x1c)
#define  SCAN_CTRL_ENTER  (SCAN_CTRL_OFFSET+0x1c)
#define  SCAN_CTRL_BREAK  (SCAN_CTRL_OFFSET+0x1d)     // Printed as 0x9d ???
#define  SCAN_CTRL_CTRL   (SCAN_CTRL_OFFSET+0x1d)
#define  SCAN_CTRL_A      (SCAN_CTRL_OFFSET+0x1e)
#define  SCAN_CTRL_S      (SCAN_CTRL_OFFSET+0x1f)

#define  SCAN_CTRL_D      (SCAN_CTRL_OFFSET+0x20)
#define  SCAN_CTRL_F      (SCAN_CTRL_OFFSET+0x21)
#define  SCAN_CTRL_G      (SCAN_CTRL_OFFSET+0x22)
#define  SCAN_CTRL_H      (SCAN_CTRL_OFFSET+0x23)
#define  SCAN_CTRL_J      (SCAN_CTRL_OFFSET+0x24)
#define  SCAN_CTRL_K      (SCAN_CTRL_OFFSET+0x25)
#define  SCAN_CTRL_L      (SCAN_CTRL_OFFSET+0x26)
#define  SCAN_CTRL_SCOLON (SCAN_CTRL_OFFSET+0x27)
#define  SCAN_CTRL_APSTPY (SCAN_CTRL_OFFSET+0x28)     // Apostrophy '''''
#define  SCAN_CTRL_SQGL   (SCAN_CTRL_OFFSET+0x29)     // ªªªªªªªª
#define  SCAN_CTRL_LSHIFT (SCAN_CTRL_OFFSET+0x2a)
#define  SCAN_CTRL_HASH   (SCAN_CTRL_OFFSET+0x2b)
#define  SCAN_CTRL_Z      (SCAN_CTRL_OFFSET+0x2c)
#define  SCAN_CTRL_X      (SCAN_CTRL_OFFSET+0x2d)
#define  SCAN_CTRL_C      (SCAN_CTRL_OFFSET+0x2e)
#define  SCAN_CTRL_V      (SCAN_CTRL_OFFSET+0x2f)

#define  SCAN_CTRL_B      (SCAN_CTRL_OFFSET+0x30)
#define  SCAN_CTRL_N      (SCAN_CTRL_OFFSET+0x31)
#define  SCAN_CTRL_M      (SCAN_CTRL_OFFSET+0x32)
#define  SCAN_CTRL_COMMA  (SCAN_CTRL_OFFSET+0x33)
#define  SCAN_CTRL_STOP   (SCAN_CTRL_OFFSET+0x34)
#define  SCAN_CTRL_FSLASH (SCAN_CTRL_OFFSET+0x35)
#define  SCAN_CTRL_RSHIFT (SCAN_CTRL_OFFSET+0x36)
#define  SCAN_CTRL_STAR   (SCAN_CTRL_OFFSET+0x37)
#define  SCAN_CTRL_ALT    (SCAN_CTRL_OFFSET+0x38)

#define  SCAN_CTRL_CAPS   (SCAN_CTRL_OFFSET+0x3a)
#define  SCAN_CTRL_F1     (SCAN_CTRL_OFFSET+0x3b)
#define  SCAN_CTRL_F2     (SCAN_CTRL_OFFSET+0x3c)
#define  SCAN_CTRL_F3     (SCAN_CTRL_OFFSET+0x3d)
#define  SCAN_CTRL_F4     (SCAN_CTRL_OFFSET+0x3e)
#define  SCAN_CTRL_F5     (SCAN_CTRL_OFFSET+0x3f)

#define  SCAN_CTRL_F6     (SCAN_CTRL_OFFSET+0x40)
#define  SCAN_CTRL_F7     (SCAN_CTRL_OFFSET+0x41)
#define  SCAN_CTRL_F8     (SCAN_CTRL_OFFSET+0x42)
#define  SCAN_CTRL_F9     (SCAN_CTRL_OFFSET+0x43)
#define  SCAN_CTRL_F10    (SCAN_CTRL_OFFSET+0x44)
#define  SCAN_CTRL_NUMLCK (SCAN_CTRL_OFFSET+0x45)
#define  SCAN_CTRL_SCRLCK (SCAN_CTRL_OFFSET+0x46)
#define  SCAN_CTRL_HOME   (SCAN_CTRL_OFFSET+0x47)
#define  SCAN_CTRL_UP     (SCAN_CTRL_OFFSET+0x48)
#define  SCAN_CTRL_PGUP   (SCAN_CTRL_OFFSET+0x49)
#define  SCAN_CTRL_DASH   (SCAN_CTRL_OFFSET+0x4a)     // Number pad minus

#define  SCAN_CTRL_LEFT   (SCAN_CTRL_OFFSET+0x4b)
#define  SCAN_CTRL_CENTRE (SCAN_CTRL_OFFSET+0x4c)     // Number pad centre

#define  SCAN_CTRL_RIGHT  (SCAN_CTRL_OFFSET+0x4d)
#define  SCAN_CTRL_PLUS   (SCAN_CTRL_OFFSET+0x4e)     // Number pad plus

#define  SCAN_CTRL_END    (SCAN_CTRL_OFFSET+0x4f)

#define  SCAN_CTRL_DOWN   (SCAN_CTRL_OFFSET+0x50)
#define  SCAN_CTRL_PGDN   (SCAN_CTRL_OFFSET+0x51)
#define  SCAN_CTRL_INS    (SCAN_CTRL_OFFSET+0x52)
#define  SCAN_CTRL_DEL    (SCAN_CTRL_OFFSET+0x53)

#define  SCAN_CTRL_BSLASH (SCAN_CTRL_OFFSET+0x56)
#define  SCAN_CTRL_F11    (SCAN_CTRL_OFFSET+0x57)
#define  SCAN_CTRL_F12    (SCAN_CTRL_OFFSET+0x58)




//
// Scancodes with alt held
//

#define  SCAN_ALT_BASE    (SCAN_ALT_OFFSET+0x00)
#define  SCAN_ALT_ESC     (SCAN_ALT_OFFSET+0x01)
#define  SCAN_ALT_1       (SCAN_ALT_OFFSET+0x02)
#define  SCAN_ALT_2       (SCAN_ALT_OFFSET+0x03)
#define  SCAN_ALT_3       (SCAN_ALT_OFFSET+0x04)
#define  SCAN_ALT_4       (SCAN_ALT_OFFSET+0x05)
#define  SCAN_ALT_5       (SCAN_ALT_OFFSET+0x06)
#define  SCAN_ALT_6       (SCAN_ALT_OFFSET+0x07)
#define  SCAN_ALT_7       (SCAN_ALT_OFFSET+0x08)
#define  SCAN_ALT_8       (SCAN_ALT_OFFSET+0x09)
#define  SCAN_ALT_9       (SCAN_ALT_OFFSET+0x0a)
#define  SCAN_ALT_0       (SCAN_ALT_OFFSET+0x0b)
#define  SCAN_ALT_MINUS   (SCAN_ALT_OFFSET+0x0c)
#define  SCAN_ALT_EQUALS  (SCAN_ALT_OFFSET+0x0d)
#define  SCAN_ALT_BSPACE  (SCAN_ALT_OFFSET+0x0e)
#define  SCAN_ALT_TAB     (SCAN_ALT_OFFSET+0x0f)

#define  SCAN_ALT_Q       (SCAN_ALT_OFFSET+0x10)
#define  SCAN_ALT_W       (SCAN_ALT_OFFSET+0x11)
#define  SCAN_ALT_E       (SCAN_ALT_OFFSET+0x12)
#define  SCAN_ALT_R       (SCAN_ALT_OFFSET+0x13)
#define  SCAN_ALT_T       (SCAN_ALT_OFFSET+0x14)
#define  SCAN_ALT_Y       (SCAN_ALT_OFFSET+0x15)
#define  SCAN_ALT_U       (SCAN_ALT_OFFSET+0x16)
#define  SCAN_ALT_I       (SCAN_ALT_OFFSET+0x17)
#define  SCAN_ALT_O       (SCAN_ALT_OFFSET+0x18)
#define  SCAN_ALT_P       (SCAN_ALT_OFFSET+0x19)
#define  SCAN_ALT_LSQRB   (SCAN_ALT_OFFSET+0x1a)
#define  SCAN_ALT_RSQRB   (SCAN_ALT_OFFSET+0x1b)
#define  SCAN_ALT_RETURN  (SCAN_ALT_OFFSET+0x1c)
#define  SCAN_ALT_ENTER   (SCAN_ALT_OFFSET+0x1c)
#define  SCAN_ALT_BREAK   (SCAN_ALT_OFFSET+0x1d)     // Printed as 0x9d ???
#define  SCAN_ALT_CTRL    (SCAN_ALT_OFFSET+0x1d)
#define  SCAN_ALT_A       (SCAN_ALT_OFFSET+0x1e)
#define  SCAN_ALT_S       (SCAN_ALT_OFFSET+0x1f)

#define  SCAN_ALT_D       (SCAN_ALT_OFFSET+0x20)
#define  SCAN_ALT_F       (SCAN_ALT_OFFSET+0x21)
#define  SCAN_ALT_G       (SCAN_ALT_OFFSET+0x22)
#define  SCAN_ALT_H       (SCAN_ALT_OFFSET+0x23)
#define  SCAN_ALT_J       (SCAN_ALT_OFFSET+0x24)
#define  SCAN_ALT_K       (SCAN_ALT_OFFSET+0x25)
#define  SCAN_ALT_L       (SCAN_ALT_OFFSET+0x26)
#define  SCAN_ALT_SCOLON  (SCAN_ALT_OFFSET+0x27)
#define  SCAN_ALT_APSTPY  (SCAN_ALT_OFFSET+0x28)     // Apostrophy '''''
#define  SCAN_ALT_SQGL    (SCAN_ALT_OFFSET+0x29)     // ªªªªªªªª
#define  SCAN_ALT_LSHIFT  (SCAN_ALT_OFFSET+0x2a)
#define  SCAN_ALT_HASH    (SCAN_ALT_OFFSET+0x2b)
#define  SCAN_ALT_Z       (SCAN_ALT_OFFSET+0x2c)
#define  SCAN_ALT_X       (SCAN_ALT_OFFSET+0x2d)
#define  SCAN_ALT_C       (SCAN_ALT_OFFSET+0x2e)
#define  SCAN_ALT_V       (SCAN_ALT_OFFSET+0x2f)

#define  SCAN_ALT_B       (SCAN_ALT_OFFSET+0x30)
#define  SCAN_ALT_N       (SCAN_ALT_OFFSET+0x31)
#define  SCAN_ALT_M       (SCAN_ALT_OFFSET+0x32)
#define  SCAN_ALT_COMMA   (SCAN_ALT_OFFSET+0x33)
#define  SCAN_ALT_STOP    (SCAN_ALT_OFFSET+0x34)
#define  SCAN_ALT_FSLASH  (SCAN_ALT_OFFSET+0x35)
#define  SCAN_ALT_RSHIFT  (SCAN_ALT_OFFSET+0x36)
#define  SCAN_ALT_STAR    (SCAN_ALT_OFFSET+0x37)
#define  SCAN_ALT_ALT     (SCAN_ALT_OFFSET+0x38)

#define  SCAN_ALT_CAPS    (SCAN_ALT_OFFSET+0x3a)
#define  SCAN_ALT_F1      (SCAN_ALT_OFFSET+0x3b)
#define  SCAN_ALT_F2      (SCAN_ALT_OFFSET+0x3c)
#define  SCAN_ALT_F3      (SCAN_ALT_OFFSET+0x3d)
#define  SCAN_ALT_F4      (SCAN_ALT_OFFSET+0x3e)
#define  SCAN_ALT_F5      (SCAN_ALT_OFFSET+0x3f)

#define  SCAN_ALT_F6      (SCAN_ALT_OFFSET+0x40)
#define  SCAN_ALT_F7      (SCAN_ALT_OFFSET+0x41)
#define  SCAN_ALT_F8      (SCAN_ALT_OFFSET+0x42)
#define  SCAN_ALT_F9      (SCAN_ALT_OFFSET+0x43)
#define  SCAN_ALT_F10     (SCAN_ALT_OFFSET+0x44)
#define  SCAN_ALT_NUMLCK  (SCAN_ALT_OFFSET+0x45)
#define  SCAN_ALT_SCRLCK  (SCAN_ALT_OFFSET+0x46)
#define  SCAN_ALT_HOME    (SCAN_ALT_OFFSET+0x47)
#define  SCAN_ALT_UP      (SCAN_ALT_OFFSET+0x48)
#define  SCAN_ALT_PGUP    (SCAN_ALT_OFFSET+0x49)
#define  SCAN_ALT_DASH    (SCAN_ALT_OFFSET+0x4a)     // Number pad minus

#define  SCAN_ALT_LEFT    (SCAN_ALT_OFFSET+0x4b)
#define  SCAN_ALT_CENTRE  (SCAN_ALT_OFFSET+0x4c)     // Number pad centre

#define  SCAN_ALT_RIGHT   (SCAN_ALT_OFFSET+0x4d)
#define  SCAN_ALT_PLUS    (SCAN_ALT_OFFSET+0x4e)     // Number pad plus

#define  SCAN_ALT_END     (SCAN_ALT_OFFSET+0x4f)

#define  SCAN_ALT_DOWN    (SCAN_ALT_OFFSET+0x50)
#define  SCAN_ALT_PGDN    (SCAN_ALT_OFFSET+0x51)
#define  SCAN_ALT_INS     (SCAN_ALT_OFFSET+0x52)
#define  SCAN_ALT_DEL     (SCAN_ALT_OFFSET+0x53)

#define  SCAN_ALT_BSLASH  (SCAN_ALT_OFFSET+0x56)
#define  SCAN_ALT_F11     (SCAN_ALT_OFFSET+0x57)
#define  SCAN_ALT_F12     (SCAN_ALT_OFFSET+0x58)





#endif


/* END OF SCANDEF.H */
