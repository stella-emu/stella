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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#import <Cocoa/Cocoa.h>

#include "MacOSUtils.hxx"

namespace MacOSUtils
{

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string applicationSupportPath()
{
  @autoreleasepool {
    NSArray* paths = NSSearchPathForDirectoriesInDomains(
        NSApplicationSupportDirectory, NSUserDomainMask, YES);
    if(paths.count > 0)
    {
      const char* s = [paths[0] UTF8String];
      if(s) return std::string{s};
    }
  }
  return {};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string desktopPath()
{
  @autoreleasepool {
    NSArray* paths = NSSearchPathForDirectoriesInDomains(
        NSDesktopDirectory, NSUserDomainMask, YES);
    if(paths.count > 0)
    {
      const char* s = [paths[0] UTF8String];
      if(s) return std::string{s};
    }
  }
  return {};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace {

// Anchor the Metal view, and every layer-backed ancestor, to one corner.
// The view-level placement is enough: AppKit propagates it to the backing
// layer's contentsGravity.  Ancestors need it too, or they rescale their own
// contents and undo it.
void anchorViews(NSView* content, NSInteger metalViewTag,
                 bool anchorLeft, bool anchorTop)
{
  NSView* const metalView = [content viewWithTag:metalViewTag];
  if(metalView == nil)
    return;  // not an SDL Metal window

  const NSViewLayerContentsPlacement placement = anchorLeft
      ? (anchorTop ? NSViewLayerContentsPlacementTopLeft
                   : NSViewLayerContentsPlacementBottomLeft)
      : (anchorTop ? NSViewLayerContentsPlacementTopRight
                   : NSViewLayerContentsPlacementBottomRight);

  for(NSView* v = metalView; v != nil; v = v.superview)
    v.layerContentsPlacement = placement;
}

}  // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void enableMetalLiveResizeAnchoring(void* nsWindow, long metalViewTag)
{
  // One pair of observers covers every window, so repeat calls add nothing
  static dispatch_once_t once;
  dispatch_once(&once, ^{
    const NSInteger tag = static_cast<NSInteger>(metalViewTag);
    NSNotificationCenter* const center = [NSNotificationCenter defaultCenter];

    // Previous step's frame, to see which edges are moving; only one window
    // can be live-resized at a time
    static NSRect lastFrame;
    // Anchor in effect, so a drag only touches the views when it changes
    static bool haveAnchor, lastLeft, lastTop;

    [center addObserverForName:NSWindowWillStartLiveResizeNotification
                        object:nil   // any window
                         queue:nil   // synchronously, on the posting thread
                    usingBlock:^(NSNotification* note) {
      lastFrame = [(NSWindow*)note.object frame];
      haveAnchor = false;
    }];

    [center addObserverForName:NSWindowDidResizeNotification
                        object:nil
                         queue:nil
                    usingBlock:^(NSNotification* note) {
      NSWindow* const window = note.object;
      if(!window.inLiveResize)
        return;  // programmatic resize; AppKit is not stretching anything

      // A dragged edge moves while its opposite stays put, so the frame delta
      // names it exactly.  Cocoa frames are bottom-left based: a moving
      // origin.x means the left edge, origin.y the bottom.  (The pointer is no
      // use here -- during a drag it sits outside the content view.)
      const NSRect frame = window.frame;
      const NSRect prev = lastFrame;
      lastFrame = frame;

      const bool draggingLeft = frame.origin.x != prev.origin.x;
      const bool draggingBottom = frame.origin.y != prev.origin.y;

      // A step that moved nothing names no edge; wait for one that does
      if(!draggingLeft && !draggingBottom &&
         frame.size.width == prev.size.width &&
         frame.size.height == prev.size.height)
        return;

      // Anchor to the edges holding still: pinning to a moving edge drags the
      // stale frame along, pushing the far side of the UI out of the window
      const bool anchorLeft = !draggingLeft;
      const bool anchorTop = draggingBottom;
      if(haveAnchor && anchorLeft == lastLeft && anchorTop == lastTop)
        return;

      haveAnchor = true;
      lastLeft = anchorLeft;
      lastTop = anchorTop;
      anchorViews(window.contentView, tag, anchorLeft, anchorTop);
    }];
  });

  // Note: lowering the layer's maximumDrawableCount was tried and is worse
  // both ways -- fullscreen emulation stutters, and the contents trail the
  // window further, because a shorter queue makes nextDrawable block on the
  // main thread, which is where the resize loop runs.  Do not do it.
}

}  // namespace MacOSUtils
