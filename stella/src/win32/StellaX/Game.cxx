//============================================================================
//
//   SSSS    tt          lll  lll          XX     XX
//  SS  SS   tt           ll   ll           XX   XX
//  SS     tttttt  eeee   ll   ll   aaaa     XX XX
//   SSSS    tt   ee  ee  ll   ll      aa     XXX
//      SS   tt   eeeeee  ll   ll   aaaaa    XX XX
//  SS  SS   tt   ee      ll   ll  aa  aa   XX   XX
//   SSSS     ttt  eeeee llll llll  aaaaa  XX     XX
//
// Copyright (c) 1995-2000 by Jeff Miller
// Copyright (c) 2004 by Stephen Anthony
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Game.cxx,v 1.2 2004-07-15 03:03:27 stephena Exp $
//============================================================================

#include "Game.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
Game::Game( void )
{
  _available    = false;
  _rom          = " ";
  _md5          = " ";
  _name         = " ";
  _rarity       = " ";
  _manufacturer = " ";
  _note         = " ";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
Game::~Game( void )
{
}
