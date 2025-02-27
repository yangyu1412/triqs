// Copyright (c) 2013-2018 Commissariat à l'énergie atomique et aux énergies alternatives (CEA)
// Copyright (c) 2013-2018 Centre national de la recherche scientifique (CNRS)
// Copyright (c) 2018 Simons Foundation
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
// Authors: Olivier Parcollet, Nils Wentzell

#ifndef TRIQS_ARRAYS_EXPRESSION_FOLD_H
#define TRIQS_ARRAYS_EXPRESSION_FOLD_H
#include "../array.hpp"

namespace triqs {
  namespace arrays {

#ifndef TRIQS_ARRAY_USE_NEw_FOLD
    template <class F> struct fold_worker {
      F f;

      template <class A, class R> struct fold_func_adaptor {
        F const &f;
        A const &a;
        R &r;
        template <typename... Args> void operator()(Args const &... args) { r = f(r, a(args...)); }
      };

      template <class A, class R>
      auto operator()(A const &a, R init) const -> typename std::decay<typename std::invoke_result_t<F, R, typename A::value_type>>::type {
        // to take into account that f may be double,double -> double, while one passes 0 (an int...)
        // R = int, R2= double in such case, and the result will be a double, or narrowing will occur
        using R2 = typename std::decay<typename std::invoke_result_t<F, R, typename A::value_type>>::type;
        R2 r2    = init;
        foreach (a, fold_func_adaptor<A, R2>{f, a, r2})
          ;
        return r2;
      }

      template <class A> typename A::value_type operator()(A const &a) const { return (*this)(a, typename A::value_type{}); }
    };

    template <class F> fold_worker<F> fold(F f) { return {std::move(f)}; }

#else
    template <typename A, typename F, typename R> auto fold(F f, A &&a, R r) {
      decltype(f(r, typename A::value_type{})) r2 = r;
      // to take into account that f may be double,double -> double, while one passes 0 (an int...)
      // R = int, R2= double in such case, and the result will be a double, or narrowing will occur
      arrays::foreach (a, [&](auto &&... args) { r2 = f(r2, a(args...)); });
      return r2;
    }

    template <typename A, typename F> R fold(F f, A &&a) { return fold(std::move(f), std::forward<A>(a), typename A::value_type{}); }

#endif

  } // namespace arrays
} // namespace triqs

#endif
