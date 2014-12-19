// 2D cyclic boundary conditions for libmpdata++
//
// licensing: GPU GPL v3
// copyright: University of Warsaw

#pragma once

#include <libmpdata++/bcond/detail/bcond_common.hpp>

namespace libmpdataxx
{
  namespace bcond
  {
    template<int d, typename real_t>
    class cyclic_left_2d : public detail::bcond_common<real_t>
    {
      using parent_t = detail::bcond_common<real_t>;
      using arr_t = blitz::Array<real_t, 2>;
      using parent_t::parent_t; // inheriting ctor

      public:

      void fill_halos_sclr(const arr_t &a, const rng_t &j, const bool deriv = false)
      {
	using namespace idxperm;
	a(pi<d>(this->left_halo_sclr, j)) = a(pi<d>(this->rght_intr_sclr, j));
      }

      void fill_halos_pres(const arr_t &a, const rng_t &j)
      {
        fill_halos_sclr(a, j);
      }
      
      void set_edge_pres(const arr_t &a, const rng_t &j) {}
      
      void set_edge_pres(const arr_t &a, const arr_t &b, const rng_t &j) {}

      void fill_halos_vctr_alng(const arrvec_t<arr_t> &av, const rng_t &j)
      {
	using namespace idxperm;
        av[d](pi<d>(this->left_halo_vctr, j)) = av[d](pi<d>(this->rght_intr_vctr, j));
      }

      void fill_halos_vctr_nrml(const arr_t &a, const rng_t &j)
      {
        fill_halos_sclr(a, j);
      }
    };

    template<int d, typename real_t>
    class cyclic_rght_2d : public detail::bcond_common<real_t>
    {
      using parent_t = detail::bcond_common<real_t>;
      using arr_t = blitz::Array<real_t, 2>;
      using parent_t::parent_t; // inheriting ctor

      public:

      void fill_halos_sclr(const arr_t &a, const rng_t &j, const bool deriv = false)
      {
	using namespace idxperm;
	a(pi<d>(this->rght_halo_sclr, j)) = a(pi<d>(this->left_intr_sclr, j));
      }
      
      void fill_halos_pres(const arr_t &a, const rng_t &j)
      {
        fill_halos_sclr(a, j);
      }
      
      void set_edge_pres(const arr_t &a, const rng_t &j) {}

      void set_edge_pres(const arr_t &a, const arr_t &b, const rng_t &j) {}

      void fill_halos_vctr_alng(const arrvec_t<arr_t> &av, const rng_t &j)
      {
	using namespace idxperm;
        av[d](pi<d>(this->rght_halo_vctr, j)) = av[d](pi<d>(this->left_intr_vctr, j));
      }
      
      void fill_halos_vctr_nrml(const arr_t &a, const rng_t &j)
      {
        fill_halos_sclr(a, j);
      }
    };
  }; // namespace bcond
}; // namespace libmpdataxx
