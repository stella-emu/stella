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

#pragma once

#include <string>
#include "StreamReader.hxx"
#include "MovieInputs.hxx"

class MovieCart
{

public:

	MovieCart()
	{
	}

	~MovieCart()
	{
	}

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


