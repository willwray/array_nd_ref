
//    Copyright (c) 2018 Will Wray https://keybase.io/willwray
//
//   Distributed under the Boost Software License, Version 1.0.
//          (http://www.boost.org/LICENSE_1_0.txt)

#pragma once
// traits.hpp

#include <cstddef>
#include <type_traits>

// remove_cvref<T> combo as proposed by Walter Brown for C++20
template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

// is_not_cvref<T>
// use to require caller to remove cv/ref quals
template <typename T>
constexpr bool is_not_cvref = std::is_same_v<T, remove_cvref_t<T>>;

// copy_ref<A,B> apply reference qualifier from A to B
template <typename A, typename B>
using copy_ref = std::conditional_t<std::is_lvalue_reference_v<A>, B&,
                 std::conditional_t<std::is_rvalue_reference_v<A>, B&&, B>>;

// copy_cv<A,B> apply cv qualifiers from A to B
template <typename A, typename B>
using copy_cv = std::conditional_t<!std::is_volatile_v<A>,
                std::conditional_t<std::is_const_v<A>, B const, B>,
                std::conditional_t<std::is_const_v<A>, B const volatile, B volatile>>;


/*
// all_same trait
template <typename...> struct all_same : std::true_type { using type = void; };
template <typename T, typename... U>
struct all_same<T,U...> : std::bool_constant<(std::is_same_v<T,U> && ...)>
{
    using type = std::conditional_t<(std::is_same_v<T,U> && ...), T, void>;
};
template <typename... T> constexpr bool all_same_v = all_same<T...>();
template <typename... T> using all_same_t = typename all_same<T...>::type;
*/
// all_same<T...> true_type if all T are the same type
template <typename...>
struct all_same : std::false_type {};
template <>
struct all_same<> : std::true_type { using type = void; };
template <typename T>
struct all_same<T> : std::true_type { using type = T; };
template <typename T, typename... U> 
requires (std::is_same_v<T,U> && ...)
struct all_same<T,U...> : std::true_type { using type = T; };

template <typename... T>
using all_same_t = typename all_same<T...>::type;

template <typename... T>
constexpr bool all_same_v = all_same<T...>();

// Trait classes for member pointers
// member_ptr<mptr> decomposes a member pointer type
template <typename>
struct member_ptr;

template <class S, typename T>
struct member_ptr<T S::*>
{
    using class_type = S;
    using value_type = T;
    using pointer_type = T S::*;
};

// named_member_ptr<&S::name> decomposes a qualified member pointer
template <auto MP>
struct named_member_ptr :
             member_ptr<decltype(MP)>
{
    static constexpr auto mp = MP;
};

template <auto MP>
using named_member_class_type = typename named_member_ptr<MP>::class_type;
template <auto MP>
using named_member_value_type = typename named_member_ptr<MP>::value_type;

// concept mptrs_of_same_class<mp...>
// true if all mp are pointers to members of same class
template <auto... mp>
concept bool mptrs_of_same_class =
        (std::is_member_pointer_v<decltype(mp)> && ...)
      && all_same_v<named_member_class_type<mp>...>;


namespace impl
{
// impl::is_complete(T*) allows to test a type T for completeness.
// Use in unevaluated context for default template arg to force re-evaluation
//  (1) that T is incomplete at some point in TU
//  (2) that T is complete at some later point
// Warning: note that this stateful behaviour can violate ODR
// https://stackoverflow.com/a/17672456/7443483

template <class T>
std::enable_if_t< sizeof(T),
std::true_type> is_complete( T* );

std::false_type is_complete( ... );
}

// array traits
// Allow generic handling of builtin array or array class types
// An array class should specialize Array and array_type

// Clashes with typeclass Array
//template <typename A>
//inline constexpr bool Array = std::is_array_v<A>;

// array_type:
// For class types that model the interface of builtin array
// provides type alias for the corresponding builtin array type.
template <typename A>
struct array_type
{
    using type = A;
};

template <typename A>
using array_t = typename array_type<A>::type;

// std::array forward declaration and specializations.
namespace std {
template <typename T, size_t N> struct array;
};

template <typename T, size_t N>
struct array_type< std::array<T,N>>
{
    using type = std::array<T,N>;
};

// Rank - array rank variable template, specialize for class types
template <typename A> inline constexpr auto Rank = std::rank_v<A>;
template <typename T, size_t N>
inline constexpr auto Rank<std::array<T,N>> = N;

// Extent - array extent variable template, specialize for class types
template <typename A> inline constexpr size_t Extent = std::extent_v<A>;

template <typename A>
struct remove_extent
: std::remove_extent<A> {};

template <typename A>
struct remove_all_extents
: std::remove_all_extents<A> {};

template <typename A>
using remove_extent_t = typename remove_extent<A>::type;

template <typename A>
using remove_all_extents_t = typename remove_all_extents<A>::type;

template <typename T>
inline constexpr
size_t array_size = std::rank_v<T> ?  
       array_size<std::remove_extent_t<T>> * std::extent_v<T> : 1;






template <typename T>
struct move_ass
{
    using type = std::conditional_t<
        std::is_move_assignable_v<T>, T&&, T const&>;
};

template <typename T>
using move_ass_t = typename move_ass<T>::type;
