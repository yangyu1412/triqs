// Copyright (c) 2015-2018 Commissariat à l'énergie atomique et aux énergies alternatives (CEA)
// Copyright (c) 2015-2018 Centre national de la recherche scientifique (CNRS)
// Copyright (c) 2016 Igor Krivenko
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
// Authors: Michel Ferrero, Igor Krivenko, Olivier Parcollet, Nils Wentzell

#pragma once
#include "./fundamental_operator_set.hpp"
#include "../operators/many_body_operator.hpp"
#include "./hilbert_space.hpp"

#include <vector>
#include <utility>
#include <algorithm>

// Workaround for GCC bug 41933
#if defined GCC_VERSION && GCC_VERSION < 40900
#define GCC_BUG_41933_WORKAROUND
#endif

#ifdef GCC_BUG_41933_WORKAROUND
#include <triqs/utility/tuple_tools.hpp>
#endif

namespace triqs {
  namespace hilbert_space {

    /*
   If UseMap is false, the constructor takes two arguments:

   imperative_operator(many_body_op, fundamental_ops)

   a trivial identity connection map is then used.

   If UseMap is true, the constructor takes three arguments:

   imperative_operator(many_body_op, fundamental_ops, hilbert_map)
   */

    /// This class is the imperative version of the [[many_body_operator]]
    /**
  It is created from a `many_body_operator` object, and is used to act on a [[state]].
  There is an optimization option `UseMap` (useful when `HilbertType = sub_hilbert_space`),
  which allows the user to give a map describing the connections between Hilbert subspaces generated by this operator.
  @warning `HilbertType = sub_hilbert_space` implies that the operator generates only one-to-one connections between the used subspaces. If this not the case, one has to use `HilbertType = hilbert_space`.
  @tparam HilbertType Hilbert space type, one of [[hilbert_space]] and [[sub_hilbert_space]]
  @tparam ScalarType Type of operator's coefficients, normally `double` or `std::complex<double>`
  @tparam UseMap Use a user-provided connection map on construction
  @include triqs/hilbert_space/imperative_operator.hpp
 */
    template <typename HilbertType, typename ScalarType = double, bool UseMap = false> class imperative_operator {

      // Fock state convention:
      // |0,...,k> = C^+_0 ... C^+_k |0>
      // Operator monomial convention:
      // C^+_0 ... C^+_i ... C_j  ... C_0

      using scalar_t = ScalarType;

      struct one_term_t {
        scalar_t coeff;
        uint64_t d_mask, dag_mask, d_count_mask, dag_count_mask;
      };
      std::vector<one_term_t> all_terms;

      std::vector<sub_hilbert_space> const *sub_spaces;
      using hilbert_map_t = std::vector<int>;
      hilbert_map_t hilbert_map;

      public:
      /// Construct a zero operator
      imperative_operator() {}

      /// Constructor from a `many_body_operator` and a `fundamental_operator_set`
      /**
   @param op Source `many_body_operator` object
   @param fops [[fundamental_operator_set]]; must contain all index sequences met in `op`
   @param hmap Map of subspaces-to-subspaces connections generated by `op` (only for `UseMap = true`)
   @param sub_spaces_set Pointer to a vector of all Hilbert subspaces referred by `hmap` (only for `UseMap = true`)
  */
      imperative_operator(triqs::operators::many_body_operator_generic<scalar_t> const &op, fundamental_operator_set const &fops,
                          hilbert_map_t hmap = hilbert_map_t(), std::vector<sub_hilbert_space> const *sub_spaces_set = nullptr) {

        sub_spaces  = sub_spaces_set;
        hilbert_map = hmap;
        if ((hilbert_map.size() == 0) != !UseMap) TRIQS_RUNTIME_ERROR << "Internal error";

        auto greater = [&fops](triqs::operators::canonical_ops_t const& op1,
                               triqs::operators::canonical_ops_t const& op2) {
          if(op1.dagger != op2.dagger) return op2.dagger;
          return op1.dagger ? (fops[op1.indices] > fops[op2.indices]) :
                              (fops[op1.indices] < fops[op2.indices]);
        };

        // The goal here is to have a transcription of the many_body_operator in terms
        // of simple vectors (maybe the code below could be more elegant)
        for (auto const& term : op) {
          auto monomial = term.monomial;
          auto coef = term.coef;

          // Sort monomial according to the order established by fops
          int n = monomial.size();
          bool swapped;
          do {
            swapped = false;
            for(int i = 1; i < n; ++i) {
              if(greater(monomial[i - 1], monomial[i])) {
                using std::swap;
                swap(monomial[i - 1], monomial[i]);
                swapped = true;
                coef *= scalar_t(-1);
              }
            }
            --n;
          } while(swapped);

          // Given the environment variable CHECK_ISSUE819 was set by the user
          // throw an exception if the result of this model was effected by issue 819
          // https://github.com/TRIQS/triqs/issues/819
          static const bool check_issue819 = std::getenv("CHECK_ISSUE819");
          if (check_issue819 && term.coef != coef)
            TRIQS_RUNTIME_ERROR << "ERROR: The Atom-Diag result of this model is affected by issue 819 (https://github.com/TRIQS/triqs/issues/819).\n"
                                   "If you have solved the same model with release 2.2.0, 2.2.1 or 3.0.0 of TRIQS the result was incorrect.";

          std::vector<int> dag, ndag;
          uint64_t d_mask = 0, dag_mask = 0;
          for (auto const &canonical_op : monomial) {
            (canonical_op.dagger ? dag : ndag).push_back(fops[canonical_op.indices]);
            (canonical_op.dagger ? dag_mask : d_mask) |= (uint64_t(1) << fops[canonical_op.indices]);
          }
          auto compute_count_mask = [](std::vector<int> const &d) {
            uint64_t mask = 0;
            bool is_on    = (d.size() % 2 == 1);
            for (int i = 0; i < 64; ++i) {
              if (std::find(begin(d), end(d), i) != end(d))
                is_on = !is_on;
              else if (is_on)
                mask |= (uint64_t(1) << i);
            }
            return mask;
          };
          uint64_t d_count_mask = compute_count_mask(ndag), dag_count_mask = compute_count_mask(dag);
          all_terms.push_back(one_term_t{scalar_t(coef), d_mask, dag_mask, d_count_mask, dag_count_mask});
        }
      }

      /// Apply a callable object to each coefficient of the operator by reference
      /**
   The callable object must take one argument convertible to `ScalarType &`

   @tparam Lambda Type of the callable object
   @param L Callable object
  */
      template <typename Lambda> void update_coeffs(Lambda L) {
        for (auto &M : all_terms) L(M.coeff);
      }

      private:
      template <typename StateType> StateType get_target_st(StateType const &st, std::true_type use_map) const {
        auto n = hilbert_map[st.get_hilbert().get_index()];
        if (n == -1) return StateType{};
        return StateType{(*sub_spaces)[n]};
      }

      template <typename StateType> StateType get_target_st(StateType const &st, std::false_type use_map) const {
        return StateType(st.get_hilbert());
      }

      static bool parity_number_of_bits(uint64_t v) {
        // http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetNaive
        // v ^= v >> 16;
        // only ok until 16 orbitals ! assert this or put the >> 16
        v ^= v >> 8;
        v ^= v >> 4;
        v ^= v >> 2;
        v ^= v >> 1;
        return v & 0x01;
      }

      // Forward the call to the coefficient
#ifdef GCC_BUG_41933_WORKAROUND
      template <typename... Args>
      static auto apply_if_possible(scalar_t const &x, std::tuple<Args...> const &args_tuple) -> typename std::invoke_result_t<scalar_t, Args...> {
        return triqs::tuple::apply(x, args_tuple);
      }
      static auto apply_if_possible(scalar_t const &x, std::tuple<> const &) -> scalar_t { return x; }
#else
      template <typename... Args>
      static auto apply_if_possible(scalar_t const &x, Args &&... args) -> std::invoke_result_t<scalar_t, Args...> {
        return x(std::forward<Args>(args)...);
      }
      static auto apply_if_possible(scalar_t const &x) -> scalar_t { return x; }
#endif

      public:
      /// Act on a state and return a new state
      /**
   The optional extra arguments `args...` are forwarded to the coefficients of the operator.

   `auto psi = op(phi,args...);`

   We apply an operator obtained from `op` by replacing its monomial coefficients with values
   returned by `coeff(args...)`. This feature makes sense only for ScalarType being a callable object.

   @tparam StateType Type of the initial state
   @tparam Args Types of the optional arguments
   @param st Initial state
   @param args Optional argument pack passed to each coefficient of the operator
  */
      template <typename StateType, typename... Args> StateType operator()(StateType const &st, Args &&... args) const {

        StateType target_st = get_target_st(st, std::integral_constant<bool, UseMap>());
        auto const &hs      = st.get_hilbert();

#ifdef GCC_BUG_41933_WORKAROUND
        auto args_tuple = std::make_tuple(args...);
#endif

        using amplitude_t = typename StateType::value_type;

        for (int i = 0; i < all_terms.size(); ++i) { // loop over monomials
          auto M = all_terms[i];
#ifdef GCC_BUG_41933_WORKAROUND
          foreach (st, [M, &target_st, hs, args_tuple](int i, typename StateType::value_type amplitude) {
#else
          foreach (st, [M, &target_st, hs, args...](int i, typename StateType::value_type amplitude) {
#endif
            fock_state_t f2 = hs.get_fock_state(i);
            if ((f2 & M.d_mask) != M.d_mask) return;
            f2 &= ~M.d_mask;
            if (((f2 ^ M.dag_mask) & M.dag_mask) != M.dag_mask) return;
            fock_state_t f3    = ~(~f2 & ~M.dag_mask);
            auto sign_is_minus = parity_number_of_bits((f2 & M.d_count_mask) ^ (f3 & M.dag_count_mask));
            // update state vector in target Hilbert space
            auto ind = target_st.get_hilbert().get_state_index(f3);
#ifdef GCC_BUG_41933_WORKAROUND
            target_st(ind) += amplitude * apply_if_possible(M.coeff, args_tuple) * (sign_is_minus ? -amplitude_t(1) : amplitude_t(1));
#else
            target_st(ind) += amplitude * apply_if_possible(M.coeff, args...) * (sign_is_minus ? -amplitude_t(1) : amplitude_t(1));
#endif
          })
            ; // foreach
        }
        return target_st;
      }
    };
  } // namespace hilbert_space
} // namespace triqs
