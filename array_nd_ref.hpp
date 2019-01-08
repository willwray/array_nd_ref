
//    Copyright (c) 2018 Will Wray https://keybase.io/willwray
//
//   Distributed under the Boost Software License, Version 1.0.
//          (http://www.boost.org/LICENSE_1_0.txt)

#pragma once

//#include <algorithm>
#include <array>
//#include <iterator>

#include "traits.hpp"

/*
   "array_nd_ref.hpp"
    ^^^^^^^^^^^^^^^^
    This header defines array_nd_ref, a reference-wrapper class template
    for C-arrays of any rank - to hide irregularities of built-in arrays.

  Usage:
      int a[3][4];
      auto b = array_nd_ref{a}; // CTAD deduces array_nd_ref<int[3][4]>

  array_nd_ref<A>
    A ref-wrapper for general, multidimensional, C-array of type A.
    Think non-owning std::array that works with multidimensions.
    Think std::reference_wrapper/std::span specialized for nd array.

  Intent:
    Compatibility wrapper for C multi-dim arrays, with std features.
    Suitable as a parameter type, like a statically-sized std::span.
    Useful standalone or alongside an owning array_nd<A> type.
    Aims at correctness and constexpr ability above runtime performance.

  Requirements:
    C++17 for constructor template argument deduction CTAD.

  Experimental:
    C++20 concepts (via gcc -fconcepts) to constrain templates.


*/
// Adds std::array features:
//    No decay.
//    Copy semantics, including copy-list assignment, fill and swap.
//    Iterator interface.
//    Tuple interface -> unpacks via structured binding.
//    Comparison operators; < is lexicographic compare as-if a flat array.

// Unlike std::array:
//    It is a non-owning view type, so user beware dangling references.
//    It is cheap to pass to functions.
//    Default copy-assignment operator is shallow copy: just pointer copy
//    Assignment from C-array is deep copy.
//    Swap with another array_nd_ref is a shallow pointer swap
//    Swap with a C-array is deep: performs element-wise swaps
//    The template signature is <A> rather than <T,size_t...>
//    There's no zero-size specialization (there's no zero size C array).
//    Copy assign and comparisons enabled for C array rhs, constexpr.

// Indexing and iterating:
//    Iterators are raw pointer-to-array
//    Index operator[] returns builtin C subarray&
//    Index operator() returns ref-wrapped subarray for rank > 1

// Constness
//    The constness of array_nd_ref{arr} reflects the constness of arr,
//    array_nd_ref{T[N]} gives array_nd_ref<T[N]>
//    (CTAD constructor template auto deduction )
//    However, this requires users to take care with auto deduction.
//    With regular references auto& deduces constness - auto& x_ref = x;
//    With a reference wrapper

// Traits:
//    Shadows array traits std::remove_extent, std::remove_all_extents.
//    Provides specialized Rank and Extent variable template traits.

// Implementation note:
// Wrapped type:
//    array_nd_ref<X[N]> 'holds' X* rather than X(&)[N] or X(*)[N]
//                    -> it can view X[N] 'windows' into larger X[M]

// Note on recursive implementation:
// Array operations are implemented with recursion.
// Hierarchical access is required for constexpr-correctness
// (multi-dim C-arrays are laid out as a hierarchy of subarrays
//  T[L][M][N] == [L x T[M][N]] == [L x [M x T[N]]] == [Lx[Mx[NxT]]] )
// 'Flat' iteration of ND array elements requires reinterpret_cast,
// precluding constexpr. Memory access is contiguous in both cases.
// Code generation in practice is less optimal for hierarchical access
// than for flat iteration.
//
template <typename A>
requires std::is_array_v<A> && std::extent_v<A> != 0
struct array_nd_ref
{
    using type            = array_nd_ref;
    using const_type      = const array_nd_ref<A const>;

    using cv_array_type   = A;
    using array_type      = std::remove_cv_t<A>;

    using element_type    = std::remove_all_extents_t<A>;
    using value_type      = std::remove_cv_t<element_type>;

    using index_type      = ptrdiff_t ;
    using difference_type = ptrdiff_t;
    using size_type       = size_t;

    using remove_extent_type = std::remove_extent_t<A>;

    using reference       = remove_extent_type&;
    using const_reference = const reference;
    using pointer         = remove_extent_type*;
    using const_pointer   = remove_extent_type const*;

    using iterator        = pointer;
    using const_iterator  = const_pointer;

    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    static constexpr unsigned rank = std::rank<A>::value;
    static constexpr index_type extent = std::extent_v<A>;
    constexpr size_t size() const noexcept { return extent; }
    constexpr size_t max_size() const noexcept { return extent; }
    constexpr bool empty() const noexcept { return false; }

    pointer a;

    constexpr array_nd_ref(pointer p) : a{p} {}

    constexpr reference operator[](size_t I) { return a[I]; }
    constexpr const_reference operator[](size_t I) const { return a[I]; }

    // (index) returns array_nd_ref wrapped subarray for rank > 1
    constexpr decltype(auto) operator()(size_t I);
    constexpr decltype(auto) operator()(size_t I) const;

    constexpr reference at(size_t I);
    constexpr const_reference at(size_t I) const;

    constexpr reference front() { return {a[0]}; }
    constexpr const_reference front() const { return {a[0]}; }
    constexpr reference back() { return {a[extent-1]}; }
    constexpr const_reference back() const { return {a[extent-1]}; }

    constexpr iterator begin() noexcept { return a; }
    constexpr const_iterator begin() const noexcept { return a; }
    constexpr iterator end() noexcept { return a+extent; }
    constexpr const_iterator end() const noexcept { return a+extent; }

    constexpr reverse_iterator rbegin() noexcept { return end(); }
    constexpr const_reverse_iterator rbegin() const noexcept { return end(); }
    constexpr reverse_iterator rend() noexcept { return begin(); }
    constexpr const_reverse_iterator rend() const noexcept { return begin(); }

    constexpr const_iterator cbegin() const noexcept { return begin(); }
    constexpr const_iterator cend() const noexcept { return end(); }
    constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
    constexpr const_reverse_iterator crend() const noexcept { return rend(); }

    // Non-const data() member function returns a ref to the full array type
    A& data() noexcept { return *reinterpret_cast<A*>(a); } // try bit_cast?
    // The const data() member function returns the array decay; a pointer
    constexpr pointer data() const noexcept { return a; }

    // Member comparisons for array rhs call non-member (ref,ref) comparisons
    // (saves having to copy const but means no comparisons with array lhs).
    constexpr bool operator==( A const& y) const {
              return *this == array_nd_ref<A const>{y};
    }
    constexpr bool operator!=( A const& y) const {
           return !operator==(y);
    }
    constexpr bool operator<( A const& y) const {
              return *this < array_nd_ref<A const>{y};
    }
    constexpr bool operator>( A const& y) const {
              return *this > array_nd_ref<A const>{y};
    }
    constexpr bool operator<=( A const& y) const {
              return *this <= array_nd_ref<A const>{y};
    }
    constexpr bool operator>=( A const& y) const {
              return *this >= array_nd_ref<A const>{y};
    }

    // mutators, but only via ref, not const qualified
    constexpr void fill( const value_type& e);
    // deep copy of C-array rhs to referred-to array
    constexpr type operator=( array_type const& rhs);
    //constexpr type operator=( type& rhs) { return *this = rhs.a; }
    //constexpr type operator=( const_type& rhs) { return *this = rhs.a; };
    constexpr void swap( A& b) noexcept(std::is_nothrow_swappable_v<value_type>)
    {
        // does the right thing, recursive via std::swap(T(&)[N],T(&)[N])
        std::swap_ranges(a, a+extent, b);
    }
    // Shallow swap when both are array_nd_ref - just swap pointers
    constexpr void swap( array_nd_ref b) { std::swap(a,b.a); } 
};

template <typename A>
requires std::is_array_v<A> && std::extent_v<A> != 0
array_nd_ref(A&) -> array_nd_ref<A>;

template <typename A>
requires std::is_array_v<A> && std::extent_v<A> != 0 && std::is_const_v<A>
array_nd_ref(A&) -> array_nd_ref<A> const;

//template <typename T, size_t N>
//array_nd_ref(T[N]) -> array_nd_ref<T[N]>;
//
//template <typename T, size_t N>
//requires std::is_const_v<T>
//array_nd_ref(T[N]) -> array_nd_ref<T[N]> const;

template <typename T, size_t N>
requires N != 0
array_nd_ref(std::array<T,N>&) -> array_nd_ref<T[N]>;


template <typename A>
requires std::is_array_v<A> && std::extent_v<A> != 0
constexpr decltype(auto) array_nd_ref<A>::operator()(size_t I)
{
    if constexpr (rank == 1)
        return a[I];
    else
        return ::array_nd_ref{a[I]};
}

template <typename A>
requires std::is_array_v<A> && std::extent_v<A> != 0
constexpr decltype(auto) array_nd_ref<A>::operator()(size_t I) const
{
    if constexpr (rank == 1)
        return a[I];
    else
        return ::array_nd_ref{a[I]};
}

template <typename A>
requires std::is_array_v<A> && std::extent_v<A> != 0
constexpr auto array_nd_ref<A>::at(size_t I) -> reference
{
    if (I >= extent)
        throw(std::out_of_range("array_nd_ref::at"));
    return a[I];
}

template <typename A>
requires std::is_array_v<A> && std::extent_v<A> != 0
constexpr auto array_nd_ref<A>::at(size_t I) const -> const_reference
{
    if (I >= extent)
        throw(std::out_of_range("array_nd_ref::at"));
    return a[I];
}


// fill,and op= have similar recursive implementations
template <typename A>
requires std::is_array_v<A> && std::extent_v<A> != 0
constexpr void array_nd_ref<A>::fill( value_type const& e)
{
    if constexpr (rank == 1)
        std::fill_n(a, extent, e);
    else
        for (unsigned i=0; i<extent; ++i)
            operator()(i).fill(e);
}

template <typename A>
requires std::is_array_v<A> && std::extent_v<A> != 0
constexpr auto array_nd_ref<A>::operator=( array_type const& rhs)
            -> array_nd_ref<A>
{
    if constexpr (rank == 1)
        std::copy(rhs, rhs+extent, a);
    else
        for (unsigned i=0; i<extent; ++i)
            operator()(i) = rhs[i];
    return *this;
}

template <typename A>
requires std::is_array_v<A> && std::extent_v<A> != 0
      && std::is_swappable_v<std::remove_all_extents_t<A>>
constexpr void
swap( array_nd_ref<A> x, A& y) noexcept(noexcept(x.swap(y)))
{
    x.swap(y);
}

template <typename A>
requires std::is_array_v<A> && std::extent_v<A> != 0
      && std::is_swappable_v<std::remove_all_extents_t<A>>
constexpr void
swap( A& x, array_nd_ref<A> y) noexcept(noexcept(y.swap(x)))
{
    y.swap(x);
}

// Swaps pointers only - i.e. shallow swap, not deep
template <typename A>
requires std::is_array_v<A> && std::extent_v<A> != 0
constexpr void
swap( array_nd_ref<A> x, array_nd_ref<A> y) noexcept
{
    x.swap(y);
}

template <typename A>
requires std::is_array_v<A> && std::extent_v<A> != 0
constexpr size_t size( array_nd_ref<A> a) { return a.size(); }


template <typename A, typename Ac>
requires std::is_array_v<A> && std::extent_v<A> != 0
      && std::is_same_v<std::remove_const_t<A>, std::remove_const_t<Ac>>
constexpr bool
operator==( array_nd_ref<A> x, array_nd_ref<Ac> y)
{
    if constexpr (std::rank<A>::value == 1)
        //return std::equal(x.begin(), x.end(), y.begin());
        return [x,y]()->bool
        {
            for (size_t i=0; i != x.size(); ++i)
                if (x(i) != y(i)) return false;
            return true;
        }();
    else
    {
        for (unsigned i=0; i < std::extent_v<A>; ++i)
            if (!(x(i) == y(i)))
                return false;
        return true;
    }   
}

template <typename A, typename Ac>
requires std::is_array_v<A> && std::extent_v<A> != 0
      && std::is_same_v<std::remove_const_t<A>, std::remove_const_t<Ac>>
constexpr bool
operator!=( array_nd_ref<A> x, array_nd_ref<Ac> y)
{
    return !(x == y);
}

template <typename A, typename Ac>
requires std::is_array_v<A> && std::extent_v<A> != 0
      && std::is_same_v<std::remove_const_t<A>, std::remove_const_t<Ac>>
constexpr bool
operator<( array_nd_ref<A> x, array_nd_ref<Ac> y)
{
    if constexpr (std::rank<A>::value == 1)
        return std::lexicographical_compare(x.begin(), x.end(),
                                            y.begin(), y.end());
    else
    {
        for (unsigned i=0; i < std::extent_v<A>; ++i)
            if ((x(i) < y(i)))
                return true;
            else if ((x(i) > y(i)))
                return false;
        return false;
    }   
}

template <typename A, typename Ac>
requires std::is_array_v<A> && std::extent_v<A> != 0
      && std::is_same_v<std::remove_const_t<A>, std::remove_const_t<Ac>>
constexpr bool
operator>( array_nd_ref<A> x, array_nd_ref<Ac> y)
{
    return y < x;
}

template <typename A, typename Ac>
requires std::is_array_v<A> && std::extent_v<A> != 0
      && std::is_same_v<std::remove_const_t<A>, std::remove_const_t<Ac>>
constexpr bool
operator<=( array_nd_ref<A> x, array_nd_ref<Ac> y)
{
    return !(y < x);
}

template <typename A, typename Ac>
requires std::is_array_v<A> && std::extent_v<A> != 0
      && std::is_same_v<std::remove_const_t<A>, std::remove_const_t<Ac>>
constexpr bool
operator>=( array_nd_ref<A> x, array_nd_ref<Ac> y)
{
    return !(x < y);
}


template <size_t I, typename A>
requires I < std::extent_v<A>
constexpr auto&
get( array_nd_ref<A>& a)
{
    return a[I];
}

template <size_t I, typename A>
requires I < std::extent_v<A>
constexpr auto const&
get( array_nd_ref<A> const& a)
{
    return a[I];
}

template <size_t I, typename A>
requires I < std::extent_v<A>
constexpr auto&
get( array_nd_ref<A>&& a)
{
    return get<I>(a);
}

namespace array_nd
{
// wrap_t<T> yields T if not an array or array_nd_ref<T> if array
template <typename T>
struct wrap
{
    using type = T;
};
template <typename A>
requires std::is_array_v<A>
struct wrap<A>
{
    using type = array_nd_ref<A>;
};
template <typename T>
using wrap_t = typename wrap<T>::type;

// ref_t<T> yields T& if not an array or array_nd_ref<T> if array
template <typename T>
struct ref
{
    using type = T&;
};
template <typename A>
requires std::is_array_v<A>
struct ref<A>
{
    using type = array_nd_ref<A>;
};
template <typename T>
using ref_t = typename ref<T>::type;
}

namespace impl
{
template <typename...> struct is_array_nd_ref
                            : std::false_type {};
template <typename A>  struct is_array_nd_ref<array_nd_ref<A>>
                            : std::true_type {};
}
template <typename A> struct is_array_nd_ref
                           : impl::is_array_nd_ref<std::remove_cv_t<A>> {};

template <typename A> inline constexpr bool is_array_nd_ref_v
                                          = is_array_nd_ref<A>::value;

template <typename A>
requires is_array_nd_ref_v<A>
struct array_type<A>
{
    using type = typename A::array_type;
};

// Extend std array traits to support array_nd_ref
//   rank, extent, remove_extent, remove_all_extents
template <typename A>
requires is_array_nd_ref_v<A>
inline constexpr size_t Rank<A> = std::rank_v<array_t<A>>;

template <typename A>
requires is_array_nd_ref_v<A>
inline constexpr size_t Extent<A> = std::extent_v<array_t<A>>;

template <typename A>
requires is_array_nd_ref_v<A>
struct remove_all_extents<A>
: std::remove_all_extents<typename A::cv_array_type> {};

template <typename A>
requires is_array_nd_ref_v<A> && Rank<A> == 1
struct remove_extent<A>
{
    using type = typename array_nd_ref<A>::element_type;
};
template <typename T, size_t N>
requires Rank<T> > 1
struct remove_extent<array_nd_ref<T[N]>>
{
    using type = array_nd_ref<T>;
};
template <typename T, size_t N>
requires Rank<T> > 1
struct remove_extent<const array_nd_ref<T[N]>>
{
    using type = const array_nd_ref<T>;
};

// Tuple access customization, confers structured binding support
namespace std
{
template <typename A>
struct tuple_size<array_nd_ref<A>>
     : std::integral_constant<size_t, std::extent_v<A>> {};

template <size_t I, typename A>
struct tuple_element<I,array_nd_ref<A>>
{
     using type = std::remove_extent_t<A>;
};
}
