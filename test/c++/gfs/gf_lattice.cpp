// Copyright (c) 2018 Commissariat à l'énergie atomique et aux énergies alternatives (CEA)
// Copyright (c) 2018 Centre national de la recherche scientifique (CNRS)
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
// Authors: Olivier Parcollet, Nils Wentzell

#define TRIQS_ARRAYS_ENFORCE_BOUNDCHECK
#include <triqs/test_tools/gfs.hpp>

using index_t = utility::mini_vector<long, 3>;
using K_t     = std::array<double, 3>;

TEST(Gfs, cyclic_lattice) {
  double beta = 1;
  triqs::clef::placeholder<0> r_;

  auto bl  = bravais_lattice{make_unit_matrix<double>(2)};
  auto m_r = gf_mesh<cyclic_lattice>{bl, 2};
  ASSERT_EQ(m_r.size(), 4);

  auto Gr = gf<cyclic_lattice>{m_r, {2, 2}};

  // Clef Assignment
  Gr(r_) << 1. + r_(0) + r_(1);
  EXPECT_COMPLEX_NEAR(Gr(index_t{0, 0, 0})(0, 0), 1);
  Gr(r_) << exp(-r_(0));

  std::cout << Gr.mesh() << "\n";
}

// FIXME : Test matrix case with same code ?
TEST(Gfs, brillouin_zone) {
  double beta = 1;
  triqs::clef::placeholder<0> k_;

  int n_k = 50;

  auto bz  = brillouin_zone{bravais_lattice{make_unit_matrix<double>(2)}};
  auto m_k = gf_mesh<brillouin_zone>{bz, n_k};
  ASSERT_EQ(m_k.size(), n_k * n_k);

  auto Gk = gf<brillouin_zone, scalar_valued>{m_k}; //, {2, 2}};

  //Gk(k_) << -2 * (cos(k_(0)) + cos(k_(1)));
  for (auto &&k : Gk.mesh()) Gk[k] = -2 * (cos(k(0)) + cos(k(1)));

  ASSERT_EQ(Gk.mesh().locate_neighbours(arrays::vector<double>{0, 0, 0}), (index_t({0, 0, 0})));
  EXPECT_COMPLEX_NEAR(Gk(K_t{0, 0, 0}), -4);
  EXPECT_COMPLEX_NEAR(Gk(K_t{M_PI, M_PI, M_PI}), 4);
  EXPECT_COMPLEX_NEAR(Gk(K_t{2 * M_PI, 2 * M_PI, 2 * M_PI}), -4);

  // reevaluate on the mesh itself.
  for (auto &&k : Gk.mesh()) {
    EXPECT_NEAR(k(0), k.index()[0] / double(n_k) * 2 * M_PI, 1.e-14);
    EXPECT_NEAR(k(1), k.index()[1] / double(n_k) * 2 * M_PI, 1.e-14);
    double res = -2 * (cos(k(0)) + cos(k(1)));
    EXPECT_COMPLEX_NEAR(Gk(K_t{k(0), k(1), 0}), res, 1.e-14);
  }

  // evaluate on a larger grid
  int n_k2 = 3 * n_k;
  for (int nkx = 0; nkx < n_k2; ++nkx)
    for (int nky = 0; nky < n_k2; ++nky) {
      double kx  = nkx / double(n_k2) * 2 * M_PI;
      double ky  = nky / double(n_k2) * 2 * M_PI;
      double res = -2 * (cos(kx) + cos(ky));
      EXPECT_COMPLEX_NEAR(Gk(K_t{kx, ky, 0}), res, 0.01);
    }
}

// ----------------------------------------------------

TEST(Gfs, brillouin_zoneMatrix) {
  double beta = 1;
  triqs::clef::placeholder<0> k_;

  int n_k = 50;

  auto bz  = brillouin_zone{bravais_lattice{make_unit_matrix<double>(2)}};
  auto m_k = gf_mesh<brillouin_zone>{bz, n_k};
  ASSERT_EQ(m_k.size(), n_k * n_k);

  auto Gk = gf<brillouin_zone, matrix_valued>{m_k, {2, 2}};

  //Gk(k_) << -2 * (cos(k_(0)) + cos(k_(1)));
  for (auto &&k : Gk.mesh()) Gk[k] = -2 * (cos(k(0)) + cos(k(1)));

  ASSERT_EQ(Gk.mesh().locate_neighbours(arrays::vector<double>{0, 0, 0}), (index_t({0, 0, 0})));
  //auto a = Gk(index_t{0, 0, 0});
  auto a = Gk(K_t{0, 0, 0});

  TRIQS_PRINT(a);
  EXPECT_COMPLEX_NEAR(a(0, 0), -4);

  // reevaluate on the mesh itself.
  for (auto &&k : Gk.mesh()) {
    EXPECT_NEAR(k(0), k.index()[0] / double(n_k) * 2 * M_PI, 1.e-14);
    EXPECT_NEAR(k(1), k.index()[1] / double(n_k) * 2 * M_PI, 1.e-14);
    double res = -2 * (cos(k(0)) + cos(k(1)));
    EXPECT_COMPLEX_NEAR(Gk(K_t{k(0), k(1), 0})(0, 0), res, 1.e-14);
  }

  // evaluate on a larger grid
  int n_k2 = 3 * n_k;
  for (int nkx = 0; nkx < n_k2; ++nkx)
    for (int nky = 0; nky < n_k2; ++nky) {
      double kx  = nkx / double(n_k2) * 2 * M_PI;
      double ky  = nky / double(n_k2) * 2 * M_PI;
      double res = -2 * (cos(kx) + cos(ky));
      EXPECT_COMPLEX_NEAR(Gk(K_t{kx, ky, 0})(0, 0), res, 0.01);
    }

  std::cout << Gk.mesh() << "\n";
}

// ----------------------------------------------------

TEST(Gfs, brillouin_zone_triangular) {
  double beta = 1;
  int n_k     = 50;

  // Tilted Triangular Lattice
  auto units = arrays::matrix<double>(2,2);
  units(0,0) = 1; units(0,1) = 0.5;
  units(1,0) = 0; units(1,1) = sqrt(3) / 2.0;
  auto bl = bravais_lattice{units};
  auto bz = brillouin_zone{bl};

  auto k_mesh = gf_mesh<brillouin_zone>{bz, n_k};
  ASSERT_EQ(k_mesh.size(), n_k * n_k);
  auto Gk = gf<brillouin_zone, scalar_valued>{k_mesh};

  // Represent k in reciprocal basis and evaluate dispersion
  auto disp = [&](gf_mesh<brillouin_zone>::point_t k) {
    auto k_rec_x_2PI = bl.units() * k;
    return -2 * (cos(k_rec_x_2PI(0)) + cos(k_rec_x_2PI(1)));
  };
  for (auto &k : Gk.mesh()) Gk[k] = disp(k);

  ASSERT_EQ(Gk.mesh().locate_neighbours(arrays::vector<double>{0, 0, 0}), (index_t{0, 0, 0}));

  auto K       = bz.reciprocal_matrix();
  auto Kx_half = K_t{0.5 * K(0, 0), 0.5 * K(0, 1), 0.5 * K(0, 2)};
  auto Ky_half = K_t{0.5 * K(1, 0), 0.5 * K(1, 1), 0.5 * K(1, 2)};
  auto Kx      = K_t{K(0, 0), K(0, 1), K(0, 2)};
  auto Ky      = K_t{K(1, 0), K(1, 1), K(1, 2)};

  EXPECT_COMPLEX_NEAR(Gk(K_t{0, 0, 0}), -4);
  EXPECT_COMPLEX_NEAR(Gk(Kx), -4, 1e-14);
  EXPECT_COMPLEX_NEAR(Gk(Ky), -4, 1e-14);
  EXPECT_COMPLEX_NEAR(Gk(Kx_half), 0.0, 1e-14);
  EXPECT_COMPLEX_NEAR(Gk(Ky_half), 0.0, 1e-14);

  // Evaluate on the mesh itself.
  for (auto &k : Gk.mesh()) {
    double res = disp(k);
    EXPECT_COMPLEX_NEAR(Gk(K_t{k(0), k(1), k(2)}), res, 1.e-14);
  }

  // Evaluate on a larger grid
  int n_k2 = 3 * n_k;
  for (int nkx = 0; nkx < n_k2; ++nkx)
    for (int nky = 0; nky < n_k2; ++nky) {
      auto k_rec = gf_mesh<brillouin_zone>::point_t{nkx / double(n_k2), nky / double(n_k2), 0.0};
      auto k     = bz.reciprocal_matrix() * k_rec;
      double res = disp(k);
      EXPECT_COMPLEX_NEAR(Gk(K_t{k(0), k(1), k(2)}), res, 0.01);
    }
}

MAKE_MAIN;
