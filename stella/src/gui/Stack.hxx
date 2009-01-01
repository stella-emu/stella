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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Stack.hxx,v 1.7 2009-01-01 18:13:39 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef STACK_HXX
#define STACK_HXX

#include <assert.h>

/**
 * Extremly simple fixed size stack class.
 */
template <class T, int MAX_SIZE = 10>
class FixedStack
{
  public:
    FixedStack<T, MAX_SIZE>() : _size(0) {}
	
    bool empty() const { return _size <= 0; }
    void clear() { _size = 0; }
    void push(const T& x)
    {
      assert(_size < MAX_SIZE);
      _stack[_size++] = x;
    }
    T top() const
    {
      if(_size > 0)
        return _stack[_size - 1];
      else
        return 0;
    }
    T pop()
    {
      T tmp;
      assert(_size > 0);
      tmp = _stack[_size - 1];
      _stack[--_size] = 0;
      return tmp;
    }
    int size() const { return _size; }
    T operator [](int i) const
    {
      assert(0 <= i && i < MAX_SIZE);
      return _stack[i];
    }

  protected:
    T   _stack[MAX_SIZE];
    int _size;
};

#endif
