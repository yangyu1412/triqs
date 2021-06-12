// Copyright (c) 2016-2018 Commissariat à l'énergie atomique et aux énergies alternatives (CEA)
// Copyright (c) 2016-2018 Centre national de la recherche scientifique (CNRS)
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
// Authors: Michel Ferrero, Olivier Parcollet, Nils Wentzell

#pragma once

namespace triqs::gfs {

  /*------------------------------------------------------------------------------------------------------
  *             Interaction with the CLEF library : auto assignment implementation
  *-----------------------------------------------------------------------------------------------------*/

  // cf nda::basic_function for the nda::array clef_auto_assign
  // we ignore the Tag: accept () and []
  template <GfBlockContainer G, clef::Lazy RHS, clef::CallOrSubscriptTag Tag, typename PhList, typename... OtherTagAndPhList>
  void clef_auto_assign(G &g, RHS &&rhs, Tag, PhList phl, OtherTagAndPhList... rest) {

    static_assert(PhList::size == G::arity, "Incorrect number of argument in lazy call");

    auto f = nda::clef::make_function(std::forward<RHS>(rhs), phl);

    if constexpr (G::arity == 1) {
      for (int i = 0; i < g.size(); ++i) {
        if constexpr (sizeof...(OtherTagAndPhList) > 0)
          clef_auto_assign(g[i], f(i), rest...);
        else
          g[i] = f(i);
      }
    } else {
      for (int i = 0; i < g.size1(); ++i)
        for (int j = 0; j < g.size2(); ++j) {
          if constexpr (sizeof...(OtherTagAndPhList) > 0)
            clef_auto_assign(g(i, j), f(i,j), rest...);
          else
            g(i, j) = f(i,j);
        }
    }
  }
} // namespace triqs::gfs
