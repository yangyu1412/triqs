// Copyright (c) 2013-2018 Commissariat à l'énergie atomique et aux énergies alternatives (CEA)
// Copyright (c) 2013-2018 Centre national de la recherche scientifique (CNRS)
// Copyright (c) 2015 Igor Krivenko
// Copyright (c) 2018-2019 Simons Foundation
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You may obtain a copy of the License at
//     https://www.gnu.org/licenses/gpl-3.0.txt
//
// Authors: Igor Krivenko, Olivier Parcollet, Nils Wentzell

#pragma once
#include <triqs/utility/macros.hpp>
#include <tuple>
#include <ostream>

// FIXME : move to nda::stdutil
namespace std {

  template <typename... T> std::ostream &operator<<(std::ostream &os, std::tuple<T...> const &t) {
    os << "(";
    auto pr = [&os, c = 0](auto &x) mutable {
      if (c++) os << ',';
      os << x;
    };
    [&]<auto... Is>(std::index_sequence<Is...>) { (..., (void)pr(std::get<Is>(t))); }
    (std::make_index_sequence<sizeof...(T)>{});
    return os << ")";
  }

  template <typename T1, typename T2> std::ostream &operator<<(std::ostream &os, std::pair<T1, T2> const &x) {
    return os << '(' << x.first << ", " << x.second << ')';
  }
} // namespace std

namespace triqs::tuple {

  // a concept to check the Tuple
  template <typename T>
  concept Tuple = requires(T const &t) {
    // clang-format off
    { int(std::tuple_size_v<T>) }; // ->std::integral; // FIXME clang <13 workaround ... 
    {std::get<0>(t)};
    // clang-format on
  };

  /*
  * for_each(t, f)
  * t: a tuple
  * f: a callable object
  * calls f on all tuple elements in the order of the tuple: f(x) for all x in t
  */
  //FIXME : REVEISER ORER
  template <typename F, Tuple Tu> void for_each(Tu &&t, F &&f) {
    [&]<auto... Is>(std::index_sequence<Is...>) mutable { (..., (void)f(std::get<Is>(t))); }
    (std::make_index_sequence<std::tuple_size_v<std::decay_t<Tu>>>{});
  }

  /* for_each_enumerate(t, f)
  * t: a tuple
  * f: a callable object
  * calls f on all tuple elements:
  *  Python equivalent :
  *    for n,x in enumrate(t): f(n,x)
  */
  template <typename F, Tuple Tu> void for_each_enumerate(Tu &&t, F &&f) {
    [&]<auto... Is>(std::index_sequence<Is...>) mutable { (..., (void)f(Is, std::get<Is>(t))); }
    (std::make_index_sequence<std::tuple_size_v<std::decay_t<Tu>>>{});
  }

  /*
  * map(f, t)
  * f : a callable object
  * t tuple
  * Returns : [f(i) for i in t]
  */
  template <typename F, Tuple Tu> auto map(F const &f, Tu const &t) {
    return [&]<auto... Is>(std::index_sequence<Is...>) { return std::make_tuple(f(std::get<Is>(t))...); }
    (std::make_index_sequence<std::tuple_size_v<std::decay_t<Tu>>>{});
  }

  /*
  * map_on_zip(f, t1,t2)
  * f : a callable object
  * t1,t2 tuples of the same size
  * Returns : [f(i,j) for i,j in zip(t1,t2)]
  */
  template <typename F, Tuple Tu1, Tuple Tu2> auto map_on_zip(F const &f, Tu1 const &t1, Tu2 const &t2) {
    static_assert(std::tuple_size_v<std::decay_t<Tu1>> == std::tuple_size_v<std::decay_t<Tu2>>, "Tuple should have the same size");
    return [&]<auto... Is>(std::index_sequence<Is...>) { return std::make_tuple(f(std::get<Is>(t1), std::get<Is>(t2))...); }
    (std::make_index_sequence<std::tuple_size_v<std::decay_t<Tu1>>>{});
  }

  /*
   * fold(f, t1, r_init)
   * f : a callable object : f(x,r) -> r'
   * t a tuple
   * Returns : f(xN,f(x_N-1,...f(x0,r_init)) on the tuple
   */

  template <int pos, typename F, typename R, Tuple Tu> decltype(auto) fold_impl(F &&f, Tu &&t, R &&r) {
    if constexpr (pos == (std::tuple_size_v<std::decay_t<Tu>> - 1))
      return f(std::get<pos>(std::forward<Tu>(t)), std::forward<R>(r));
    else
      return fold_impl<pos + 1>(std::forward<F>(f), std::forward<Tu>(t), f(std::get<pos>(std::forward<Tu>(t)), std::forward<R>(r)));
  }

  template <typename F, typename R, Tuple Tu> decltype(auto) fold(F &&f, Tu &&t, R &&r) {
    return fold_impl<0>(std::forward<F>(f), t, std::forward<R>(r));
  }

  /*
   * fold(f, t1, t2, init)
   * f : a callable object
   * t1, t2 two tuples of the same size
   * Returns : f(x0,y0,f(x1,y1,,f(....)) for t1 = (x0,x1 ...) and t2 = (y0,y1...).
   */
  template <int pos, typename F, Tuple Tu1, Tuple Tu2, typename R> decltype(auto) fold_impl(F &&f, Tu1 &&t1, Tu2 &&t2, R &&r) {
    if constexpr (pos == std::tuple_size_v<std::decay_t<Tu1>> - 1)
      return f(std::get<pos>(std::forward<Tu1>(t1)), std::get<pos>(std::forward<Tu2>(t2)), std::forward<R>(r));
    else
      return fold_impl<pos + 1>(std::forward<F>(f), std::forward<Tu1>(t1), std::forward<Tu2>(t2),
                                f(std::get<pos>(std::forward<Tu1>(t1)), std::get<pos>(std::forward<Tu2>(t2)), std::forward<R>(r)));
  }

  template <typename F, typename R, Tuple Tu1, Tuple Tu2> decltype(auto) fold(F &&f, Tu1 &&t1, Tu2 &&t2, R &&r) {
    return fold_impl<0>(std::forward<F>(f), t1, t2, std::forward<R>(r));
  }
#if 0
   template <int pos, typename F, typename R, Tuple Tu0, Tuple... Tu> decltype(auto) fold_impl(F &&f, Tu0 &&t0, Tu &&...t, R &&r) {
    if constexpr (pos == (std::tuple_size_v<std::decay_t<Tu0>> - 1))
      return f(std::get<pos>(std::forward<Tu0>(t0)), std::get<pos>(std::forward<Tu>(t))..., std::forward<R>(r));
    else
      return fold_impl<pos + 1>(std::forward<F>(f), std::forward<Tu0>(t0), std::forward<Tu>(t)...,
                                f(std::get<pos>(std::forward<Tu0>(t0)), std::get<pos>(std::forward<Tu>(t))..., std::forward<R>(r)));
  }

  template <typename F, typename R, Tuple... Tu> decltype(auto) fold(F &&f, Tu &&...t, R &&r) {
    return fold_impl<0>(std::forward<F>(f), std::forward<Tu>(t)..., std::forward<R>(r));
  }

#endif

  /*
   * replace<int ... I>(t,r)
   *  Given a tuple t, and integers, returns the tuple where the elements at initial position I are replaced by r
   */
  template <int... N, Tuple Tu, typename R> auto replace(Tu &&tu, R const &r) {
    return [&]<auto... Is>(std::index_sequence<Is...>) {
      auto choose = []<int J>(std::integral_constant<int, J>, auto &&x, auto &&y) {
        if constexpr (((J == N) or ...)) // if J is one of the N
          return x;
        else
          return y;
      };
      return std::make_tuple(choose(std::integral_constant<int, Is>{}, r, std::get<Is>(std::forward<Tu>(tu)))...);
    }
    (std::make_index_sequence<std::tuple_size_v<std::decay_t<Tu>>>{});
  }

} // namespace triqs::tuple
