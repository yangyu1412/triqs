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

  template <GfContainer G, typename RHS, clef::CallOrSubscriptTag Tag, typename PhList, typename... OtherTagAndPhList>
  void clef_auto_assign(G &g, RHS &&rhs, Tag, PhList phl, OtherTagAndPhList... rest) {

    auto f = nda::clef::make_function(std::forward<RHS>(rhs), phl);
    static_assert(PhList::size == G::arity, "Incorrect number of argument in lazy call");

    for (auto const &w : g.mesh()) {
      if constexpr (sizeof...(OtherTagAndPhList) > 0) {
        if constexpr (mesh::is_product_v<typename G::mesh_t>)
          clef_auto_assign(g[w], std::apply(f, w.components_tuple()), rest...);
        else
          clef_auto_assign(g[w], f(w), rest...);
      } else {
        if constexpr (mesh::is_product_v<typename G::mesh_t>)
          g[w] = std::apply(f, w.components_tuple());
        else
          g[w] = f(w);
      }
    }
  }
} // namespace triqs::gfs
