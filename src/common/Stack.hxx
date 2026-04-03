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

#ifndef STACK_HXX
#define STACK_HXX

#include "bspf.hxx"

/**
 * Simple fixed size stack class.
 */
namespace Common {

template <typename T, size_t CAPACITY = 50>
class FixedStack
{
  private:
    std::array<T, CAPACITY> _stack{};
    size_t _size{0};

  public:
    constexpr FixedStack() = default;
    ~FixedStack() = default;

    constexpr bool empty() const { return _size == 0; }
    constexpr bool full()  const { return _size == CAPACITY; }
    constexpr size_t size() const { return _size; }
    static constexpr size_t capacity() { return CAPACITY; }

    constexpr T& top() {
      assert(_size > 0);
      return _stack[_size - 1];
    }
    constexpr const T& top() const {
      assert(_size > 0);
      return _stack[_size - 1];
    }

    constexpr const T& get(size_t pos) const {
      assert(pos < _size);
      return _stack[pos];
    }

    constexpr void push(const T& value) {
      assert(_size < CAPACITY);
      _stack[_size++] = value;
    }
    constexpr void push(T&& value) {
      assert(_size < CAPACITY);
      _stack[_size++] = std::move(value);
    }
    constexpr T pop() {
      assert(_size > 0);
      return std::move(_stack[--_size]);
    }

    // Apply the given function to every item in the stack
    // We do it this way so the stack API can be preserved,
    // and no access to individual elements is allowed outside
    // the class.
    template <typename Func>
    constexpr void applyAll(Func&& func) {
      // Create a subrange of active stack elements, and apply func to each
      auto r = std::ranges::subrange(_stack.begin(), _stack.begin() + _size);
      std::ranges::for_each(r, std::forward<Func>(func));
    }
    template <typename Func>
    constexpr void applyAll(Func&& func) const {
      auto r = std::ranges::subrange(_stack.begin(), _stack.begin() + _size);
      std::ranges::for_each(r, std::forward<Func>(func));
    }

    friend std::ostream& operator<<(std::ostream& os, const FixedStack<T>& s) {
      s.applyAll([&os](const T& v) { os << v << ' '; });
      return os;
    }

  private:
    // Following constructors and assignment operators not supported
    FixedStack(const FixedStack&) = delete;
    FixedStack(FixedStack&&) = delete;
    FixedStack& operator=(const FixedStack&) = delete;
    FixedStack& operator=(FixedStack&&) = delete;
};

} // namespace Common

#endif
