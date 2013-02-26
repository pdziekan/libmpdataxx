/** 
  * @file
  * @copyright University of Warsaw
  * @section LICENSE
  * GPLv3+ (see the COPYING file or http://www.gnu.org/licenses/)
  *
  * @brief minimum residual pressure solver 
  *   (for more detailed discussion consult Smolarkiewicz & Margolin 1994 
  *  Appl. Math and Comp. Sci. 
  *  Variational solver for elliptic problems in atmospheric flows)
  *
  * @section DERIVATION
  * 
  * equations are solved for pressure perturbation (in reference to inertial ambient state)
  * \f$ \Phi = \frac{p-p_e}{\bar{\rho}} \f$
  *
  * where: 
  *
  * \f$ p_e \f$ is pressure of the inertial ambient state (a state that satisfies the equations)
  *
  * \f$ \bar{\rho} \f$ is the reference state density of the fluid
  * 
  * from the continuity equation (applied after the first half-step of advection scheme)
  * 
  * \f$ -\frac{1}{\rho} \nabla \cdot (\rho (\hat{u} - \frac{\triangle t}{2} \nabla \Phi)) = 0 \f$
  *
  * where:
  *
  * \f$ \hat{u} \f$ is the velocity after the first half-step of advection (after applying all the known forcing terms)
  * 
  * multiplying by -1 is to get the diffusion equation
  * 
  * to derive the iterative solver we need to augment continuity equation with a pseudo-time (\f$ \tau \f$) dependence
  * 
  * \f$ -\frac{1}{\rho} \nabla \cdot (\rho (\hat{u} - \frac{\triangle t}{2} \nabla \Phi)) 
      = \frac{\partial \Phi}{\partial \tau} \f$
  *
  * discretizing in pseudo-time (with increment \f$ \beta \f$) leads to
  *
  * \f$ \Phi^{n+1} = \Phi^{n} + \beta (-\frac{1}{\rho} \nabla \cdot (\rho(\hat{u} - \nabla \Phi^{n})) ) \f$
  *
  * where term \f$ -\frac{1}{\rho} \nabla \cdot (\rho(\hat{u} - \nabla \Phi^{n})) \f$ can be denoted as \f$ r^{n} \f$
  * 
  * \f$ r^{n} \f$ is the residual error that needs to be minimized by the pseudo-time iterations within the pressure solver 
  *
  * \f$ \Phi^{n+1} = \Phi^{n} + \beta r^{n} \f$
  *
  * from the definition of \f$ r^{n} \f$ we can write that
  *
  * \f$ r^{n+1} = -\frac{1}{\rho} \nabla \cdot (\rho(\hat{u} - \nabla \Phi^{n+1})) \f$
  *
  * \f$ r^{n+1} = -\frac{1}{\rho} \nabla \cdot (\rho(\hat{u} - \nabla(\Phi^{n} + \beta r^{n}))) \f$
  *
  * \f$ r^{n+1} = r^{n} + \beta \Delta (r^{n}) \f$
  *
  * equations for \f$ \Phi^{n+1} \f$ and \f$ r^{n+1} \f$ form the recurrence formula used by the solver. 
  * To assure convergence pseudo-time increment for solver iterations has to be chosen. 
  * \f$ \beta = const = .25 \f$ assures convergence for every case (this is Richardson scheme).
  *
  * To assure the fastest possible convergence in a given case beta (pseudo-time increment) 
  * has to be chosen in a way that minimizes \f$ r^{n+1} \f$
  * 
  * \f$ <(r^{n+1})^2> \to min \;\;\;\;\;\; / \frac{\partial}{\partial \beta} \;\;\;\;\;\; 
  * \Rightarrow\;\;\;\;\;\; <2(r^{n+1}) \Delta(r^{n})> = 0 \f$
  *
  * where \f$ <r> \f$  denotes the norm of \f$ r \f$ (meaning the sum of \f$ r \f$ over all grid points)
  *
  * substituting \f$ r^{n+1} \f$ by it's recurrence formula and further rearranging leads to the formula for pseudo-time step
  *
  * \f$ \beta = - \frac{r^{n} \Delta r^{n}}{\Delta r^{n} \Delta r^{n}} \f$
  *
  *
  * pseudo-time iterations stop when residual error is smaller than a given value (for example .0001)
  */

#pragma once
#include "detail/solver_pressure_common.hpp"
#include "../formulae/nabla_formulae.hpp" //gradient, diveregnce

namespace advoocat
{
  namespace solvers
  {
    template <class inhomo_solver_t, int u, int w, int tht>
    class pressure_mr : public detail::pressure_solver_common<inhomo_solver_t, u, w, tht>
    {
      public:

      using parent_t = detail::pressure_solver_common<inhomo_solver_t, u, w, tht>;
      typedef typename parent_t::real_t real_t;

      //TODO probably don't need those
      typename parent_t::arr_t tmp_x, tmp_z;
      typename parent_t::arr_t tmp_e1, tmp_e2;

      private:

      void pressure_solver_update(real_t dt)
      {
	using namespace arakawa_c;
	using formulae::nabla_op::grad;
	using formulae::nabla_op::div;

	real_t rho = 1.;   //TODO    
        real_t beta = .25; //TODO tmp

	int halo = this->halo;
	rng_t &i = this->i;
	rng_t &j = this->j;

	this->tmp_u(i,j) = this->state(u)(i,j);
	this->tmp_w(i,j) = this->state(w)(i,j);

        this->xchng(this->Phi,   i^halo, j^halo);
        this->xchng(this->tmp_u, i^halo, j^halo);
        this->xchng(this->tmp_w, i^halo, j^halo);

        tmp_x(i, j) = rho * (this->tmp_u(i, j) - grad<0>(this->Phi, i, j, real_t(1)));
        tmp_z(i, j) = rho * (this->tmp_w(i, j) - grad<1>(this->Phi, j, i, real_t(1)));

        this->xchng(tmp_x, i^halo, j^halo);
        this->xchng(tmp_z, i^halo, j^halo);

        this->err(i, j) = - 1./ rho * div(tmp_x, tmp_z, i, j, real_t(1), real_t(1)); //error
        this->xchng(this->err, i^halo, j^halo);

std::cerr<<"--------------------------------------------------------------"<<std::endl;
	//pseudo-time loop
	real_t error = 1.;
	while (error > .00001) //TODO tmp TODO tmp !!!
	{
          this->xchng(this->err, i^halo, j^halo);

          tmp_e1(i, j) = grad<0>(this->err, i, j, real_t(1));
          tmp_e2(i, j) = grad<1>(this->err, j, i, real_t(1));
          this->xchng(tmp_e1, i^halo, j^halo);
          this->xchng(tmp_e2, i^halo, j^halo);
 
          this->lap_err(i,j) = div(tmp_e1, tmp_e2, i, j, real_t(1), real_t(1)); //laplasjan(error)

// if (!richardson) TODO - jako opcja (template?)
          this->mem->barrier();
          tmp_e1(i,j) = this->err(i,j) * this->lap_err(i,j);
          tmp_e2(i,j) = this->lap_err(i,j) * this->lap_err(i,j);
          beta = - this->mem->sum(tmp_e1,i,j) / this->mem->sum(tmp_e2,i,j);
// endif

          this->Phi(i, j) += beta * this->err(i, j);
          this->err(i, j) += beta * this->lap_err(i, j);

	  error = std::max(
	    std::abs(this->mem->max(this->err(i,j))),
	    std::abs(this->mem->min(this->err(i,j)))
	  );
          this->iters++;
	}
std::ostringstream s;
s << " error " << error << std::endl;
std::cerr << s.str();
	//end of pseudo_time loop

	this->xchng(this->Phi, i^halo, j^halo);
	this->tmp_u(i, j) = - grad<0>(this->Phi, i, j, real_t(1));
	this->tmp_w(i, j) = - grad<1>(this->Phi, j, i, real_t(1));
      }

      public:

      struct params_t : parent_t::params_t { };

      // ctor
      pressure_mr(
	typename parent_t::mem_t *mem,
        typename parent_t::bc_p &bcxl,
        typename parent_t::bc_p &bcxr,
        typename parent_t::bc_p &bcyl,
        typename parent_t::bc_p &bcyr,
	const rng_t &i,
	const rng_t &j,
	const params_t &p
      ) :
	parent_t(mem, bcxl, bcxr, bcyl, bcyr, i, j, p),
        // (i^hlo, j^hlo)
	tmp_x(mem->tmp[std::string(__FILE__)][0][0]),
	tmp_z(mem->tmp[std::string(__FILE__)][0][1]),
	tmp_e1(mem->tmp[std::string(__FILE__)][0][2]),
	tmp_e2(mem->tmp[std::string(__FILE__)][0][3])
      {}

      static void alloc(typename parent_t::mem_t *mem, const int nx, const int ny)
      {
        parent_t::alloc(mem, nx, ny);

        const std::string file(__FILE__);
        const rng_t i(0, nx-1), j(0, ny-1);
        const int halo = parent_t::halo;

        // temporary fields
        mem->tmp[file].push_back(new arrvec_t<typename parent_t::arr_t>());
        {
          for (int n=0; n < 4; ++n) 
            mem->tmp[file].back().push_back(new typename parent_t::arr_t( i^halo, j^halo )); 
        }
      }
    }; 
  }; // namespace solvers
}; // namespace advoocat
