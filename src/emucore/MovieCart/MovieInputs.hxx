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
  State of current switches and joystick positions to control MovieCart
  @author  Rob Bairos
*/

#pragma once

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

