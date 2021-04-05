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
  Simulate retrieval of 512 byte chunks from a serial source
  @author  Rob Bairos
*/

#include "StreamReader.hxx"

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

