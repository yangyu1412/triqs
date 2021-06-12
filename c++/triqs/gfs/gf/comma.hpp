/*******************************************************************************
 *
 * TRIQS: a Toolbox for Research in Interacting Quantum Systems
 *
 * Copyright (C) 2012-2016 by O. Parcollet
 *
 * TRIQS is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * TRIQS is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * TRIQS. If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/
#pragma once
#include <triqs/mesh/details/mesh_tools.hpp>

namespace triqs::mesh {

  /* ---------------------------------------------------------------------------------------------------
  * Overload comma operator for a few types
  *  --------------------------------------------------------------------------------------------------- */

  // any type tagged with this will overload , operator and make a comma tuple
  template <typename T> inline constexpr bool overloads_comma_v = false;

  template <> inline  constexpr bool overloads_comma_v<all_t>                   = true;
  template <> inline constexpr bool overloads_comma_v<matsubara_freq>          = true;
  template <typename M> inline constexpr bool overloads_comma_v<mesh_point<M>> = true;

  template <typename T, typename U>
  requires(overloads_comma_v<std::decay_t<T>> or overloads_comma_v<std::decay_t<U>>) //
     nda::clef::comma_tuple<std::decay_t<T>, std::decay_t<U>>
     operator,(T &&x, U &&u) {
    return {std::make_tuple(x, u)};
    //return {x, u};
  }

} // namespace nda::clef
