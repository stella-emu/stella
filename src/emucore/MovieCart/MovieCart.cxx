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
// Key kernel positions, automatically generated

/**
  Implementation of MovieCart.
  1K of memory is presented on the bus, but is repeated to fill the 4K image space.
  Contents are dynamically altered with streaming image and audio content as specific
  128-byte regions are entered.
  Original implementation: github.com/lodefmode/moviecart

  @author  Rob Bairos
*/

#include <string.h>
#include "KernelData.hxx"
#include "MovieCart.hxx"

#define LO_JUMP_BYTE(X) ((X) & 0xff)
#define HI_JUMP_BYTE(X) ((((X) & 0xff00) >> 8) | 0x10)

#define	COLOR_BLUE			0x9A
#define	COLOR_WHITE			0x0E

#define OSD_FRAMES			180
#define BACK_SECONDS		10

#define TITLE_CYCLES 		1000000

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

