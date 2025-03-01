// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
#ifndef O2_FRAMEWORK_TRAITS_H_
#define O2_FRAMEWORK_TRAITS_H_

#include <type_traits>
#include <concepts>

namespace o2::framework
{
/// Helper trait to determine if a given type T
/// is a specialization of a given reference type Ref.
/// See Framework/Core/test_TypeTraits.cxx for an example

template <typename T, template <typename...> class Ref>
struct is_specialization : std::false_type {
};

template <template <typename...> class Ref, typename... Args>
struct is_specialization<Ref<Args...>, Ref> : std::true_type {
};

template <typename T, template <typename...> class Ref>
inline constexpr bool is_specialization_v = is_specialization<T, Ref>::value;

template <template <typename...> typename T, typename S>
concept specialization_of_template = requires {
  {
    []<typename... Ts>(T<Ts...>*) -> T<Ts...> {}(std::declval<S*>())
  } -> std::same_as<S>;
};

template <typename A, typename B>
struct is_overriding : public std::bool_constant<std::is_same_v<A, B> == false && std::is_member_function_pointer_v<A> && std::is_member_function_pointer_v<B>> {
};

template <typename... T>
struct always_static_assert : std::false_type {
};

template <typename... T>
inline constexpr bool always_static_assert_v = always_static_assert<T...>::value;

template <template <typename...> typename B, typename D>
concept base_of_template = requires {
  []<typename... Ts>(B<Ts...>*) {}(std::declval<D*>());
};

template <template <typename...> typename B, typename D>
constexpr bool is_base_of_template_v = base_of_template<B, D>;

} // namespace o2::framework

#endif // O2_FRAMEWORK_TRAITS_H_
