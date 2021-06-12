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

namespace triqs::gfs::details {

  using nda::range;

  //------------------   slice_or_access   -----------------------------
  // implement g[...]
  // g : a gf, gf_view, gf_const_view container.
  // args : range::all_t or linear_indices of meshes
  // returns
  //   -  A sliced g      if at least one args is all_t
  //   -  g.data[args...] otherwise
  // The sliced g is such that :
  //    - its data is the slice of the data array in the dimensions corresponding to all_t arguments
  //    - its mesh is the (product) of the mesh(es) corresponding to all_t arguments.
  //
  template <typename G, typename... Args> decltype(auto) slice_or_access(G &g, Args const &... args) {

    // arguments must be all_t or long
    static_assert(((std::is_same_v<range::all_t, Args> or std::is_same_v<long, Args>)and...), "Internal error : unexpected type in slice_or_access");

    // if no argument is a all_t, it is simple : just all the data...
    // FIXME : simplify this call ?
    if constexpr (not(std::is_same_v<range::all_t, Args> or ...))
      return g.on_mesh_from_linear_index(args...);

    else {
      // at least one argument is an all_t
      // prepare the new mesh, of size new_arity, made of all the meshes corresponding to all_t arguments.
      static constexpr int new_arity = (std::is_base_of_v<range::all_t, Args> + ...);

      // take an array of bool : true iif if the args if an all_t and
      // compute as std::array which is the list of position of the meshes to be picked up to form the new mesh of the result
      // ie the positions of the all_t in the args list
      constexpr auto compute_pos_mesh_kept = [](std::array<bool, G::arity> const &args_is_allrange) {
        std::array<int, new_arity> result = {};
        for (int n = 0, p = 0; n < G::arity; ++n)
          if (args_is_allrange[n]) result[p++] = n;
        return result;
      };

      // an array of size new_arity with the positions of the meshes to pick up to build the new mesh
      static constexpr std::array<int, new_arity> pos_mesh_kept =
         compute_pos_mesh_kept(std::array<bool, G::arity>{std::is_base_of_v<range::all_t, Args>...});

      auto make_new_mesh = [&g]() {
        if constexpr (new_arity == 1) {
          return std::get<pos_mesh_kept[0]>(g.mesh());
        } else {
          return [&g]<auto... Is>(std::index_sequence<Is...>) { return mesh::prod{std::get<pos_mesh_kept[Is]>...}; }
          (std::make_index_sequence<new_arity>{});
        }
      };

      // build the gf or view
      using mesh_t = decltype(make_new_mesh());
      if constexpr (G::is_const or std::is_const<G>::value)
        return gf_const_view<mesh_t, typename G::target_t>{make_new_mesh(), g.data()(args..., nda::ellipsis()), g.indices()};
      else
        return gf_view<mesh_t, typename G::target_t>{make_new_mesh(), g.data()(args..., nda::ellipsis()), g.indices()};
    }
  }

  //------------------   get_linidx   -----------------------------
  //
  template <typename M> FORCEINLINE auto get_linidx(M const &m, typename M::mesh_point_t const &x) { return x.linear_index(); }
  template <typename M> FORCEINLINE auto get_linidx(M const &m, typename M::index_t const &x) { return m.index_to_linear(x); }
  template <typename M> FORCEINLINE range::all_t get_linidx(M &&, all_t) { return {}; }

} // namespace triqs::gfs::details
