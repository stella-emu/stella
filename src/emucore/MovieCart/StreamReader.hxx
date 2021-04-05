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
  Simulate retrieval 512 byte chunks from a serial source
  @author  Rob Bairos
*/

#pragma once

#include <stdint.h>
#include <fstream>
#include <string>

#define MVC_FIELD_SIZE		 2560 // round field to nearest 512 byte boundary
#define MVC_FIELD_PAD_SIZE   4096 // round to nearest 4K 

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

