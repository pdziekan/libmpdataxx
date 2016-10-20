/** 
 * @file
 * @copyright University of Warsaw
 * @section LICENSE
 * GPLv3+ (see the COPYING file or http://www.gnu.org/licenses/)
 */

#include <random>
#include <cmath>
#include "pbl.hpp"
#include <libmpdata++/concurr/threads.hpp>
#include <libmpdata++/output/hdf5_xdmf.hpp>

using namespace libmpdataxx;

void test(const std::string &dirname)
{
  const int nx = 65, ny = 65, nz = 51, nt = 1500;

  struct ct_params_t : ct_params_default_t
  {
    using real_t = double;
    enum { n_dims = 3 };
    enum { n_eqns = 4 };
    enum { rhs_scheme = solvers::trapez };
    enum { vip_vab = solvers::impl };
    enum { prs_scheme = solvers::cr };
    enum { impl_tht = true };
    struct ix { enum {
      u, v, w, tht,
      vip_i=u, vip_j=v, vip_k=w, vip_den=-1
    }; };
  }; 

  using ix = typename ct_params_t::ix;

  using solver_t = output::hdf5_xdmf<pbl<ct_params_t>>;

  solver_t::rt_params_t p;
  p.n_iters = 2;
  p.dt = 10;
  p.di = 50;
  p.dj = 50;
  p.dk = 30;
  p.grid_size = {nx, ny, nz};
  p.Tht_ref = 300;
  p.g = 10;
  p.hscale = 25;
  p.cdrag = 0.;

  double mixed_length = 500;
  double st = 1e-4 / p.g;

  p.outfreq = 15;
  p.outwindow = 1;
  p.outvars = {
    {ix::u,   {.name = "u",   .unit = "m/s"}}, 
    {ix::v,   {.name = "v",   .unit = "m/s"}}, 
    {ix::w,   {.name = "w",   .unit = "m/s"}}, 
    {ix::tht,   {.name = "tht",   .unit = "K"}}, 
  };
  p.outdir = dirname;

  p.prs_tol = 1e-6;

  libmpdataxx::concurr::threads<
    solver_t, 
    bcond::cyclic, bcond::cyclic,
    bcond::cyclic, bcond::cyclic,
    bcond::rigid, bcond::rigid
  > slv(p);

  {
    // random distribution
    std::random_device rd;
    auto seed = rd();
    std::mt19937 gen(seed);
    std::uniform_real_distribution<> dis(-0.5, 0.5);

    blitz::Array<double, 3> prtrb(nx, ny, nz);
    for (int i = 0; i < nx; ++i)
    {
      for (int j = 0; j < ny; ++j)
      {
        for (int k = 0; k < nz; ++k)
        {
          prtrb(i, j, k) = dis(gen) * std::max(0.0, (1 - k * p.dk / mixed_length));
        }
      }
    }
    
    auto i_r = blitz::Range(0, nx - 1);
    auto j_r = blitz::Range(0, ny - 1);
    auto k_r = blitz::Range(0, nz - 1);

    // enforce cyclic perturbation
    prtrb(nx - 1, j_r, k_r) = prtrb(0, j_r, k_r);
    prtrb(i_r, ny - 1, k_r) = prtrb(i_r, 0, k_r);
    
    // initial conditions
    slv.advectee(ix::tht)(i_r, j_r, k_r) = 0.001 * prtrb(i_r, j_r, k_r);
    slv.advectee(ix::w)(i_r, j_r, k_r) = 0.2 * prtrb(i_r, j_r, k_r);
    slv.advectee(ix::u) = 0;
    slv.advectee(ix::v) = 0; 

    {
      blitz::thirdIndex k;
      // prescribed heat flux
      slv.sclr_array("hflux") = 0.01 * 1. / p.hscale * exp(- k * p.dk / p.hscale);
      // environmental potential temperature profile
      slv.sclr_array("tht_e") = 300 * where(k * p.dk <= mixed_length, 1, 1 + (k * p.dk - mixed_length) * st);
      // tht absorber profile
      slv.sclr_array("tht_abs") = where(k * p.dk >= 1000, 1. / 1020 * (k * p.dk - 1000) / (1500-1000.0), 0);
      // velocity absorbers
      slv.vab_coefficient()(i_r, j_r, k_r) = slv.sclr_array("tht_abs")(i_r, j_r, k_r);
      slv.vab_relaxed_state(0) = 0;
      slv.vab_relaxed_state(1) = 0;
      slv.vab_relaxed_state(2) = 0;
    }
  }

  slv.advance(nt); 
};

int main()
{
  test("out_pbl");
}
