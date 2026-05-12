/**
    Type-safe enum class bitmasks.
    Based on work by Andre Haupt from his blog at
    http://blog.bitwigglers.org/using-enum-classes-as-type-safe-bitmasks/

    C++20 modernization: concepts replace enable_if, requires-clauses replace
    SFINAE, redundant inline/typename removed. Macro removed in favour of a
    direct variable template specialization.

    To enable enum classes to be used as bitmasks, specialize is_bitmask_enum_v:

    enum class MyBitmask {
        None  = 0b0000,
        One   = 0b0001,
        Two   = 0b0010,
        Three = 0b0100,
    };
    template<> inline constexpr bool Bitmask::is_enum_v<MyBitmask> = true;

    From now on, MyBitmask's values can be used with bitwise operators.
    Wrap in BitmaskEnum for ergonomic checks:

    Bitmask::Enum wbm{bm};
    wbm.any_of(MyBitmask::One | MyBitmask::Three);
    wbm.all_of(MyBitmask::One | MyBitmask::Three);
    if (wbm) { ... }                 // any bit set?
    MyBitmask back = wbm;            // implicit conversion back

    NOTE: Complement (~) and any_except/none_except operate on the full width
    of the underlying type.  Bits above the highest defined enumerator will be
    set by ~, so any_except/none_except may return true for a zero value if the
    enum does not cover the full underlying type.  This is an inherent property
    of bitmask enums; document the underlying type width for users.

    -------------------------------------------------------------------------------
    MIT License
    Copyright (c) 2019 Ivan Roberto de Oliveira
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/
#ifndef BITMASK_ENUM_HXX
#define BITMASK_ENUM_HXX

#include <bit>
#include <concepts>
#include <limits>
#include <type_traits>

namespace Bitmask {

// ---------------------------------------------------------------------------
// Opt-in trait
// ---------------------------------------------------------------------------

template<typename Enum>
inline constexpr bool is_enum_v = false;

// ---------------------------------------------------------------------------
// Concept
// ---------------------------------------------------------------------------

template<typename Enum>
concept Type =
    std::is_enum_v<Enum> &&
    std::is_unsigned_v<std::underlying_type_t<Enum>> &&
    is_enum_v<Enum>;

// ---------------------------------------------------------------------------
// Underlying helpers (safe conversion boundary)
// ---------------------------------------------------------------------------

template<Type Enum>
[[nodiscard]] constexpr std::underlying_type_t<Enum>
to_underlying(Enum e) noexcept {
    return static_cast<std::underlying_type_t<Enum>>(e);
}

template<Type Enum>
[[nodiscard]] constexpr Enum
from_underlying(std::underlying_type_t<Enum> v) noexcept {
    // static_cast<Enum> triggers clang-analyzer-optin.core.EnumCastOutOfRange
    // for combined bitmask values that don't match a named enumerator.
    // std::bit_cast is well-defined for same-size trivially-copyable types
    // and produces identical codegen without the diagnostic.
    return std::bit_cast<Enum>(v);
}

// ---------------------------------------------------------------------------
// Bitmask wrapper
// ---------------------------------------------------------------------------

template<Type Enum>
struct BitmaskEnum {
    Enum value;

    static constexpr Enum none{};

    constexpr explicit BitmaskEnum(Enum v) noexcept
        : value{v} {}

    constexpr explicit operator Enum() const noexcept {
        return value;
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return value != none;
    }

    [[nodiscard]] constexpr bool any() const noexcept {
        return value != none;
    }

    [[nodiscard]] constexpr bool empty() const noexcept {
        return value == none;
    }

    [[nodiscard]] constexpr bool any_of(Enum mask) const noexcept {
        return (value & mask) != none;
    }

    [[nodiscard]] constexpr bool all_of(Enum mask) const noexcept {
        return (value & mask) == mask;
    }

    [[nodiscard]] constexpr bool none_of(Enum mask) const noexcept {
        return (value & mask) == none;
    }

//     // NOTE: any_except/none_except flip all bits in the underlying type width.
//     // See file-level note about complement semantics.
//     [[nodiscard]] constexpr bool any_except(Enum mask) const noexcept {
//         return (value & ~mask) != none;
//     }
//
//     [[nodiscard]] constexpr bool none_except(Enum mask) const noexcept {
//         return (value & ~mask) == none;
//     }

    friend constexpr bool
    operator==(BitmaskEnum, BitmaskEnum) noexcept = default;

    constexpr BitmaskEnum& operator|=(Enum rhs) noexcept {
        value = from_underlying<Enum>(to_underlying(value) | to_underlying(rhs));
        return *this;
    }

    constexpr BitmaskEnum& operator&=(Enum rhs) noexcept {
        value = from_underlying<Enum>(to_underlying(value) & to_underlying(rhs));
        return *this;
    }

    constexpr BitmaskEnum& operator^=(Enum rhs) noexcept {
        value = from_underlying<Enum>(to_underlying(value) ^ to_underlying(rhs));
        return *this;
    }
};

// ---------------------------------------------------------------------------
// Public alias
// ---------------------------------------------------------------------------

template<Type T>
using Enum = BitmaskEnum<T>;

// ---------------------------------------------------------------------------
// CTAD
// ---------------------------------------------------------------------------

template<Type T>
BitmaskEnum(T) -> BitmaskEnum<T>;

} // namespace Bitmask

// ===========================================================================
// Bitwise operators (global namespace for ADL compatibility)
// ===========================================================================

template<Bitmask::Type Enum>
[[nodiscard]] constexpr Enum operator|(Enum lhs, Enum rhs) noexcept {
    return Bitmask::from_underlying<Enum>(
        Bitmask::to_underlying(lhs) |
        Bitmask::to_underlying(rhs)
    );
}

template<Bitmask::Type Enum>
[[nodiscard]] constexpr Enum operator&(Enum lhs, Enum rhs) noexcept {
    return Bitmask::from_underlying<Enum>(
        Bitmask::to_underlying(lhs) &
        Bitmask::to_underlying(rhs)
    );
}

template<Bitmask::Type Enum>
[[nodiscard]] constexpr Enum operator^(Enum lhs, Enum rhs) noexcept {
    return Bitmask::from_underlying<Enum>(
        Bitmask::to_underlying(lhs) ^
        Bitmask::to_underlying(rhs)
    );
}

template<Bitmask::Type Enum>
[[nodiscard]] constexpr Enum operator~(Enum rhs) noexcept {
    using U = std::underlying_type_t<Enum>;
    constexpr U all_bits = std::numeric_limits<U>::max();
    return Bitmask::from_underlying<Enum>(
        static_cast<U>(~Bitmask::to_underlying(rhs) & all_bits)
    );
}

template<Bitmask::Type Enum>
constexpr Enum& operator|=(Enum& lhs, Enum rhs) noexcept {
    lhs = Bitmask::from_underlying<Enum>(
        Bitmask::to_underlying(lhs) |
        Bitmask::to_underlying(rhs)
    );
    return lhs;
}

template<Bitmask::Type Enum>
constexpr Enum& operator&=(Enum& lhs, Enum rhs) noexcept {
    lhs = Bitmask::from_underlying<Enum>(
        Bitmask::to_underlying(lhs) &
        Bitmask::to_underlying(rhs)
    );
    return lhs;
}

template<Bitmask::Type Enum>
constexpr Enum& operator^=(Enum& lhs, Enum rhs) noexcept {
    lhs = Bitmask::from_underlying<Enum>(
        Bitmask::to_underlying(lhs) ^
        Bitmask::to_underlying(rhs)
    );
    return lhs;
}

#endif // BITMASK_ENUM_HXX
