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

#ifndef LINKED_OBJECT_POOL_HXX
#define LINKED_OBJECT_POOL_HXX

#include <cassert>

#include "bspf.hxx"

/**
  A fixed-size object-pool based list (based on a vector) that makes
  no further allocations after being resized.

  @author Stephen Anthony
*/
namespace Common {

template <typename T>
class LinkedObjectPool
{
    static constexpr uInt32 npos = std::numeric_limits<uInt32>::max();
    static constexpr uInt32 DEFAULT_CAPACITY = 100;

    struct Node {
      T value{};
      uInt32 prev{npos};
      uInt32 next{npos};
      bool active{false};
    };

  public:
    // =========================
    // Iterator (STL-like)
    // =========================
    class iter {
      public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = T*;
        using reference         = T&;

        iter() : pool{nullptr}, idx{npos} { }
        iter(LinkedObjectPool* p, uInt32 i) : pool{p}, idx{i} { }

        reference operator*() const { return pool->myNodes[idx].value;  }
        pointer operator->() const  { return &pool->myNodes[idx].value; }

        iter& operator++() {
          idx = (idx == npos) ? npos : pool->myNodes[idx].next;
          return *this;
        }
        iter& operator--() {
          if(idx == npos) idx = pool->myTail;
          else            idx = pool->myNodes[idx].prev;
          return *this;
        }
        iter operator++(int) { iter tmp = *this; ++(*this); return tmp; }
        iter operator--(int) { iter tmp = *this; --(*this); return tmp; }

        bool operator==(const iter& o) const { return idx == o.idx; }
        bool operator!=(const iter& o) const { return idx != o.idx; }
        uInt32 index() const { return idx; }

      private:
        LinkedObjectPool* pool;  // non-const pointer — allows mutation
        uInt32 idx;
    };
    class const_iter {
      public:
        // Required type aliases for the concept:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const T*;
        using reference         = const T&;

        const_iter() : pool{nullptr}, idx{npos} { }
        const_iter(const LinkedObjectPool* p, uInt32 i)
          : pool{p}, idx{i} { }

        reference operator*() const { return pool->myNodes[idx].value; }
        pointer operator->() const { return &pool->myNodes[idx].value; }

        const_iter& operator++() {
          idx = (idx == npos) ? npos : pool->myNodes[idx].next;
          return *this;
        }
        const_iter& operator--() {
          if(idx == npos) idx = pool->myTail;
          else            idx = pool->myNodes[idx].prev;
          return *this;
        }
        const_iter operator++(int) { const_iter tmp = *this; ++(*this); return tmp; }
        const_iter operator--(int) { const_iter tmp = *this; --(*this); return tmp; }

        bool operator==(const const_iter& o) const { return idx == o.idx; }
        bool operator!=(const const_iter& o) const { return idx != o.idx; }

        uInt32 index() const { return idx; }

      private:
        const LinkedObjectPool* pool;
        uInt32 idx;
      };

  public:
    /*
      Create a pool of default size.
    */
    LinkedObjectPool() { resize(DEFAULT_CAPACITY); }
    ~LinkedObjectPool() = default;

    /**
      Return node data that the 'current' iterator points to.
      Note that this returns a valid value only in the case where the list
      is non-empty (at least one node has been added).

      Make sure to call 'currentIsValid()' before accessing this method.
    */
    T& current() {
      assert(currentIsValid());
      return myNodes[myCurrentIdx].value;
    }
    const T& current() const {
      assert(currentIsValid());
      return myNodes[myCurrentIdx].value;
    }

    /**
      Does the 'current' iterator point to a valid node in the list?
      This must be called before 'current()' is called.
    */
    [[nodiscard]] bool currentIsValid() const { return myCurrentIdx != npos; }

    /**
      Returns current's position in the list.
    */
    uInt32 currentIdx() const {
      return currentIsValid() ? myCurrentPos : 0;
    }

    /**
      Advance 'current' iterator to previous position in the list.
    */
    void moveToPrevious() {
      if(currentIsValid() && myNodes[myCurrentIdx].prev != npos) {
          myCurrentIdx = myNodes[myCurrentIdx].prev;
          --myCurrentPos;
      }
    }

    /**
      Advance 'current' iterator to next position in the list.
    */
    void moveToNext() {
      if(currentIsValid() && myNodes[myCurrentIdx].next != npos) {
        myCurrentIdx = myNodes[myCurrentIdx].next;
        ++myCurrentPos;
      }
    }

    /**
      Advance 'current' iterator to first position in the list.
    */
    void moveToFirst() { myCurrentIdx = myHead; myCurrentPos = 1; }

    /**
      Advance 'current' iterator to last position in the list.
    */
    void moveToLast() { myCurrentIdx = myTail; myCurrentPos = mySize; }

    /**
      Return an iterator to the specified node in the list.
    */
    const_iter first() const { return const_iter(this, myHead); }
    const_iter last()  const { return const_iter(this, myTail); }
    const_iter previous(const_iter i) const {
      return const_iter(this, (i.index() == npos) ? npos : myNodes[i.index()].prev);
    }
    const_iter next(const_iter i) const {
      return const_iter(this, (i.index() == npos) ? npos : myNodes[i.index()].next);
    }

    /**
      Canonical iterators from C++ STL.
    */
    iter begin() { return iter(this, myHead); }
    iter end()   { return iter(this, npos);   }
    const_iter begin()  const { return const_iter(this, myHead); }
    const_iter end()    const { return const_iter(this, npos);   }
    const_iter cbegin() const { return const_iter(this, myHead); }
    const_iter cend()   const { return const_iter(this, npos);   }

    /**
      Answer whether 'current' is at the specified position.
    */
    bool atFirst() const { return myCurrentIdx == myHead; }
    bool atLast()  const { return myCurrentIdx == myTail; }

    /**
      Add a new node at the beginning of the list, and update 'current'
      to point to that node.
    */
    void addFirst() {
      if(full()) return;

      uInt32 n = alloc();

      Node& nd = myNodes[n];
      nd.active = true;

      nd.prev = npos;
      nd.next = myHead;

      if(myHead != npos) myNodes[myHead].prev = n;
      myHead = n;

      if(myTail == npos) myTail = n;

      myCurrentIdx = n;
      myCurrentPos = 1;  // head is always position 1
      ++mySize;
    }

    /**
      Add a new node at the end of the list, and update 'current'
      to point to that node.
    */
    void addLast() {
      if(full()) return;

      const uInt32 n = alloc();

      Node& nd = myNodes[n];
      nd.active = true;

      nd.prev = myTail;
      nd.next = npos;

      if(myTail != npos) myNodes[myTail].next = n;
      myTail = n;

      if(myHead == npos) myHead = n;

      myCurrentIdx = n;
      myCurrentPos = mySize + 1;  // appended to end
      ++mySize;
    }

    /**
      Remove the specified node of the list, updating 'current' if it
      happens to be the one removed.
    */
    void removeFirst() {
      if(myHead != npos)
        removeNode(myHead);
    }
    void removeLast() {
      if(myTail != npos)
        removeNode(myTail);
    }

    /**
      Remove a single element from the list at position of the iterator.
    */
    void remove(const_iter i) {
      if(i.index() != npos)
        removeNode(i.index());
    }

    /**
      Remove a single element from the active list by index, offset from
      the beginning of the list. (ie, '0' means first element, '1' is second,
      and so on).
    */
    void remove(uInt32 index) {
      if(index >= mySize) return;

      uInt32 it = myHead;
      while(index--)
        it = myNodes[it].next;

      removeNode(it);
    }

    /**
      Remove range of elements from the beginning of the list to
      the 'current' node.
    */
    void removeToFirst() {
      while(myHead != npos && myHead != myCurrentIdx)
        removeNode(myHead);
    }

    /**
      Remove range of elements from the node after 'current' to the end of the list.
    */
    void removeToLast() {
      while(myTail != npos && myTail != myCurrentIdx)
        removeNode(myTail);
    }

    /**
      Resize the pool to specified size, invalidating the list in the process
      (ie, the list essentially becomes empty again).
    */
    void resize(uInt32 capacity) {
      if(capacity == myCapacity) return;
      assert(mySize == 0 && "resize() discards all list contents — call clear() first");

      myCapacity = capacity;
      myNodes.resize(myCapacity);
      clear();
    }

    /**
      Erase entire contents of active list.
    */
    void clear() {
      for(uInt32 i = 0; i < myCapacity; ++i) {
        myNodes[i].next = i + 1;
        myNodes[i].active = false;
      }

      myNodes[myCapacity - 1].next = npos;

      myFreeHead = 0;
      myHead = myTail = myCurrentIdx = npos;
      myCurrentPos = mySize = 0;
    }

    [[nodiscard]] uInt32 capacity() const { return myCapacity; }
    [[nodiscard]] uInt32 size() const { return mySize; }
    [[nodiscard]] bool empty() const { return mySize == 0; }
    [[nodiscard]] bool full()  const { return mySize == myCapacity; }

  #if 0
    friend std::ostream& operator<<(std::ostream& os, const LinkedObjectPool<T>& p) {
      for(const auto& i: p.myList)
        os << i << (p.current() == i ? "* " : "  ");
      return os;
    }
  #endif

  private:
    uInt32 alloc() {
      assert(myFreeHead != npos && "alloc() called on full pool");
      const uInt32 n = myFreeHead;
      myFreeHead = myNodes[n].next;
      return n;
    }

    void freeNode(uInt32 n) {
      myNodes[n].next = myFreeHead;
      myNodes[n].active = false;
      myFreeHead = n;
    }

    void removeNode(uInt32 n) {
      const bool removingCurrent = (myCurrentIdx == n);

      const uInt32 p  = myNodes[n].prev;
      const uInt32 nx = myNodes[n].next;

      // Compute ordinal position of n before unlinking
      uInt32 removedPos = 1;
      uInt32 it = myHead;
      while(it != npos && it != n) {
        ++removedPos;
        it = myNodes[it].next;
      }

      if(p != npos) myNodes[p].next = nx;
      else          myHead = nx;

      if(nx != npos) myNodes[nx].prev = p;
      else           myTail = p;

      freeNode(n);

      if(removingCurrent) {
        myCurrentIdx = nx;
        if(nx == npos && myCurrentPos > 1)
          --myCurrentPos;
      }
      else if(removedPos < myCurrentPos)
        --myCurrentPos;

      --mySize;
    }

  private:
    std::vector<Node> myNodes;

    uInt32 myHead{npos};
    uInt32 myTail{npos};
    uInt32 myCurrentIdx{npos};
    uInt32 myCurrentPos{0};

    uInt32 myFreeHead{npos};

    uInt32 mySize{0};
    uInt32 myCapacity{0};

  private:
    // Following constructors and assignment operators not supported
    LinkedObjectPool(const LinkedObjectPool&) = delete;
    LinkedObjectPool(LinkedObjectPool&&) = delete;
    LinkedObjectPool& operator=(const LinkedObjectPool&) = delete;
    LinkedObjectPool& operator=(LinkedObjectPool&&) = delete;
};

static_assert(std::bidirectional_iterator<LinkedObjectPool<int>::iter>);
static_assert(std::bidirectional_iterator<LinkedObjectPool<int>::const_iter>);

} // namespace Common

#endif
