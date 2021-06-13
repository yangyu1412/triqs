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

#include <triqs/utility/tuple_tools.hpp>
#include <cmath>
#include <stdexcept>
#include <functional>
#include <string>

struct fun {
  double operator()(int i, double x, double y, int k) { return 6 * k + i - 1.3 * x + 2 * y; }
};

struct print_t {
  template <typename T> void operator()(T x) { std::cerr << x << " "; }
  std::string s;
};

struct A {
  template <typename T1, typename T2> std::string str(T1 const &x, T2 const &y) const {
    std::stringstream fs;
    fs << "A : the string is " << x << " " << y;
    return fs.str();
  }
};

std::string my_print_str(int x, int y) {
  std::stringstream fs;
  fs << "the string is " << x << " " << y;
  return fs.str();
}

int main(int argc, char **argv) {

  auto t  = std::make_tuple(1, 2.3, 4.3, 8);
  auto t2 = std::make_tuple(1, 2, 3, 4);
  auto t1 = std::make_tuple(1, 2.3, 4.3, 8);

  {
    triqs::tuple::for_each(t, print_t());
    std::cerr << std::endl;
  }

  {
    auto t = [x = 4]<auto... Is>(std::index_sequence<Is...>) { return std::make_tuple(((void)Is, x)...); }
    (std::make_index_sequence<3>{});
    if (t != std::make_tuple(4, 4, 4)) throw std::runtime_error(" ");
  }

  {
    auto r = triqs::tuple::map_on_zip([](double x, double y) { return x + y; }, t1, t2);
    std::cerr << " [f(a,b) for (a,b) in zip(t1,t2)] =" << r << std::endl;
  }

  std::cerr << "  ----- fold ----" << std::endl;

  {
    auto res = triqs::tuple::fold(
       [](double x, double r) {
         std::cout << x << " " << r << std::endl;
         return x + r;
       },
       t, 0);
    std::cerr << " " << res << std::endl;
    if (std::abs((res - 15.6)) > 1.e-13) throw std::runtime_error(" ");
  }

  {
    auto res = triqs::tuple::fold([](double x, double y) { return x + 2 * y; }, t, 0);
    std::cerr << " " << res << std::endl;
    if (std::abs((res - 33.8)) > 1.e-13) throw std::runtime_error(" ");
  }

  {
    auto res = triqs::tuple::fold([](double x, double y, double r) { return x + 2 * y + r; }, t1, t2, 0);
    std::cerr << " " << res << std::endl;
    if (std::abs((res - 35.6)) > 1.e-13) throw std::runtime_error(" ");
  }

  {
    // ex of real code
    auto fl = [](int i, std::string s) {
      auto r = std::to_string(i);
      return s.empty() ? r : r + "_" + s;
    };
    auto _name = [fl](auto... is) {
      auto t = std::make_tuple(is...);
      return triqs::tuple::fold(fl, t, std::string{});
    };
    auto r = _name(1, 2, 3);
    std::cerr << r << std::endl;
  }

  { // replace
    std::cout << "  ----- filter ----" << std::endl;
    auto t = std::make_tuple(0, 1, 2, 3, 4, "=5");
    auto s = std::string{"--"};
    std::cout << "replace 0,2,3" << t << triqs::tuple::replace<0, 2, 3>(t, s) << std::endl;
    std::cout << "replace 1,3,5" << t << triqs::tuple::replace<1, 3, 5>(t, s) << std::endl;
  }
}
