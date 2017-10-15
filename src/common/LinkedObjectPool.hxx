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
 * A fixed-size object-pool based doubly-linked list that makes use of
 * multiple STL lists, to reduce frequent (de)allocations.
 *
 * This structure can be used as either a stack or queue, but also allows
 * for removal at any location in the list.
 *
 * There are two internal lists; one stores active nodes, and the other
 * stores pool nodes that have been 'deleted' from the active list (note
 * that no actual deletion takes place; nodes are simply moved from one list
 * to another).  Similarly, when a new node is added to the active list, it
 * is simply moved from the pool list to the active list.
 *
 * NOTE: For efficiency reasons, none of the methods check for overflow or
 *       underflow; that is the responsibility of the caller.
 *
 *       In the case of methods which wrap the C++ 'splice()' method, the
 *       semantics of splice are followed wrt invalid/out-of-range/etc
 *       iterators.  See the applicable documentation for such behaviour.
 *
 * @author Stephen Anthony
 */
namespace Common {

template <class T, uInt32 CAPACITY = 100>
class LinkedObjectPool
{
  public:
    using iter = typename std::list<T>::iterator;
    using const_iter = typename std::list<T>::const_iterator;

    /*
     * Create a pool of size CAPACITY; the active list starts out empty.
     */
    LinkedObjectPool<T, CAPACITY>() {
      for(uInt32 i = 0; i < CAPACITY; ++i)
        myPool.emplace_back(T());
    }

    /**
     * Return a reference to the element at the first node in the active list.
     */
    T& first() { return myList.front(); }

    /**
     * Return a reference to the element at the last node in the active list.
     */
    T& last() { return myList.back(); }

    /**
     * Add a new node at beginning of the active list, and return a reference
     * to that nodes' data. The reference may then be modified; ie, you're
     * able to change the data located at that node.
     */
    T& addFirst() {
      myList.splice(myList.begin(), myPool, myPool.begin());
      return myList.front();
    }

    /**
     * Add a new node at the end of the active list, and return a reference
     * to that nodes' data. The reference may then be modified; ie, you're
     * able to change the data located at that node.
     */
    T& addLast() {
      myList.splice(myList.end(), myPool, myPool.begin());
      return myList.back();
    }

    /**
     * Remove the first node of the active list.
     */
    void removeFirst() {
      myPool.splice(myPool.end(), myList, myList.begin());
    }

    /**
     * Remove the last node of the active list.
     */
    void removeLast() {
      myPool.splice(myPool.end(), myList, std::prev(myList.end(), 1));
    }

    /**
     * Convenience method to remove a single element from the active list at
     * position of the iterator +- the offset.
     */
    void remove(const_iter i, Int32 offset = 0) {
      myPool.splice(myPool.end(), myList,
                    offset >= 0 ? std::next(i, offset) : std::prev(i, -offset));
    }

    /**
     * Convenience method to remove a single element from the active list by
     * index, offset from the beginning of the list. (ie, '0' means first
     * element, '1' is second, and so on).
     */
    void remove(uInt32 index) {
      myPool.splice(myPool.end(), myList, std::next(myList.begin(), index));
    }

    /**
     * Convenience method to remove a range of elements from 'index' to the
     * end of the active list.
     */
    void removeLast(uInt32 index) {
      myPool.splice(myPool.end(), myList, std::next(myList.begin(), index), myList.end());
    }

    /**
     * Erase entire contents of active list.
     */
    void clear() {
      myPool.splice(myPool.end(), myList, myList.begin(), myList.end());
    }

    /** Access the list with iterators, just as you would a normal C++ STL list */
    iter begin() { return myList.begin(); }
    iter end()   { return myList.end();   }
    const_iter begin() const { return myList.cbegin(); }
    const_iter end() const   { return myList.cend();   }

    uInt32 size() const { return myList.size();             }
    bool empty() const  { return myList.size() == 0;        }
    bool full() const   { return myList.size() >= CAPACITY; }

  private:
    std::list<T> myList, myPool;

  private:
    // Following constructors and assignment operators not supported
    LinkedObjectPool(const LinkedObjectPool&) = delete;
    LinkedObjectPool(LinkedObjectPool&&) = delete;
    LinkedObjectPool& operator=(const LinkedObjectPool&) = delete;
    LinkedObjectPool& operator=(LinkedObjectPool&&) = delete;
};

}  // Namespace Common

#endif
