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
// Copyright (c) 1995-2002 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: MediaSrc.hxx,v 1.6 2003-10-26 19:40:39 stephena Exp $
//============================================================================

#ifndef MEDIASOURCE_HXX
#define MEDIASOURCE_HXX

#include <string>

class MediaSource;

#include "bspf.hxx"

/**
  This class provides an interface for accessing graphics and audio data.

  @author  Bradford W. Mott
  @version $Id: MediaSrc.hxx,v 1.6 2003-10-26 19:40:39 stephena Exp $
*/
class MediaSource
{
  public:
    /**
      Create a new media source
    */
    MediaSource();
 
    /**
      Destructor
    */
    virtual ~MediaSource();

  public:
    /**
      This method should be called at an interval corresponding to the 
      desired frame rate to update the media source.  Invoking this method
      will update the graphics buffer and generate the corresponding audio
      samples.
    */
    virtual void update() = 0;

    /**
      Answers the current frame buffer

      @return Pointer to the current frame buffer
    */
    virtual uInt8* currentFrameBuffer() const = 0;

    /**
      Answers the previous frame buffer

      @return Pointer to the previous frame buffer
    */
    virtual uInt8* previousFrameBuffer() const = 0;

  public:
    /**
      Get the palette which maps frame data to RGB values.

      @return Array of integers which represent the palette (RGB)
    */
    virtual const uInt32* palette() const = 0;

    /**
      Answers the height of the frame buffer

      @return The frame's height
    */
    virtual uInt32 height() const = 0;

    /**
      Answers the width of the frame buffer

      @return The frame's width
    */
    virtual uInt32 width() const = 0;

  public:
    /**
      Answers the total number of scanlines the media source generated
      in producing the current frame buffer.

      @return The total number of scanlines generated
    */
    virtual uInt32 scanlines() const = 0;

  public:
    /**
      Enumeration of the possible audio sample types.
    */
    enum AudioSampleType
    {
      UNSIGNED_8BIT_MONO_AUDIO
    };

    /**
      Dequeues all of the samples in the audio sample queue.
    */
    virtual void clearAudioSamples() = 0;

    /**
      Dequeues up to the specified number of samples from the audio sample
      queue into the buffer.  If the requested number of samples are not
      available then all of samples are dequeued.  The method returns the
      actual number of samples removed from the queue.

      @return The actual number of samples which were dequeued.
    */
    virtual uInt32 dequeueAudioSamples(uInt8* buffer, int size) = 0;

    /**
      Answers the number of samples currently available in the audio
      sample queue.

      @return The number of samples in the audio sample queue.
    */ 
    virtual uInt32 numberOfAudioSamples() const = 0;

    /**
      Returns the type of audio samples which are being stored in the audio
      sample queue.  Currently, only unsigned 8-bit audio samples are created,
      however, in the future this will be extended to support stereo samples.

      @return The type of audio sample stored in the sample queue.
    */
    virtual AudioSampleType typeOfAudioSamples() const = 0;

  private:
    // Copy constructor isn't supported by this class so make it private
    MediaSource(const MediaSource&);

    // Assignment operator isn't supported by this class so make it private
    MediaSource& operator = (const MediaSource&);
};
#endif

