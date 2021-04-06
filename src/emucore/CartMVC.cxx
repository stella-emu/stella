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
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "System.hxx"
#include "CartMVC.hxx"


/**
  Implementation of MovieCart.
  1K of memory is presented on the bus, but is repeated to fill the 4K image space.
  Contents are dynamically altered with streaming image and audio content as specific
  128-byte regions are entered.
  Original implementation: github.com/lodefmode/moviecart

  @author  Rob Bairos
*/

#include <string.h>
#include <stdint.h>
#include <fstream>
#include <string>

#define LO_JUMP_BYTE(X) ((X) & 0xff)
#define HI_JUMP_BYTE(X) ((((X) & 0xff00) >> 8) | 0x10)

#define	COLOR_BLUE			0x9A
#define	COLOR_WHITE			0x0E

#define OSD_FRAMES			180
#define BACK_SECONDS		10

#define TITLE_CYCLES 		1000000

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/**
  Simulate retrieval 512 byte chunks from a serial source
*/

class StreamReader
{
public:

	StreamReader()
	{
	}

	~StreamReader()
	{
		close();
	}

	bool	open(const std::string& path);
	void	close();
	void	swapField(bool index);
	bool	readField(uint32_t fnum, bool index);

	uint8_t
	readVersion()
	{
		return *myVersion++;
	}

	uint8_t
	readFrame()
	{
		return *myFrame++;
	}

	uint8_t
	readColor()
	{
		return *myColor++;
	}

	uint8_t
	readGraph()
	{
		uint8_t	v;

		if (myGraphOverride)
			v = *myGraphOverride++;
		else
			v = *myGraph++;

		return v;
	}

	void
	overrideGraph(const uint8_t* p)
	{
		myGraphOverride = p;
	}

	uint8_t
	readAudio()
	{
		return *myAudio++;
	}

	uint8_t
	peekAudio()
	{
		return *myAudio;
	}

	void
	startTimeCode()
	{
		myGraph = myTimecode;
	}


private:

	static int constexpr VERSION_DATA_OFFSET = 0;
	static int constexpr FRAME_DATA_OFFSET = 4;
	static int constexpr AUDIO_DATA_OFFSET = 7;
	static int constexpr GRAPH_DATA_OFFSET = 269;
	static int constexpr TIMECODE_DATA_OFFSET = 1229;
	static int constexpr COLOR_DATA_OFFSET = 1289;
	static int constexpr END_DATA_OFFSET = 2249;


	const uint8_t*	myAudio{nullptr};

	const uint8_t*	myGraph{nullptr};
	const uint8_t*	myGraphOverride{nullptr};

	const uint8_t*	myTimecode{nullptr};
	const uint8_t*	myColor{nullptr};
	const uint8_t*	myVersion{nullptr};
	const uint8_t*	myFrame{nullptr};

	uint8_t			myBuffer1[MVC_FIELD_SIZE];
	uint8_t			myBuffer2[MVC_FIELD_SIZE];

	std::ifstream*	myFile{nullptr};
	size_t			myFileSize{0};

};

bool
StreamReader::open(const std::string& path)
{
	close();

	myFile = new std::ifstream(path, std::ios::binary);

	if (!myFile || !*myFile)
		close();

	myFile->seekg(0, std::ios::end);
	myFileSize = static_cast<size_t>(myFile->tellg());
	myFile->seekg(0, std::ios::beg);

	return myFile ? true:false;
}

void
StreamReader::close()
{
	delete myFile;
	myFile = nullptr;
}

void
StreamReader::swapField(bool index)
{
	if (index == true)
	{
		myVersion  = myBuffer1 + VERSION_DATA_OFFSET;
		myFrame  = myBuffer1 + FRAME_DATA_OFFSET;
		myAudio = myBuffer1 + AUDIO_DATA_OFFSET;
		myGraph = myBuffer1 + GRAPH_DATA_OFFSET;
		myTimecode= myBuffer1 + TIMECODE_DATA_OFFSET;
		myColor = myBuffer1 + COLOR_DATA_OFFSET;
	}
	else
	{
		myVersion  = myBuffer2 + VERSION_DATA_OFFSET;
		myFrame  = myBuffer2 + FRAME_DATA_OFFSET;
		myAudio = myBuffer2 + AUDIO_DATA_OFFSET;
		myGraph = myBuffer2 + GRAPH_DATA_OFFSET;
		myTimecode = myBuffer2 + TIMECODE_DATA_OFFSET;
		myColor = myBuffer2 + COLOR_DATA_OFFSET;
	}
}

bool
StreamReader::readField(uint32_t fnum, bool index)
{
	bool read = false;

	if (myFile)
	{
		size_t	offset = ((fnum + 0) * MVC_FIELD_PAD_SIZE);

		if (offset + MVC_FIELD_PAD_SIZE < myFileSize)
		{
			myFile->seekg(offset);
			if (index == true)
				myFile->read((char*)myBuffer1, MVC_FIELD_SIZE);
			else
				myFile->read((char*)myBuffer2, MVC_FIELD_SIZE);

			read = true;
		}
	}

	return read;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/**
  State of current switches and joystick positions to control MovieCart
*/

class MovieInputs
{
public:

	MovieInputs()
	{
		init();
	}

	void
	init()
	{
		bw = false;
		fire = false;
		select = false;
		reset = false;

		right = false;
		left = false;
		up = false;
		down = false;
	}

	bool	bw;
	bool	fire;
	bool	select;
	bool	reset;

	bool	right;
	bool	left;
	bool	up;
	bool	down;

	void
	updateDirection(uint8_t val)
	{
		right = val & TRANSPORT_RIGHT;
		left = val & TRANSPORT_LEFT;
		up = val & TRANSPORT_UP;
		down = val & TRANSPORT_DOWN;
	}

	void updateTransport(uint8_t val)
	{
		bw = val & TRANSPORT_BW;
		fire = val & TRANSPORT_BUTTON;
		select = val & TRANSPORT_SELECT;
		reset = val & TRANSPORT_RESET;
	}

private:

	static int constexpr TRANSPORT_RIGHT	= 0x10;
	static int constexpr TRANSPORT_LEFT		= 0x08;
	static int constexpr TRANSPORT_DOWN		= 0x04;
	static int constexpr TRANSPORT_UP		= 0x02;
	static int constexpr TRANSPORT_UNUSED1	= 0x01;	// Right-2

	static int constexpr TRANSPORT_BW		= 0x10;
	static int constexpr TRANSPORT_UNUSED2	= 0x08;
	static int constexpr TRANSPORT_SELECT	= 0x04;
	static int constexpr TRANSPORT_RESET	= 0x02;
	static int constexpr TRANSPORT_BUTTON	= 0x01;

};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


/**
  Various kernel, OSD and scale definitions
  @author  Rob Bairos
*/


#define TIMECODE_HEIGHT     12
#define MAX_LEVEL           11
#define DEFAULT_LEVEL       6
#define BLANK_LINE_SIZE		(28+3+37) // 68

#define addr_kernel_48 0x800
#define addr_transport_buttons 0x880
#define addr_transport_direction 0x897
#define addr_right_line 0x94c
#define addr_left_line 0x980
#define addr_pick_continue 0x9c2
#define addr_main_start 0xa00
#define addr_aud_bank_setup 0xa0c
#define addr_tg0 0xa24
#define addr_title_again 0xa3b
#define addr_end_lines 0xa80
#define addr_end_lines_audio 0xaa1
#define addr_set_overscan_size 0xaad
#define addr_set_vblank_size 0xac3
#define addr_pick_transport 0xacc
#define addr_last_audio 0xacf
#define addr_wait_lines 0xad4
#define addr_transport_done1 0xae7
#define addr_draw_title 0xb00
#define addr_title_loop 0xb50
#define addr_black_bar 0xb52
#define addr_animate_bar1 0xb58
#define addr_animate_bar_again1 0xb5a
#define addr_animate_dex1 0xb65
#define addr_audio_bank 0xb80
#define addr_reset_loop 0xbfa

// scale adjustments, automatically generated
const uint8_t  scale0[16] = {  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8}; /* 0.0000 */
const uint8_t  scale1[16] = {  6,  6,  7,  7,  7,  7,  7,  7,  8,  8,  8,  8,  8,  8,  9,  9}; /* 0.1667 */
const uint8_t  scale2[16] = {  5,  5,  6,  6,  6,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10, 10}; /* 0.3333 */
const uint8_t  scale3[16] = {  4,  4,  5,  5,  6,  6,  7,  7,  8,  8,  9,  9, 10, 10, 11, 11}; /* 0.5000 */
const uint8_t  scale4[16] = {  3,  3,  4,  5,  5,  6,  7,  7,  8,  9,  9, 10, 11, 11, 12, 13}; /* 0.6667 */
const uint8_t  scale5[16] = {  1,  2,  3,  4,  5,  5,  6,  7,  8,  9, 10, 10, 11, 12, 13, 14}; /* 0.8333 */
const uint8_t  scale6[16] = {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15}; /* 1.0000 */
const uint8_t  scale7[16] = {  0,  0,  0,  1,  3,  4,  5,  7,  8, 10, 11, 12, 14, 15, 15, 15}; /* 1.3611 */
const uint8_t  scale8[16] = {  0,  0,  0,  0,  1,  3,  5,  7,  8, 10, 12, 14, 15, 15, 15, 15}; /* 1.7778 */
const uint8_t  scale9[16] = {  0,  0,  0,  0,  0,  2,  4,  6,  9, 11, 13, 15, 15, 15, 15, 15}; /* 2.2500 */
const uint8_t scale10[16] = {  0,  0,  0,  0,  0,  1,  3,  6,  9, 12, 14, 15, 15, 15, 15, 15}; /* 2.7778 */
const uint8_t *scales[11] = {scale0, scale1, scale2, scale3, scale4, scale5, scale6, scale7, scale8, scale9, scale10};

// lower bit is ignored anyways
const uint8_t  shiftBright[16 + MAX_LEVEL - 1] = {  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 15, 15, 15, 15};

// Compiled kernel
const unsigned char kernelROM[] = 
{
	133, 2, 185, 50, 248, 133, 27, 185, 62, 248, 133, 28, 185, 74, 248, 133,
	27, 185, 86, 248, 133, 135, 185, 98, 248, 190, 110, 248, 132, 136, 164, 135,
	132, 28, 133, 27, 134, 28, 134, 27, 164, 136, 102, 137, 176, 210, 136, 16,
	207, 96, 0, 1, 1, 1, 0, 0, 48, 48, 50, 53, 56, 48, 249, 129,
	129, 128, 248, 0, 99, 102, 102, 102, 230, 99, 140, 252, 140, 136, 112, 0,
	192, 97, 99, 102, 102, 198, 198, 198, 248, 198, 248, 0, 193, 32, 48, 24,
	24, 25, 24, 24, 24, 24, 126, 0, 249, 97, 97, 97, 97, 249, 0, 0,
	0, 0, 0, 0, 248, 128, 128, 224, 128, 248, 255, 255, 255, 255, 255, 255,
	165, 12, 10, 173, 130, 2, 42, 41, 23, 133, 129, 133, 128, 133, 128, 133,
	128, 133, 128, 133, 128, 234, 96, 173, 128, 2, 74, 74, 74, 133, 129, 133,
	128, 133, 128, 133, 128, 133, 128, 133, 128, 133, 128, 234, 96, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 133, 25, 133, 128,
	134, 32, 134, 33, 169, 243, 133, 27, 169, 207, 133, 28, 169, 251, 133, 27,
	169, 239, 162, 191, 160, 66, 133, 28, 169, 54, 133, 6, 134, 27, 169, 238,
	133, 7, 132, 6, 132, 28, 169, 182, 133, 6, 169, 114, 133, 7, 133, 42,
	162, 0, 162, 0, 169, 0, 162, 128, 133, 25, 134, 32, 134, 33, 169, 231,
	133, 27, 169, 159, 133, 28, 169, 253, 133, 27, 169, 247, 162, 223, 160, 242,
	133, 28, 169, 248, 133, 6, 134, 27, 169, 172, 133, 7, 132, 6, 132, 28,
	169, 216, 133, 6, 169, 98, 133, 7, 169, 0, 162, 0, 162, 0, 162, 0,
	133, 42, 76, 76, 249, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	120, 216, 162, 255, 154, 169, 0, 149, 0, 202, 208, 251, 169, 128, 133, 130,
	169, 251, 133, 131, 169, 1, 133, 37, 133, 38, 169, 3, 133, 4, 133, 5,
	133, 2, 162, 4, 133, 128, 202, 208, 251, 133, 128, 133, 16, 133, 17, 169,
	208, 133, 32, 169, 224, 133, 33, 133, 2, 133, 42, 165, 132, 106, 106, 133,
	6, 133, 7, 169, 85, 133, 137, 32, 0, 251, 176, 239, 169, 0, 133, 9,
	141, 129, 2, 141, 131, 2, 169, 6, 133, 4, 169, 2, 133, 5, 169, 0,
	133, 2, 133, 128, 76, 128, 250, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	133, 25, 169, 0, 133, 27, 133, 28, 133, 27, 133, 28, 133, 128, 133, 128,
	133, 128, 169, 0, 133, 16, 234, 133, 17, 169, 208, 133, 32, 169, 192, 133,
	33, 169, 0, 133, 2, 133, 42, 133, 128, 133, 25, 160, 0, 162, 28, 32,
	212, 250, 169, 2, 133, 0, 162, 3, 32, 212, 250, 169, 0, 133, 0, 169,
	2, 133, 1, 162, 37, 32, 212, 250, 162, 0, 134, 1, 32, 128, 248, 169,
	0, 76, 76, 249, 177, 130, 133, 2, 133, 128, 133, 128, 133, 25, 165, 129,
	240, 5, 198, 129, 173, 128, 20, 200, 202, 208, 233, 96, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	162, 30, 32, 82, 251, 169, 2, 133, 0, 162, 3, 32, 82, 251, 169, 0,
	133, 0, 169, 2, 133, 1, 162, 37, 32, 82, 251, 169, 0, 133, 1, 198,
	132, 165, 132, 133, 133, 160, 255, 162, 30, 32, 88, 251, 162, 54, 32, 82,
	251, 160, 11, 32, 0, 248, 169, 0, 133, 27, 133, 28, 133, 27, 133, 28,
	162, 54, 32, 82, 251, 165, 132, 133, 133, 160, 1, 162, 30, 32, 88, 251,
	56, 96, 169, 0, 133, 133, 160, 0, 132, 134, 24, 165, 133, 101, 134, 133,
	133, 133, 2, 133, 9, 202, 208, 242, 96, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0, 250, 0, 250, 0, 250,
};

// OSD labels

const uint8_t brightLabelEven[] = 
{
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 225, 48, 12, 252,
	6, 140, 231, 96, 0,
	0, 113, 48, 12, 96,
	6, 140, 192, 96, 0,
	0, 225, 49, 15, 96,
	6, 152, 195, 96, 0,
	0, 49, 48, 12, 96,
	6, 140, 231, 96, 0,
	0, 225, 48, 12, 96,
};

const uint8_t brightLabelOdd[] = 
{
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	7, 252, 126, 99, 0,
	0, 113, 48, 12, 96,
	6, 140, 192, 96, 0,
	0, 97, 49, 12, 96,
	7, 248, 223, 224, 0,
	0, 113, 49, 12, 96,
	6, 156, 195, 96, 0,
	0, 113, 48, 12, 96,
	7, 142, 127, 96, 0,
	0, 0, 0, 0, 0
};

const uint8_t volumeLabelEven[] = 
{
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 199, 192, 14, 254,
	113, 112, 99, 112, 0,
	0, 140, 192, 14, 192,
	51, 48, 99, 240, 0,
	0, 28, 192, 15, 254,
	31, 48, 99, 240, 0,
	0, 12, 192, 15, 192,
	30, 112, 119, 176, 0,
	0, 7, 252, 12, 254,
};

const uint8_t volumeLabelOdd[] = 
{
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	97, 224, 99, 112, 0,
	0, 142, 192, 14, 192,
	51, 112, 99, 112, 0,
	0, 28, 192, 15, 192,
	59, 48, 99, 240, 0,
	0, 28, 192, 15, 192,
	30, 112, 99, 176, 0,
	0, 14, 192, 13, 192,
	14, 224, 62, 48, 0,
	0, 0, 0, 0, 0
};

// Level bars
// 8 rows * 5 columns = 40

const uint8_t levelBarsEvenData[] =
{
	/**/
	0, 0, 0, 0, 0,
	3, 239, 191, 253, 247,
	247, 223, 126, 251, 224,
	2, 40, 161, 133, 20,
	20, 80, 66, 10, 32,
	3, 239, 191, 253, 247,
	247, 223, 126, 251, 224,
	0, 0, 0, 0, 0,
	/**/
	0, 0, 0, 0, 0,
	3, 239, 191, 253, 247,
	247, 223, 126, 251, 224,
	3, 40, 161, 133, 20,
	244, 80, 66, 10, 32,
	3, 239, 191, 253, 247,
	247, 223, 126, 251, 224,
	0, 0, 0, 0, 0,
	/**/
	0, 0, 0, 0, 0,
	3, 239, 191, 253, 247,
	247, 223, 126, 251, 224,
	3, 232, 161, 133, 20,
	247, 80, 66, 10, 32,
	3, 239, 191, 253, 247,
	247, 223, 126, 251, 224,
	0, 0, 0, 0, 0,
	/**/
	0, 0, 0, 0, 0,
	3, 239, 191, 253, 247,
	247, 223, 126, 251, 224,
	3, 239, 161, 133, 20,
	247, 208, 66, 10, 32,
	3, 239, 191, 253, 247,
	247, 223, 126, 251, 224,
	0, 0, 0, 0, 0,
	/**/
	0, 0, 0, 0, 0,
	3, 239, 191, 253, 247,
	247, 223, 126, 251, 224,
	3, 239, 161, 133, 20,
	247, 223, 66, 10, 32,
	3, 239, 191, 253, 247,
	247, 223, 126, 251, 224,
	0, 0, 0, 0, 0,
	/**/
	0, 0, 0, 0, 0,
	3, 239, 191, 253, 247,
	247, 223, 126, 251, 224,
	3, 239, 191, 133, 20,
	247, 223, 66, 10, 32,
	3, 239, 191, 253, 247,
	247, 223, 126, 251, 224,
	0, 0, 0, 0, 0,
	/**/
	0, 0, 0, 0, 0,
	3, 239, 191, 253, 247,
	247, 223, 126, 251, 224,
	3, 239, 191, 133, 20,
	247, 223, 126, 10, 32,
	3, 239, 191, 253, 247,
	247, 223, 126, 251, 224,
	0, 0, 0, 0, 0,
	/**/
	0, 0, 0, 0, 0,
	3, 239, 191, 253, 247,
	247, 223, 126, 251, 224,
	3, 239, 191, 253, 20,
	247, 223, 126, 10, 32,
	3, 239, 191, 253, 247,
	247, 223, 126, 251, 224,
	0, 0, 0, 0, 0,
	/**/
	0, 0, 0, 0, 0,
	3, 239, 191, 253, 247,
	247, 223, 126, 251, 224,
	3, 239, 191, 253, 20,
	247, 223, 126, 250, 32,
	3, 239, 191, 253, 247,
	247, 223, 126, 251, 224,
	0, 0, 0, 0, 0,
	/**/
	0, 0, 0, 0, 0,
	3, 239, 191, 253, 247,
	247, 223, 126, 251, 224,
	3, 239, 191, 253, 244,
	247, 223, 126, 251, 32,
	3, 239, 191, 253, 247,
	247, 223, 126, 251, 224,
	0, 0, 0, 0, 0,
	/**/
	0, 0, 0, 0, 0,
	3, 239, 191, 253, 247,
	247, 223, 126, 251, 224,
	3, 239, 191, 253, 247,
	247, 223, 126, 251, 224,
	3, 239, 191, 253, 247,
	247, 223, 126, 251, 224,
	0, 0, 0, 0, 0,

};

const uint8_t levelBarsOddData[] =
{
	/**/
	247, 223, 126, 251, 224,
	3, 239, 191, 253, 247,
	20, 80, 66, 10, 32,
	2, 40, 161, 133, 20,
	247, 223, 126, 251, 224,
	3, 239, 191, 253, 247,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	/**/
	247, 223, 126, 251, 224,
	3, 239, 191, 253, 247,
	244, 80, 66, 10, 32,
	3, 40, 161, 133, 20,
	247, 223, 126, 251, 224,
	3, 239, 191, 253, 247,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	/**/
	247, 223, 126, 251, 224,
	3, 239, 191, 253, 247,
	247, 80, 66, 10, 32,
	3, 232, 161, 133, 20,
	247, 223, 126, 251, 224,
	3, 239, 191, 253, 247,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	/**/
	247, 223, 126, 251, 224,
	3, 239, 191, 253, 247,
	247, 208, 66, 10, 32,
	3, 239, 161, 133, 20,
	247, 223, 126, 251, 224,
	3, 239, 191, 253, 247,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	/**/
	247, 223, 126, 251, 224,
	3, 239, 191, 253, 247,
	247, 223, 66, 10, 32,
	3, 239, 161, 133, 20,
	247, 223, 126, 251, 224,
	3, 239, 191, 253, 247,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	/**/
	247, 223, 126, 251, 224,
	3, 239, 191, 253, 247,
	247, 223, 66, 10, 32,
	3, 239, 191, 133, 20,
	247, 223, 126, 251, 224,
	3, 239, 191, 253, 247,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	/**/
	247, 223, 126, 251, 224,
	3, 239, 191, 253, 247,
	247, 223, 126, 10, 32,
	3, 239, 191, 133, 20,
	247, 223, 126, 251, 224,
	3, 239, 191, 253, 247,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	/**/
	247, 223, 126, 251, 224,
	3, 239, 191, 253, 247,
	247, 223, 126, 10, 32,
	3, 239, 191, 253, 20,
	247, 223, 126, 251, 224,
	3, 239, 191, 253, 247,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	/**/
	247, 223, 126, 251, 224,
	3, 239, 191, 253, 247,
	247, 223, 126, 250, 32,
	3, 239, 191, 253, 20,
	247, 223, 126, 251, 224,
	3, 239, 191, 253, 247,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	/**/
	247, 223, 126, 251, 224,
	3, 239, 191, 253, 247,
	247, 223, 126, 251, 32,
	3, 239, 191, 253, 244,
	247, 223, 126, 251, 224,
	3, 239, 191, 253, 247,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	/**/
	247, 223, 126, 251, 224,
	3, 239, 191, 253, 247,
	247, 223, 126, 251, 224,
	3, 239, 191, 253, 247,
	247, 223, 126, 251, 224,
	3, 239, 191, 253, 247,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
};

////////////////////////////////////////////////////////////////////////////////


class MovieCart
{

public:

	MovieCart();
	~MovieCart();

	bool	init(const std::string& path);
	bool	process(uint16_t address);

	uint8_t
	readROM(uint16_t address)
	{
		return myROM[address & 1023];
	}

	void
	writeROM(uint16_t address, uint8_t data)
	{
		myROM[address & 1023] = data;
	}

private:

	enum Mode
	{
		Volume,
		Bright,
		Time,
		Last = Time
	};

	enum TitleState
	{
		Display,
		Exiting,
		Stream
	};


	void	stopTitleScreen();

	void	writeColor(uint16_t address);
	void	writeAudioData(uint16_t address, uint8_t val);
	void	writeAudio(uint16_t address);
	void	writeGraph(uint16_t address);

	void	runStateMachine();

	void	fill_addr_right_line();
	void	fill_addr_left_line(bool again);
	void	fill_addr_end_lines();
	void	fill_addr_blank_lines();

	void	updateTransport();


	StreamReader	myStream;

	// data

	uint8_t			myROM[1024];


	// title screen state
	int				myTitleCycles;
	uint8_t			myTitleState;

	
	// address info
	bool 			myA7;
	bool 			myA10;
	uint8_t			myA10_Count;

	// state machine info

	uint8_t 		myState;
	bool			myPlaying;
	bool			myOdd;
	bool			myBufferIndex;


	uint8_t 		myLines;
	int32_t			myFrameNumber;	// signed

	uint8_t			myMode;
	uint8_t			myBright;
	uint8_t			myForceColor;

	// expressed in frames
	uint8_t			myDrawLevelBars;
	uint8_t			myDrawTimeCode;

	MovieInputs		myInputs;
	MovieInputs		myLastInputs;

	int8_t			mySpeed;	// signed
	uint8_t			myJoyRepeat;
	uint8_t			myDirectionValue;
	uint8_t			myButtonsValue;

	uint8_t			myVolume;
	const uint8_t*	myVolumeScale;
	uint8_t			myFirstAudioVal;

};


MovieCart::MovieCart()
{
}

MovieCart::~MovieCart()
{
}

bool
MovieCart::init(const std::string& path)
{
	memcpy(myROM, kernelROM, 1024);

	myTitleCycles = 0;
	myTitleState = TitleState::Display;

	myA7 = false;
	myA10 = false;
	myA10_Count = 0;

	myState = 3;
	myPlaying = true;
	myOdd = true;
	myBufferIndex = false;
	myFrameNumber = 1;

	myInputs.init();
	myLastInputs.init();
	mySpeed = 1;
	myJoyRepeat = 0;
	myDirectionValue = 0;
	myButtonsValue = 0;

	myLines = 0;
	myForceColor = 0;
	myDrawLevelBars = 0;
	myDrawTimeCode = 0;
	myFirstAudioVal = 0;

	myMode = Mode::Volume;
	myVolume = DEFAULT_LEVEL;
    myVolumeScale = scales[DEFAULT_LEVEL];
	myBright = DEFAULT_LEVEL;

	if (!myStream.open(path))
		return false;
    
	myStream.swapField(true);

	return true;
}

void
MovieCart::stopTitleScreen()
{
	writeROM(addr_title_loop + 0, 0x18);  // clear carry, one bit difference from 0x38 sec
}


void
MovieCart::writeColor(uint16_t address)
{
	uint8_t	v = myStream.readColor();
	v = (v & 0xf0) | shiftBright[(v & 0x0f) + myBright];

	if (myForceColor)
		v = myForceColor;
	if (myInputs.bw)
		v &= 0x0f;

	writeROM(address, v);
}

void
MovieCart::writeAudioData(uint16_t address, uint8_t val)
{
	uint8_t	v;
	v = myVolumeScale[val];
	writeROM(address, v);
}

void
MovieCart::writeAudio(uint16_t address)
{
	uint8_t	v = myStream.readAudio();
	writeAudioData(address, v);
}

void
MovieCart::writeGraph(uint16_t address)
{
	uint8_t	v = myStream.readGraph();
	writeROM(address, v);
}

void
MovieCart::updateTransport()
{
	myStream.overrideGraph(nullptr);

    
    // have to cut rate in half, to remove glitches...todo..
    {
        if (myBufferIndex == true)
        {
			uint8_t	temp = ~(myA10_Count & 0x1e) & 0x1e;

            if (temp == myDirectionValue)
				myInputs.updateDirection(temp);

			myDirectionValue = temp;
        }
        else
        {
			uint8_t temp = ~(myA10_Count & 0x17) & 0x17;

            if (temp == myButtonsValue)
				myInputs.updateTransport(temp);

            myButtonsValue = temp;
        }

		myA10_Count = 0;
    }

	if (myInputs.reset)
	{
		myFrameNumber = 1;
		myPlaying = true;
		myDrawTimeCode = OSD_FRAMES;

		goto update_stream;
	}

	uint8_t	lastMainMode = myMode;

	if (myInputs.up && !myLastInputs.up)
	{
		if (myMode == 0)
			myMode = Mode::Last;
		else
			myMode--;
	}
	else if (myInputs.down && !myLastInputs.down)
	{
		if (myMode == Mode::Last)
			myMode = 0;
		else
			myMode++;
	}

	if (myInputs.left || myInputs.right)
	{
		myJoyRepeat++;
	}
	else
	{
		myJoyRepeat = 0;
		mySpeed = 1;
	}


	if (myJoyRepeat & 16)
	{
		myJoyRepeat = 0;

		if (myInputs.left || myInputs.right)
		{
			if (myMode == Mode::Time)
			{
				myDrawTimeCode = OSD_FRAMES;
				mySpeed += 4;
				if (mySpeed < 0)
					mySpeed -= 4;
			}
			else if (myMode == Mode::Volume)
			{
				myDrawLevelBars = OSD_FRAMES;
				if (myInputs.left)
				{
					if (myVolume)
						myVolume--;
				}
				else
				{
					myVolume++;
					if (myVolume >= MAX_LEVEL)
						myVolume--;
				}
			}
			else if (myMode == Mode::Bright)
			{
				myDrawLevelBars = OSD_FRAMES;
				if (myInputs.left)
				{
					if (myBright)
						myBright--;
				}
				else
				{
					myBright++;
					if (myBright >= MAX_LEVEL)
						myBright--;
				}
			}
		}
	}


	if (myInputs.select && !myLastInputs.select)
	{
		myDrawTimeCode = OSD_FRAMES;
		myFrameNumber -= 60 * BACK_SECONDS + 1;
		goto update_stream;
	}

	if (myInputs.fire && !myLastInputs.fire)
		myPlaying = !myPlaying;

	switch (myMode)
	{
		case Mode::Time:
			if (lastMainMode != myMode)
				myDrawTimeCode = OSD_FRAMES;
			break;

		case Mode::Bright:
		case Mode::Volume:
		default:
			if (lastMainMode != myMode)
				myDrawLevelBars = OSD_FRAMES;
			break;
	}

	// just draw one
	if (myDrawLevelBars > myDrawTimeCode)
		myDrawTimeCode = 0;
	else
		myDrawLevelBars = 0;

	if (myPlaying)
		myVolumeScale = scales[myVolume];
	else
		myVolumeScale = scales[0];

	// update frame

	int8_t				step = 1;

	if (!myPlaying)  // step while paused
	{
		if (myMode == Mode::Time)
		{
			if (myInputs.right && !myLastInputs.right)
				step = 3;
			else if (myInputs.left && !myLastInputs.left)
				step = -3;
			else
				step = (myFrameNumber & 1) ? -1 : 1;
		}
		else
		{
			step = (myFrameNumber & 1) ? -1 : 1;
		}
	}
	else
	{
		if (myMode == Mode::Time)
		{
			if (myInputs.right)
				step = mySpeed;
			else if (myInputs.left)
				step = -mySpeed;
		}
		else
		{
			step = 1;
		}
	}

	myFrameNumber += step;
	if (myFrameNumber < 1)
	{
		myFrameNumber = 1;
		mySpeed = 1;
	}

update_stream:

	myLastInputs = myInputs;

}

void
MovieCart::fill_addr_right_line()
{
	writeGraph(addr_right_line +  9); // #GDATA0
	writeGraph(addr_right_line + 13); // #GDATA1
	writeGraph(addr_right_line + 17); // #GDATA2
	writeGraph(addr_right_line + 21); // #GDATA3
	writeGraph(addr_right_line + 23); // #GDATA4

	writeColor(addr_right_line + 25); // #GCOL0
	writeColor(addr_right_line + 29); // #GCOL1
	writeColor(addr_right_line + 35); // #GCOL2
	writeColor(addr_right_line + 43); // #GCOL3
	writeColor(addr_right_line + 47); // #GCOL4
}

void
MovieCart::fill_addr_left_line(bool again)
{
	writeAudio(addr_left_line + 5); // #AUD_DATA

	writeGraph(addr_left_line + 15); // #GDATA5
	writeGraph(addr_left_line + 19); // #GDATA6
	writeGraph(addr_left_line + 23); // #GDATA7
	writeGraph(addr_left_line + 27); // #GDATA8
	writeGraph(addr_left_line + 29); // #GDATA9

	writeColor(addr_left_line + 31); // #GCOL5
	writeColor(addr_left_line + 35); // #GCOL6
	writeColor(addr_left_line + 41); // #GCOL7
	writeColor(addr_left_line + 49); // #GCOL8
	writeColor(addr_left_line + 53); // #GCOL9

	writeAudio(addr_left_line + 57); // #AUD_DATA

	// addr_pick_line_end = 0x0ee;
	//		jmp right_line
	//		jmp end_lines
	if (again)
	{
		writeROM(addr_pick_continue + 1, LO_JUMP_BYTE(addr_right_line));
		writeROM(addr_pick_continue + 2, HI_JUMP_BYTE(addr_right_line));
	}
	else
	{
		writeROM(addr_pick_continue + 1, LO_JUMP_BYTE(addr_end_lines));
		writeROM(addr_pick_continue + 2, HI_JUMP_BYTE(addr_end_lines));
	}
}


void
MovieCart::fill_addr_end_lines()
{
	writeAudio(addr_end_lines_audio + 1);
	myFirstAudioVal = myStream.peekAudio();

	// normally overscan=28, vblank=37
	// todo: clicky noise..
	if (myOdd)
	{
		writeROM(addr_set_overscan_size + 1, 28);
		writeROM(addr_set_vblank_size + 1, 36);
	}
	else
	{
		writeROM(addr_set_overscan_size + 1, 29);
		writeROM(addr_set_vblank_size + 1, 37);
	}

	if (myBufferIndex == false)
	{
		writeROM(addr_pick_transport + 1, LO_JUMP_BYTE(addr_transport_direction));
		writeROM(addr_pick_transport + 2, HI_JUMP_BYTE(addr_transport_direction));
	}
	else
	{
		writeROM(addr_pick_transport + 1, LO_JUMP_BYTE(addr_transport_buttons));
		writeROM(addr_pick_transport + 2, HI_JUMP_BYTE(addr_transport_buttons));
	}

}

void
MovieCart::fill_addr_blank_lines()
{
	uint8_t	i;
	uint8_t	v;

	// version number
	myStream.readVersion();
	myStream.readVersion();
	myStream.readVersion();
	myStream.readVersion();

	// frame number
	myStream.readFrame();
	myStream.readFrame();
	v = myStream.readFrame();
	
	// make sure we're in sync with frame data
	myOdd = (v & 1);

	// 28 overscan
	// 3 vsync
	// 37 vblank
	
	if (myOdd)
	{
		writeAudioData(addr_audio_bank + 0, myFirstAudioVal);
		for (i = 1; i < (BLANK_LINE_SIZE + 1); i++)
			writeAudio(addr_audio_bank + i);
	}
	else
	{
		for (i = 0; i < (BLANK_LINE_SIZE -1); i++)
			writeAudio(addr_audio_bank + i);
	}

	writeAudio(addr_last_audio + 1);
}

void
MovieCart::runStateMachine()
{   
	switch(myState)
	{
		case 1:
			if (myA7)
			{
				if (myLines == (TIMECODE_HEIGHT-1))
				{
					if (myDrawTimeCode)
					{
						myDrawTimeCode--;
						myForceColor = COLOR_BLUE;
						myStream.startTimeCode();
					}
				}

				// label = 12, bars = 7
				if (myLines == 21)
				{
					if (myDrawLevelBars)
					{
						myDrawLevelBars--;
						myForceColor = COLOR_BLUE;

						switch (myMode)
						{
							case Mode::Time:
								myStream.overrideGraph(nullptr);
								break;

							case Mode::Bright:
								if (myOdd)
									myStream.overrideGraph(brightLabelOdd);
								else
									myStream.overrideGraph(brightLabelEven);
								break;

							case Mode::Volume:
							default:
								if (myOdd)
									myStream.overrideGraph(volumeLabelOdd);
								else
									myStream.overrideGraph(volumeLabelEven);
								break;
						}
					}
				}

				if (myLines == 7)
				{
					if (myDrawLevelBars)
					{
						uint8_t	levelValue;

						switch (myMode)
						{
							case Mode::Time:
								levelValue = 0;
								break;

							case Mode::Bright:
								levelValue = myBright;
								break;

							case Mode::Volume:
							default:
								levelValue = myVolume;
								break;
						}

						if (myOdd)
							myStream.overrideGraph(&levelBarsOddData[levelValue * 40]);
						else
							myStream.overrideGraph(&levelBarsEvenData[levelValue * 40]);
					}
				}

				fill_addr_right_line();

				myLines -= 1;
				myState = 2;
			}
			break;


		case 2:
			if (!myA7)
			{
				if (myLines >= 1)
				{
					fill_addr_left_line(1);

					myLines -= 1;
					myState = 1;
				}
				else
				{
					fill_addr_left_line(0);
					fill_addr_end_lines();

					myStream.swapField(myBufferIndex);
					myBufferIndex = !myBufferIndex;
					updateTransport();

					fill_addr_blank_lines();

					myState = 3;
				}
			}
			break;

		case 3:
			if (myA7)
			{
				// hit end? rewind just before end
				while (myFrameNumber >= 2 && !myStream.readField(myFrameNumber, myBufferIndex))
				{
					myFrameNumber -= 2;
					myJoyRepeat = 0;
				}

				myForceColor = 0;
				myLines = 191;
				myState = 1;
			}
			break;

		default:
			break;
	}       
}

bool
MovieCart::process(uint16_t address)
{

	bool a12 = (address & (1 << 12)) ? 1:0;
	bool a11 = (address & (1 << 11)) ? 1:0;

	// count a10 pulses
	bool a10i = (address & (1 << 10));
	if (a10i && !myA10)
		myA10_Count++;
	myA10 = a10i;

	// latch a7 state
	if (a11)	// a12
		myA7 = (address & (1 << 7));		// each 128

	switch(myTitleState)
	{
		case TitleState::Display:
			myTitleCycles++;
			if (myTitleCycles == TITLE_CYCLES)
			{
				stopTitleScreen();
				myTitleState = TitleState::Exiting;
				myTitleCycles = 0;
			}
			break;

		case TitleState::Exiting:
			if (myA7)
				myTitleState = TitleState::Stream;
			break;

		case TitleState::Stream:
			runStateMachine();
			break;
	}

	return a12;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeMVC::CartridgeMVC(const string& path, size_t size,
                         const string& md5, const Settings& settings,
                         size_t bsSize) 
  : Cartridge(settings, md5)
{
  myPath = path;
  myMovie = make_unique<MovieCart>();

  // not used
  mySize = 1024;
  myImage = make_unique<uInt8[]>(mySize);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeMVC::install(System& system)
{
  mySystem = &system;

  // Map all of the accesses to call peek and poke
  System::PageAccess access(this, System::PageAccessType::READWRITE);

  access.directPeekBase = nullptr;
  access.directPokeBase = nullptr;

  for(uInt16 addr = 0x1000; addr < 0x2000; addr += System::PAGE_SIZE)
    mySystem->setPageAccess(addr, access);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeMVC::reset()
{
	myMovie->init(myPath);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteBuffer& CartridgeMVC::getImage(size_t& size) const
{
  // not used
  size = mySize;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeMVC::patch(uInt16 address, uInt8 value)
{
  myMovie->writeROM(address, value);
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeMVC::peek(uInt16 address)
{
  myMovie->process(address);
  return myMovie->readROM(address);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeMVC::poke(uInt16 address, uInt8 value)
{
	return myMovie->process(address);
}

