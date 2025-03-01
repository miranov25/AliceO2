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
#ifndef O2_FRAMEWORK_PACK_H_
#define O2_FRAMEWORK_PACK_H_

#include <cstddef>
#include <utility>

namespace o2::framework
{

/// Type helper to hold a parameter pack.  This is different from a tuple
/// as there is no data associated to it.
template <typename...>
struct pack {
};

/// template function to determine number of types in a pack
template <typename... Ts>
constexpr std::size_t pack_size(pack<Ts...> const&)
{
  return sizeof...(Ts);
}

template <std::size_t I, typename T>
struct pack_element;

#if __has_builtin(__type_pack_element)
template <std::size_t I, typename... Ts>
struct pack_element<I, pack<Ts...>> {
  using type = __type_pack_element<I, Ts...>;
};
#else
// recursive case
template <std::size_t I, typename Head, typename... Tail>
struct pack_element<I, pack<Head, Tail...>>
  : pack_element<I - 1, pack<Tail...>> {
};

// base case
template <typename Head, typename... Tail>
struct pack_element<0, pack<Head, Tail...>> {
  typedef Head type;
};
#endif

template <std::size_t I, typename T>
using pack_element_t = typename pack_element<I, T>::type;

template <typename T>
using pack_head_t = typename pack_element<0, T>::type;

template <typename Head, typename... Tail>
constexpr auto pack_tail(pack<Head, Tail...>)
{
  return pack<Tail...>{};
}

/// Templates for manipulating type lists in pack
/// (see https://codereview.stackexchange.com/questions/201209/filter-template-meta-function/201222#201222)
/// Example of use:
///     template<typename T>
///         struct is_not_double: std::true_type{};
///     template<>
///         struct is_not_double<double>: std::false_type{};
/// The following will return a pack, excluding double
///  filtered_pack<is_not_double, double, int, char, float*, double, char*, double>()
///
template <typename... Args1, typename... Args2>
constexpr auto concatenate_pack(pack<Args1...>, pack<Args2...>)
{
  return pack<Args1..., Args2...>{};
}

template <typename P1, typename P2, typename... Ps>
constexpr auto concatenate_pack(P1 p1, P2 p2, Ps... ps)
{
  return concatenate_pack(p1, concatenate_pack(p2, ps...));
}

template <typename... Ps>
using concatenated_pack_t = decltype(concatenate_pack(Ps{}...));

template <typename... Args1, typename... Args2>
constexpr auto interleave_pack(pack<Args1...>, pack<Args2...>)
{
  return concatenated_pack_t<pack<Args1, Args2>...>{};
}

template <typename P1, typename P2>
using interleaved_pack_t = decltype(interleave_pack(P1{}, P2{}));

/// Marks as void the types that do not satisfy the condition
template <template <typename...> typename Condition, typename... Ts>
using with_condition_pack = pack<std::conditional_t<Condition<Ts>::value, Ts, void>...>;

template <typename... Ts>
consteval auto count_non_void_pack(pack<Ts...> const&)
{
  return ((std::is_void_v<Ts> ? 0 : 1) + ...);
}

template <typename Result>
consteval auto prune_voids_pack(Result result, pack<>)
{
  return result;
}

// The first one is non void, but one of the others is void
template <typename... Rs, typename T, typename... Ts>
  requires(!std::is_void_v<T>)
consteval auto prune_voids_pack(pack<Rs...> result, pack<T, Ts...>)
{
  return prune_voids_pack(pack<Rs..., T>{}, pack<Ts...>{});
}

// The first one is void
template <typename... Rs, typename V, typename... Ts>
  requires(std::is_void_v<V>)
consteval auto prune_voids_pack(pack<Rs...> result, pack<V, Ts...>)
{
  return prune_voids_pack(pack<Rs...>{}, pack<Ts...>{});
}

// The first one is non void, but one of the others is void
template <typename... Rs, typename T1, typename T2, typename... Ts>
  requires(!std::is_void_v<T1> && !std::is_void_v<T2>)
consteval auto prune_voids_pack(pack<Rs...> result, pack<T1, T2, Ts...>)
{
  return prune_voids_pack(pack<Rs..., T1, T2>{}, pack<Ts...>{});
}

// Eats 4 types at the time
template <typename... Rs, typename T1, typename T2, typename T3, typename T4, typename... Ts>
  requires(!std::is_void_v<T1> && !std::is_void_v<T2> && !std::is_void_v<T3> && !std::is_void_v<T4>)
consteval auto prune_voids_pack(pack<Rs...> result, pack<T1, T2, T3, T4, Ts...>)
{
  return prune_voids_pack(pack<Rs..., T1, T2, T3, T4>{}, pack<Ts...>{});
}

// Eats 8 types at the time
template <typename... Rs, typename T1, typename T2, typename T3, typename T4,
          typename T5, typename T6, typename T7, typename T8, typename... Ts>
  requires(!std::is_void_v<T1> && !std::is_void_v<T2> && !std::is_void_v<T3> && !std::is_void_v<T4> && !std::is_void_v<T5> && !std::is_void_v<T6> && !std::is_void_v<T7> && !std::is_void_v<T8>)
consteval auto prune_voids_pack(pack<Rs...> result, pack<T1, T2, T3, T4, T5, T6, T7, T8, Ts...>)
{
  return prune_voids_pack(pack<Rs..., T1, T2, T3, T4, T5, T6, T7, T8>{}, pack<Ts...>{});
}

/// Selects from the pack types that satisfy the Condition
/// Multicondition takes the type to check as first template parameter
/// and any helper types as the following parameters
template <template <typename...> typename Condition, typename Result, typename... Cs>
consteval auto select_pack(Result result, pack<>, pack<Cs...>)
{
  return result;
}

template <template <typename...> typename Condition, typename Result, typename T, typename... Cs, typename... Ts>
consteval auto select_pack(Result result, pack<T, Ts...>, pack<Cs...> condPack)
{
  if constexpr (Condition<T, Cs...>()) {
    return select_pack<Condition>(concatenate_pack(result, pack<T>{}), pack<Ts...>{}, condPack);
  } else {
    return select_pack<Condition>(result, pack<Ts...>{}, condPack);
  }
}

template <template <typename...> typename Condition, typename... Types>
using selected_pack = std::decay_t<decltype(prune_voids_pack(pack<>{}, with_condition_pack<Condition, Types...>{}))>;
template <template <typename...> typename Condition, typename CondPack, typename Pack>
using selected_pack_multicondition = std::decay_t<decltype(select_pack<Condition>(pack<>{}, Pack{}, CondPack{}))>;

/// Select only the items of a pack which match Condition
template <template <typename> typename Condition, typename Result>
constexpr auto filter_pack(Result result, pack<>)
{
  return result;
}

template <template <typename> typename Condition, typename Result, typename T, typename... Ts>
constexpr auto filter_pack(Result result, pack<T, Ts...>)
{
  if constexpr (Condition<T>()) {
    return filter_pack<Condition>(result, pack<Ts...>{});
  } else {
    return filter_pack<Condition>(concatenate_pack(result, pack<T>{}), pack<Ts...>{});
  }
}

template <typename T>
void print_pack()
{
  puts(__PRETTY_FUNCTION__);
}

template <template <typename> typename Condition, typename... Types>
using filtered_pack = std::decay_t<decltype(filter_pack<Condition>(pack<>{}, pack<Types...>{}))>;

template <typename T, typename... Us>
bool consteval has_type(framework::pack<Us...>)
{
  return (std::same_as<T, Us> || ...);
}

template <typename T, typename P>
inline constexpr bool has_type_v = has_type<T>(P{});

template <template <typename, typename> typename Condition, typename T, typename... Us>
bool consteval has_type_conditional(framework::pack<Us...>)
{
  return (Condition<T, Us>::value || ...);
}

template <template <typename, typename> typename Condition, typename T, typename P>
inline constexpr bool has_type_conditional_v = has_type_conditional<Condition, T>(P{});

template <typename T, typename... Ts>
consteval size_t has_type_at_v(pack<Ts...>)
{
  constexpr size_t size = sizeof...(Ts);
  constexpr bool found[size] = {std::same_as<T, Ts>...};
  for (size_t i = 0; i < size; ++i) {
    if (found[i]) {
      return i;
    }
  }
  return size + 1;
}

template <template <typename, typename> typename Condition, typename T, typename... Ts>
consteval size_t has_type_at_conditional_v(pack<Ts...>)
{
  constexpr size_t size = sizeof...(Ts);
  constexpr bool found[size] = {Condition<T, Ts>::value...};
  for (size_t i = 0; i < size; ++i) {
    if (found[i]) {
      return i;
    }
  }
  return size + 1;
}

/// Intersect two packs
template <typename S1, typename S2>
struct intersect_pack {
  template <std::size_t... Indices>
  static constexpr auto make_intersection(std::index_sequence<Indices...>)
  {
    return filtered_pack<std::is_void,
                         std::conditional_t<
                           has_type<pack_element_t<Indices, S1>>(S2{}),
                           pack_element_t<Indices, S1>, void>...>{};
  }
  using type = decltype(make_intersection(std::make_index_sequence<pack_size(S1{})>{}));
};

template <typename S1, typename S2>
using intersected_pack_t = typename intersect_pack<S1, S2>::type;

template <typename... A1, typename... A2>
constexpr auto intersected_pack(pack<A1...>, pack<A2...>)
{
  return intersected_pack_t<pack<A1...>, pack<A2...>>{};
}

template <typename P1, typename P2, typename... Ps>
constexpr auto intersected_pack(P1 p1, P2 p2, Ps... ps)
{
  return intersected_pack(p1, intersected_pack(p2, ps...));
}

template <typename... Ps>
using full_intersected_pack_t = decltype(intersected_pack(Ps{}...));

/// Subtract two packs
template <typename S1, typename S2>
struct subtract_pack {
  template <std::size_t... Indices>
  static constexpr auto make_subtraction(std::index_sequence<Indices...>)
  {
    return filtered_pack<std::is_void,
                         std::conditional_t<
                           !has_type<pack_element_t<Indices, S1>>(S2{}),
                           pack_element_t<Indices, S1>, void>...>{};
  }
  using type = decltype(make_subtraction(std::make_index_sequence<pack_size(S1{})>{}));
};

template <typename... Args1, typename... Args2>
constexpr auto concatenate_pack_unique(pack<Args1...>, pack<Args2...>)
{
  using p1 = typename subtract_pack<pack<Args1...>, pack<Args2...>>::type;
  return concatenate_pack(p1{}, pack<Args2...>{});
}

template <typename P1>
constexpr auto concatenate_pack_unique(P1 p1)
{
  return p1;
}

template <typename P1, typename P2, typename... Ps>
constexpr auto concatenate_pack_unique(P1 p1, P2 p2, Ps... ps)
{
  return concatenate_pack_unique(p1, concatenate_pack_unique(p2, ps...));
}

template <typename... Ps>
using concatenated_pack_unique_t = decltype(concatenate_pack_unique(Ps{}...));

template <typename PT>
constexpr auto unique_pack(pack<>, PT p2)
{
  return p2;
}

template <typename PT, typename T, typename... Ts>
constexpr auto unique_pack(pack<T, Ts...>, PT p2)
{
  return unique_pack(pack<Ts...>{}, concatenate_pack_unique(pack<T>{}, p2));
}

template <typename P>
using unique_pack_t = decltype(unique_pack(P{}, pack<>{}));

template <typename... Ts>
inline constexpr std::tuple<Ts...> pack_to_tuple(pack<Ts...>)
{
  return std::tuple<Ts...>{};
}

template <typename P>
using pack_to_tuple_t = decltype(pack_to_tuple(P{}));

template <typename T, std::size_t... Is>
inline auto sequence_to_pack(std::integer_sequence<std::size_t, Is...>)
{
  return pack<decltype((Is, T{}))...>{};
};

template <typename T, std::size_t N>
using repeated_type_pack_t = decltype(sequence_to_pack<T>(std::make_index_sequence<N>()));

} // namespace o2::framework

#endif // O2_FRAMEWORK_PACK_H_
