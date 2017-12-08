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
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef LINKED_OBJECT_POOL_HXX
#define LINKED_OBJECT_POOL_HXX

#include <list>
#include "bspf.hxx"

/**
  A fixed-size object-pool based doubly-linked list that makes use of
  multiple STL lists, to reduce frequent (de)allocations.

  This structure can be used as either a stack or queue, but also allows
  for removal at any location in the list.

  There are two internal lists; one stores active nodes, and the other
  stores pool nodes that have been 'deleted' from the active list (note
  that no actual deletion takes place; nodes are simply moved from one list
  to another).  Similarly, when a new node is added to the active list, it
  is simply moved from the pool list to the active list.

  In all cases, the variable 'myCurrent' is updated to point to the
  current node.

  NOTE: You must always call 'currentIsValid()' before calling 'current()',
        to make sure that the return value is a valid reference.

        In the case of methods which wrap the C++ 'splice()' method, the
        semantics of splice are followed wrt invalid/out-of-range/etc
        iterators.  See the applicable C++ STL documentation for such
        behaviour.

  @author Stephen Anthony
*/
namespace Common {

template <class T, uInt32 CAPACITY = 100>
class LinkedObjectPool
{
  public:
    using iter = typename std::list<T>::iterator;
    using const_iter = typename std::list<T>::const_iterator;

    /*
      Create a pool of size CAPACITY; the active list starts out empty.
    */
    LinkedObjectPool<T, CAPACITY>() : myCurrent(myList.end()) {
      for(uInt32 i = 0; i < CAPACITY; ++i)
        myPool.emplace_back(T());
    }

    /**
      Return node data that the 'current' iterator points to.
      Note that this returns a valid value only in the case where the list
      is non-empty (at least one node has been added to the active list).

      Make sure to call 'currentIsValid()' before accessing this method.
    */
    T& current() { return *myCurrent; }

    /**
      Does the 'current' iterator point to a valid node in the active list?
      This must be called before 'current()' is called.
    */
    bool currentIsValid() { return myCurrent != myList.end(); }

    /**
      Advance 'current' iterator to previous position in the active list.
      If we go past the beginning, it is reset to one past the end (indicates nullptr).
    */
    void moveToPrevious() {
      if(currentIsValid())
        myCurrent = myCurrent == myList.begin() ? myList.end() : std::prev(myCurrent, 1);
    }

    /**
      Advance 'current' iterator to next position in the active list.
      If we go past the last node, it will point to one past the end (indicates nullptr).
    */
    void moveToNext() {
      if(currentIsValid())
        myCurrent = std::next(myCurrent, 1);
    }

    /**
      Return an iterator to the first node in the active list.
    */
    iter first() { return myList.begin(); }

    /**
      Return an iterator to the last node in the active list.
    */
    iter last() { return std::prev(myList.end(), 1); }

    /**
      Add a new node at the beginning of the active list, and update 'current'
      to point to that node.
    */
    void addFirst() {
      myList.splice(myList.begin(), myPool, myPool.begin());
      myCurrent = myList.begin();
    }

    /**
      Add a new node at the end of the active list, and update 'current'
      to point to that node.
    */
    void addLast() {
      myList.splice(myList.end(), myPool, myPool.begin());
      myCurrent = std::prev(myList.end(), 1);
    }

    /**
      Remove the first node of the active list, updating 'current' if it
      happens to be the one removed.
    */
    void removeFirst() {
      const_iter i = myList.begin();
      myPool.splice(myPool.end(), myList, i);
      if(myCurrent == i)  // did we just invalidate 'current'
        moveToNext();     // if so, move to the next node
    }

    /**
      Remove the last node of the active list, updating 'current' if it
      happens to be the one removed.
    */
    void removeLast() {
      const_iter i = std::prev(myList.end(), 1);
      myPool.splice(myPool.end(), myList, i);
      if(myCurrent == i)  // did we just invalidate 'current'
        moveToPrevious(); // if so, move to the previous node
    }

#if 0
    /**
      Convenience method to remove a single element from the active list at
      position of the iterator +- the offset.
    */
    void remove(const_iter i, Int32 offset = 0) {
      myPool.splice(myPool.end(), myList,
                    offset >= 0 ? std::next(i, offset) : std::prev(i, -offset));
    }

    /**
      Convenience method to remove a single element from the active list by
      index, offset from the beginning of the list. (ie, '0' means first
      element, '1' is second, and so on).
    */
    void remove(uInt32 index) {
      myPool.splice(myPool.end(), myList, std::next(myList.begin(), index));
    }
#endif

    /**
      Remove range of elements from the beginning of the active list to
      the 'current' node.
    */
    void removeToFirst() {
      myPool.splice(myPool.end(), myList, myList.begin(), myCurrent);
    }

    /**
      Remove range of elements from the node after 'current' to the end of the
      active list.
    */
    void removeToLast() {
      myPool.splice(myPool.end(), myList, std::next(myCurrent, 1), myList.end());
    }

    /**
      Erase entire contents of active list.
    */
    void clear() {
      myPool.splice(myPool.end(), myList, myList.begin(), myList.end());
      myCurrent = myList.end();
    }

#if 0
    /** Access the list with iterators, just as you would a normal C++ STL list */
    iter begin() { return myList.begin(); }
    iter end()   { return myList.end();   }
    const_iter begin() const { return myList.cbegin(); }
    const_iter end() const   { return myList.cend();   }
#endif
    uInt32 capacity() const { return CAPACITY; }

    uInt32 size() const { return myList.size();             }
    bool empty() const  { return myList.size() == 0;        }
    bool full() const   { return myList.size() >= CAPACITY; }

  private:
    std::list<T> myList, myPool;

    // Current position in the active list (end() indicates an invalid position)
    iter myCurrent;

  private:
    // Following constructors and assignment operators not supported
    LinkedObjectPool(const LinkedObjectPool&) = delete;
    LinkedObjectPool(LinkedObjectPool&&) = delete;
    LinkedObjectPool& operator=(const LinkedObjectPool&) = delete;
    LinkedObjectPool& operator=(LinkedObjectPool&&) = delete;
};

}  // Namespace Common

#endif
