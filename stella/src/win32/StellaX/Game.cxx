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
// Copyright (c) 2004 by Stephen Anthony
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Game.cxx,v 1.1 2004-07-10 22:25:58 stephena Exp $
//============================================================================ 

#include "Game.hxx"

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


Game::~Game( void )
{
}
