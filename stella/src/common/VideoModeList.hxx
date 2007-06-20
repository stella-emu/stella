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
// Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: VideoModeList.hxx,v 1.1 2007-06-20 16:33:22 stephena Exp $
//============================================================================

#ifndef VIDMODE_LIST_HXX
#define VIDMODE_LIST_HXX

#include "Array.hxx"
#include "bspf.hxx"

struct VideoMode {
  uInt32 image_x, image_y, image_w, image_h;
  uInt32 screen_w, screen_h;
  uInt32 zoom;
  string name;
};

/**
  This class implements an iterator around an array of VideoMode objects.

  @author  Stephen Anthony
  @version $Id: VideoModeList.hxx,v 1.1 2007-06-20 16:33:22 stephena Exp $
*/
class VideoModeList
{
  public:
    VideoModeList() : myIdx(-1) { }

    void add(VideoMode mode) { myModeList.push_back(mode); }

    void clear() { myModeList.clear(); }

    bool isEmpty() const { return myModeList.isEmpty(); }

    uInt32 size() const { return myModeList.size(); }

    const VideoMode& previous()
    {
      --myIdx;
      if(myIdx < 0) myIdx = myModeList.size() - 1;
      return current();
    }

    const VideoMode& current() const
    {
      return myModeList[myIdx];
    }

    const VideoMode& next()
    {
      myIdx = (myIdx + 1) % myModeList.size();
      return current();
    }

    void setByResolution(uInt32 width, uInt32 height)
    {
      // Find the largest resolution within the given bounds
      myIdx = 0;
      for(unsigned int i = myModeList.size() - 1; i; --i)
      {
        if(myModeList[i].screen_w <= width && myModeList[i].screen_h <= height)
        {
          myIdx = i;
          break;
        }
      }
    }

    void setByZoom(uInt32 zoom)
    {
      // Find the largest zoom within the given bounds
      myIdx = 0;
      for(unsigned int i = myModeList.size() - 1; i; --i)
      {
        if(myModeList[i].zoom <= zoom)
        {
          myIdx = i;
          break;
        }
      }
    }

    static bool modesAreEqual(const VideoMode& m1, const VideoMode& m2)
    {
      return (m1.image_x == m2.image_x) &&
             (m1.image_y == m2.image_y) &&
             (m1.image_w == m2.image_w) &&
             (m1.image_h == m2.image_h) &&
             (m1.screen_w == m2.screen_w) &&
             (m1.screen_h == m2.screen_h) &&
             (m1.zoom == m2.zoom) &&
             (m1.name == m2.name);
    }

  private:
    Common::Array<VideoMode> myModeList;
    int myIdx;
};

#endif
