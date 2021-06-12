// Copyright (c) 2019-2020 Simons Foundation
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

// ************************************************
//  operator ()
// ************************************************

template <typename... Args> decltype(auto) operator()(Args &&... args) const & {
  if constexpr (sizeof...(Args) == 0)
    return const_view_type{*this};
  else {
    static_assert((sizeof...(Args) == evaluator_t::arity) or (evaluator_t::arity == -1), "Incorrect number of arguments");
    if constexpr ((... or clef::Lazy<Args>)) // any argument is lazy ?
      return clef::make_expr_call(*this, std::forward<Args>(args)...);
    else
      return evaluator_t()(*this, std::forward<Args>(args)...);
  }
}
template <typename... Args> decltype(auto) operator()(Args &&... args) & {
  if constexpr (sizeof...(Args) == 0)
    return view_type{*this};
  else {
    static_assert((sizeof...(Args) == evaluator_t::arity) or (evaluator_t::arity == -1), "Incorrect number of arguments");
    if constexpr ((... or clef::Lazy<Args>)) // any argument is lazy ?
      return clef::make_expr_call(*this, std::forward<Args>(args)...);
    else
      return evaluator_t()(*this, std::forward<Args>(args)...);
  }
}
template <typename... Args> decltype(auto) operator()(Args &&... args) && {
  if constexpr (sizeof...(Args) == 0)
    return view_type{std::move(*this)};
  else {
    static_assert((sizeof...(Args) == evaluator_t::arity) or (evaluator_t::arity == -1), "Incorrect number of arguments");
    if constexpr ((... or clef::Lazy<Args>)) // any argument is lazy ?
      return clef::make_expr_call(std::move(*this), std::forward<Args>(args)...);
    else
      return evaluator_t()(std::move(*this), std::forward<Args>(args)...);
  }
}

// ************************************************
//  operator []
// ************************************************

private:
#ifdef NDA_ENFORCE_BOUNDCHECK
static constexpr bool has_no_boundcheck = false;
#else
static constexpr bool has_no_boundcheck = true;
#endif

// ------------- helper  call_data  data[i] or data[tuple ...]

// Self can not be a rvalue, since we return a view !
// similar coding as for nda
template <typename Self> FORCEINLINE static decltype(auto) call_data(Self &&self, long i) noexcept(has_no_boundcheck) {
  return data_t::template call<(target_t::is_matrix ? 'M' : 'A'), false>(std::forward<Self>(self)._data, i, ellipsis{});
}

template <typename Self, typename... T>
FORCEINLINE static decltype(auto) call_data(Self &&self, std::tuple<T...> const &tu) noexcept(has_no_boundcheck) {
  return [&]<auto... Is>(std::index_sequence<Is...>)->decltype(auto) {
    return data_t::template call<(target_t::is_matrix ? 'M' : 'A'), false>(std::forward<Self>(self)._data, std::get<Is>(tu)..., ellipsis{});
  }
  (std::make_index_sequence<sizeof...(T)>{});
}

public:
// ------------- g[ mesh_index_t]
// transform the mesh_index_t into a linear_mesh_index_t and pass it to the data array
// mesh_index_t can be a long, or a tuple, etc.. whatever the mesh says

decltype(auto) operator[](mesh_index_t const &idx) {
  EXPECTS(_mesh.is_within_boundary(idx));
  return call_data(*this, _mesh.index_to_linear(idx));
}

// same code for const call
decltype(auto) operator[](mesh_index_t const &idx) const {
  EXPECTS(_mesh.is_within_boundary(idx));
  return call_data(*this, _mesh.index_to_linear(idx));
}

// ------------- g[ mesh_point_t]
// transform the mesh_point_t into a linear_mesh_index_t and pass it to the data array

// pass a mesh_point of the mesh
decltype(auto) operator[](mesh_point_t const &x) {
#ifdef TRIQS_DEBUG
  if (!mesh_point_compatible_to_mesh(x, _mesh)) TRIQS_RUNTIME_ERROR << "gf[ ] : mesh point's mesh and gf's mesh mismatch";
#endif
  return call_data(*this, x.linear_index());
}

decltype(auto) operator[](mesh_point_t const &x) const {
#ifdef TRIQS_DEBUG
  if (!mesh_point_compatible_to_mesh(x, _mesh)) TRIQS_RUNTIME_ERROR << "gf[ ] : mesh point's mesh and gf's mesh mismatch";
#endif
  return call_data(*this, x.linear_index());
}

// ------------- g[ closest_mesh_pt(...) ]
// pass an abtract closest_point. We extract the value of the domain from p, call the gf_closest_point trait

template <typename... U> decltype(auto) operator[](closest_pt_wrap<U...> const &p) {
  return call_data(*this, _mesh.index_to_linear(mesh::closest_point<Mesh, Target>::invoke(_mesh, p)));
}
template <typename... U> decltype(auto) operator[](closest_pt_wrap<U...> const &p) const {
  return call_data(*this, _mesh.index_to_linear(mesh::closest_point<Mesh, Target>::invoke(_mesh, p)));
}

// -------------- g[ nda::clef::comma_tuple] . Ersatz of [x,y,z,..]
private:
// the implementation is similar for the 3 calls, we factorize it as usual with the Self mechanism
template <typename Self, typename... U> static decltype(auto) subscript_impl(Self &&self, nda::clef::comma_tuple<U...> const &tu) {
  static_assert(sizeof...(U) == arity, "Incorrect number of argument in [] operator");
  if constexpr ((... or clef::Lazy<U>)) // any argument is lazy ?
    return clef::make_expr_subscript(std::forward<Self>(self), tu);
  else
    return [&]<auto... Is>(std::index_sequence<Is...>) mutable->decltype(auto) {
      return details::slice_or_access(std::forward<Self>(self),
                                      details::get_linidx(std::get<Is>(std::forward<Self>(self).mesh()), std::get<Is>(tu._t))...);
    }
  (std::make_index_sequence<arity>{});
}

public:
template <typename... U> decltype(auto) operator[](nda::clef::comma_tuple<U...> const &tu) & { return subscript_impl(*this, tu); }
template <typename... U> decltype(auto) operator[](nda::clef::comma_tuple<U...> const &tu) const & { return subscript_impl(*this, tu); }
template <typename... U> decltype(auto) operator[](nda::clef::comma_tuple<U...> const &tu) && { return subscript_impl(std::move(*this), tu); }

// ------------- g[lazy expression] -----------------------------
// one argument only. The multiple argument is handled by the comma_tuple call above

template <clef::Lazy Arg> auto operator[](Arg &&arg) const & { return clef::make_expr_subscript(*this, std::forward<Arg>(arg)); }
template <clef::Lazy Arg> auto operator[](Arg &&arg) & { return clef::make_expr_subscript(*this, std::forward<Arg>(arg)); }
template <clef::Lazy Arg> auto operator[](Arg &&arg) && { return clef::make_expr_subscript(std::move(*this), std::forward<Arg>(arg)); }

// --------------------- A direct access to the grid point --------------------------

template <typename... Args> FORCEINLINE decltype(auto) on_mesh(Args &&... args) {
  return call_data(*this, _mesh.index_to_linear(mesh_index_t(std::forward<Args>(args)...)));
}

template <typename... Args> FORCEINLINE decltype(auto) on_mesh(Args &&... args) const {
  return call_data(*this, _mesh.index_to_linear(mesh_index_t(std::forward<Args>(args)...)));
}

template <typename... Args> FORCEINLINE decltype(auto) on_mesh_from_linear_index(Args &&... args) {
  return call_data(*this, linear_mesh_index_t(std::forward<Args>(args)...));
}

template <typename... Args> FORCEINLINE decltype(auto) on_mesh_from_linear_index(Args &&... args) const {
  return call_data(*this, linear_mesh_index_t(std::forward<Args>(args)...));
}

// ************************************************
//       HDF5 
// ************************************************

/// HDF5 name
static std::string hdf5_format() { return "Gf"; }

friend struct gf_h5_rw<Mesh, Target>;

/// Write into HDF5
friend void h5_write(h5::group fg, std::string const &subgroup_name, this_t const &g) {
  auto gr = fg.create_group(subgroup_name);
  write_hdf5_format(gr, g);
  gf_h5_rw<Mesh, Target>::write(gr, g);
}

/// Read from HDF5
friend void h5_read(h5::group fg, std::string const &subgroup_name, this_t &g) {
  auto gr       = fg.open_group(subgroup_name);
  auto tag_file = read_hdf5_format(gr);
  if (!(tag_file[0] == 'G' and tag_file[1] == 'f'))
    TRIQS_RUNTIME_ERROR << "h5_read : For a Green function, the type tag should be Gf (or Gfxxxx for old archive) "
                        << " while I found " << tag_file;
  gf_h5_rw<Mesh, Target>::read(gr, g);
}

//-----------------------------  BOOST Serialization -----------------------------
private:
friend class boost::serialization::access;

public:
/// The serialization as required by Boost
template <class Archive> void serialize(Archive &ar, const unsigned int version) {
  ar &_data;
  ar &_mesh;
  ar &_indices;
}

//----------------------------- print  -----------------------------

/// IO
friend std::ostream &operator<<(std::ostream &out, this_t const &x) { return out << "this_t"; }

//----------------------------- MPI  -----------------------------

/**
* Initiate (lazy) MPI broadcast 
*
* When the returned object is used at the RHS of operator = or in a constructor of a gf,
* the MPI  operation is performed.
*
* @group MPI
* @param g The Green function
* @param c The MPI communicator (default is world)
* @param root The root of the broadcast communication in the MPI sense.
* @return Returns a lazy object describing the object and the MPI operation to be performed.
*
*/
friend void mpi_broadcast(this_t &g, mpi::communicator c = {}, int root = 0) {
  // Shall we bcast mesh ?
  mpi::broadcast(g.data(), c, root);
}

friend mpi::lazy<mpi::tag::reduce, const_view_type> mpi_reduce(this_t const &a, mpi::communicator c = {}, int root = 0, bool all = false,
                                                               MPI_Op op = MPI_SUM) {
  return {a(), c, root, all, op};
}

friend mpi::lazy<mpi::tag::scatter, const_view_type> mpi_scatter(this_t const &a, mpi::communicator c = {}, int root = 0) {
  return {a(), c, root, true};
}

friend mpi::lazy<mpi::tag::gather, const_view_type> mpi_gather(this_t const &a, mpi::communicator c = {}, int root = 0, bool all = false) {
  return {a(), c, root, all};
}
