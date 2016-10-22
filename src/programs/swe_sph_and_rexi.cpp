/*
 * AppTestSWE.hpp
 *
 *  Created on: 15 Aug 2016
 *      Author: Martin Schreiber <M.Schreiber@exeter.ac.uk>
 */

#ifndef SRC_TESTSWE_HPP_
#define SRC_TESTSWE_HPP_

#include <benchmarks_sphere/BenchmarkGalewsky.hpp>
#include <sweet/sphere/SphereDataComplex.hpp>
#include <sweet/sphere/SphereDataTimesteppingRK.hpp>
#include <sweet/sphere/SphBandedMatrix.hpp>
#include <sweet/sphere/SphereOperators.hpp>
#include <sweet/sphere/SphereOperatorsComplex.hpp>

#include <rexi/REXI.hpp>
#include <rexi/SWERexi_SPH.hpp>
#include <rexi/SWERexi_SPHRobert.hpp>

SimulationVariables simVars;

//Diagnostic measures at initial stage
double diagnostics_energy_start, diagnostics_mass_start, diagnostics_potential_entrophy_start;

// Plane data config
SphereDataConfig sphereDataConfigInstance;
SphereDataConfig *sphereDataConfig = &sphereDataConfigInstance;



class SWE_and_REXI
{
public:
	SphereOperators op;
	SphereOperatorsComplex opComplex;

	SphereDataConfig *sphereDataConfig;

	// Runge-Kutta stuff
	SphereDataTimesteppingRK timestepping;


	SphereData prog_h;
	SphereData prog_u;
	SphereData prog_v;
	SphereData fdata;


	BenchmarkGalewsky benchmarkGalewsky;

	REXI rexi;



public:
	SWE_and_REXI(
			SphereDataConfig *i_sphereConfig
	)	:
		sphereDataConfig(i_sphereConfig),
		prog_h(i_sphereConfig),
		prog_u(i_sphereConfig),
		prog_v(i_sphereConfig),
		fdata(i_sphereConfig),
		benchmarkGalewsky(::simVars)
	{
	}



	void write_output()
	{
		char buffer[1024];

		sprintf(buffer, "prog_h_t%020.8f.csv", simVars.timecontrol.current_simulation_time/(60*60));
		if (simVars.setup.benchmark_scenario_id == 0)
			prog_h.file_physical_writeFile_lon_pi_shifted(buffer);
		else
			prog_h.file_physical_writeFile(buffer);
		std::cout << buffer << " (min: " << prog_h.reduce_min() << ", max: " << prog_h.reduce_max() << ")" << std::endl;

		sprintf(buffer, "prog_u_t%020.8f.csv", simVars.timecontrol.current_simulation_time/(60*60));
		if (simVars.setup.benchmark_scenario_id == 0)
			prog_u.file_physical_writeFile_lon_pi_shifted(buffer);
		else
			prog_u.file_physical_writeFile(buffer);
		std::cout << buffer << std::endl;

		sprintf(buffer, "prog_v_t%020.8f.csv", simVars.timecontrol.current_simulation_time/(60*60));
		if (simVars.setup.benchmark_scenario_id == 0)
			prog_v.file_physical_writeFile_lon_pi_shifted(buffer);
		else
			prog_v.file_physical_writeFile(buffer);
		std::cout << buffer << std::endl;

		sprintf(buffer, "prog_eta_t%020.8f.csv", simVars.timecontrol.current_simulation_time/(60*60));
		SphereData vort = op.vort(prog_u, prog_v)/simVars.sim.earth_radius;
		if (simVars.setup.benchmark_scenario_id == 0)
			vort.file_physical_writeFile_lon_pi_shifted(buffer, "vorticity, lon pi shifted");
		else
			vort.file_physical_writeFile(buffer);
		std::cout << buffer << std::endl;
	}



	void setup_initial_conditions_gaussian(
			double i_center_lat = M_PI/3,
			double i_center_lon = M_PI/3
	)
	{
		double exp_fac = 10.0;

		double center_lat = i_center_lat;
		double center_lon = i_center_lon;

		auto initial_condition_h = [&](double lon, double mu, double &o_data)
		{
			// https://en.wikipedia.org/wiki/Great-circle_distance
			// d = acos(sin(phi1)*sin(phi2) + cos(phi1)*cos(phi2)*cos(lon1-lon2))
			// exp(-pow(acos(sin(phi1)*sin(phi2) + cos(phi1)*cos(phi2)*cos(lon1-lon2)), 2)*A)

			double phi1 = asin(mu);
			double phi2 = center_lat;
			double lambda1 = lon;
			double lambda2 = center_lon;

			double d = acos(sin(phi1)*sin(phi2) + cos(phi1)*cos(phi2)*cos(lambda1-lambda2));

			o_data = exp(-d*d*exp_fac)*0.1*simVars.sim.h0 + simVars.sim.h0;
		};

		prog_h.physical_update_lambda_gaussian_grid(initial_condition_h);
		prog_u.physical_set_zero();
		prog_v.physical_set_zero();
	}



	SphereData f(SphereData i_sphData)
	{
//		return op.coriolis(i_sphData, simVars.sim.coriolis_omega);
		return fdata*i_sphData*2.0*simVars.sim.coriolis_omega;
	}



	void run()
	{
		// one month runtime
		if (simVars.timecontrol.max_simulation_time == -1)
		{
			simVars.timecontrol.max_simulation_time = 31*60*60*24;

			// 144 h
			//simVars.timecontrol.max_simulation_time = 144*60*60;

			// 200 h
			simVars.timecontrol.max_simulation_time = 200*60*60;
	//		simVars.timecontrol.max_simulation_time = 1;
		}

		simVars.misc.output_next_sim_seconds = 0;


		if (simVars.sim.CFL < 0)
			simVars.timecontrol.current_timestep_size = -simVars.sim.CFL;


		if (simVars.timecontrol.current_timestep_size <= 0)
		{
			// TRY to guess optimal time step size

			// time step size
			if (sphereDataConfig->spat_num_lat < 256)
			{
	//			simVars.sim.viscosity2 = 1e5;
				simVars.timecontrol.current_timestep_size = 0.002*simVars.sim.earth_radius/(double)sphereDataConfig->spat_num_lat;
			}
			else
			{
	//			simVars.sim.viscosity2 = 1e5;
				simVars.timecontrol.current_timestep_size = 0.001*simVars.sim.earth_radius/(double)sphereDataConfig->spat_num_lat;
			}
			//simVars.timecontrol.current_timestep_size = 30;
		}

		if (simVars.misc.output_each_sim_seconds <= 0)
		{
			simVars.misc.output_each_sim_seconds = 60*30;	// output every 1/2 hour
		}


		if (simVars.rexi.use_rexi == 1)
		{
			// Override for REXI
			if (simVars.timecontrol.current_timestep_size <= 0)
			{
				std::cout << "Timestep size not positive" << std::endl;
				assert(false);
				exit(1);
			}

			simVars.misc.use_nonlinear_equations = 0;
		}

		if (simVars.setup.benchmark_scenario_id == 0)
		{
			setup_initial_conditions_gaussian(M_PI/3.0, M_PI/3.0);
		}
		else if (simVars.setup.benchmark_scenario_id == 1)
		{
			benchmarkGalewsky.setup_initial_h(prog_h);
//			prog_h.spat_set_zero();
			benchmarkGalewsky.setup_initial_h_add_bump(prog_h);

			benchmarkGalewsky.setup_initial_u(prog_u);
			benchmarkGalewsky.setup_initial_v(prog_v);
		}
		else if (simVars.setup.benchmark_scenario_id == 2 || simVars.setup.benchmark_scenario_id == 3)
		{
			// Non-dimensional stuff
#if 0
			if (simVars.timecontrol.current_timestep_size <= 0)
			{
				std::cout << "Timestep size not positive" << std::endl;
				assert(false);
				exit(1);
			}
#endif

			simVars.sim.h0 = 1;
			simVars.sim.gravitation = 1;
			simVars.sim.earth_radius = 1;

			simVars.misc.use_nonlinear_equations = 0;

			if (simVars.setup.benchmark_scenario_id == 2)
			{
				setup_initial_conditions_gaussian(0);
#if 0
	//			prog_h.spat_set_value(simVars.sim.h0);
				prog_v.physical_set_zero();
				prog_u.spat_update_lambda_cogaussian_grid(
						[](double lon, double comu, double &io_data)
						{
							io_data = 1*comu;
						}
					);
				//prog_u.spat_set_zero();
#endif
			}
			else if (simVars.setup.benchmark_scenario_id == 3)
			{
				setup_initial_conditions_gaussian(M_PI/3.0);
				//setup_initial_conditions_gaussian(M_PI, M_PI);
//				setup_initial_conditions_gaussian(M_PI*0.5, M_PI*0.5);
//				setup_initial_conditions_gaussian(-M_PI/3.0);
			}
		}


		bool with_coriolis = false;

		if (simVars.sim.coriolis_omega != 0)
			with_coriolis = true;

		std::cout << "Using time step size dt = " << simVars.timecontrol.current_timestep_size << std::endl;
		std::cout << "Running simulation until t_end = " << simVars.timecontrol.max_simulation_time << std::endl;
		std::cout << "Parameters:" << std::endl;
		std::cout << " + gravity: " << simVars.sim.gravitation << std::endl;
		std::cout << " + earth_radius: " << simVars.sim.earth_radius << std::endl;
		std::cout << " + average height: " << simVars.sim.h0 << std::endl;
		std::cout << " + coriolis_omega: " << simVars.sim.coriolis_omega << std::endl;
		std::cout << " + viscosity D: " << simVars.sim.viscosity << std::endl;
		std::cout << " + use_nonlinear: " << simVars.misc.use_nonlinear_equations << std::endl;
		std::cout << " + Coriolis: " << with_coriolis << std::endl;
		std::cout << std::endl;
		std::cout << " + Benchmark scenario id: " << simVars.setup.benchmark_scenario_id << std::endl;
		std::cout << " + Use robert functions: " << simVars.misc.sphere_use_robert_functions << std::endl;
		std::cout << " + Use REXI: " << simVars.rexi.use_rexi << std::endl;
		std::cout << " + REXI h: " << simVars.rexi.rexi_h << std::endl;
		std::cout << " + REXI M: " << simVars.rexi.rexi_M << std::endl;
		std::cout << " + REXI use half poles: " << simVars.rexi.rexi_use_half_poles << std::endl;
		std::cout << " + REXI additional modes: " << simVars.rexi.rexi_use_extended_modes << std::endl;
		std::cout << std::endl;
		std::cout << " + timestep size: " << simVars.timecontrol.current_timestep_size << std::endl;
		std::cout << " + output timestep size: " << simVars.misc.output_each_sim_seconds << std::endl;

		std::cout << std::endl;

		fdata.physical_update_lambda_gaussian_grid(
				[&](double lon, double mu, double &o_data)
				{
					o_data = mu;
				}
			);


		// This class is only used in case of added modes
		SphereDataConfig sphereConfigRexiAddedModes;

		// Pointer to SPH configuration for REXI computations
		SphereDataConfig *sphereConfigRexi = nullptr;

		if (simVars.rexi.use_rexi == false)
		{
			simVars.timecontrol.current_simulation_time = 0;
			while (simVars.timecontrol.current_simulation_time < simVars.timecontrol.max_simulation_time)
			{
				if (simVars.timecontrol.current_simulation_time >= simVars.misc.output_next_sim_seconds)
				{
					std::cout << std::endl;
					write_output();

					simVars.misc.output_next_sim_seconds += simVars.misc.output_each_sim_seconds;
					if (simVars.misc.output_next_sim_seconds < simVars.timecontrol.current_simulation_time)
						simVars.misc.output_next_sim_seconds = simVars.timecontrol.current_simulation_time;
				}

				double o_dt;
				timestepping.run_rk_timestep(
						this,
						&SWE_and_REXI::p_run_euler_timestep_update,	///< pointer to function to compute euler time step updates
						prog_h, prog_u, prog_v,
						o_dt,
						simVars.timecontrol.current_timestep_size,
						simVars.disc.timestepping_runge_kutta_order,
						simVars.timecontrol.current_simulation_time,
						simVars.timecontrol.max_simulation_time
					);

				std::cout << "." << std::flush;

				if (prog_h.physical_isAnyNaNorInf())
				{
					std::cerr << "Instability detected (NaN value in H)" << std::endl;
					assert(false);
					exit(1);
				}

				simVars.timecontrol.current_simulation_time += simVars.timecontrol.current_timestep_size;
			}

			std::cout << std::endl;
			write_output();
			std::cout << std::endl;
		}
		else
		{
			if (simVars.rexi.rexi_use_extended_modes == 0)
			{
				sphereConfigRexi = sphereDataConfig;
			}
			else
			{
				// Add modes only along latitude since these are the "problematic" modes
				sphereConfigRexiAddedModes.setupAdditionalModes(
						sphereDataConfig,
						simVars.rexi.rexi_use_extended_modes,	// TODO: Extend SPH wrapper to also support m != n to set this guy to 0
						simVars.rexi.rexi_use_extended_modes
				);
				sphereConfigRexi = &sphereConfigRexiAddedModes;
			}

			SphBandedMatrix<double> sphSolver;
			sphSolver.setup(sphereDataConfig, 4);

			rexi.setup(simVars.rexi.rexi_h, simVars.rexi.rexi_M, 0, simVars.rexi.rexi_use_half_poles);

			std::cout << "REXI poles: " << rexi.alpha.size() << std::endl;

			SphereData tmp_prog_phi(sphereConfigRexi);
			SphereData tmp_prog_u(sphereConfigRexi);
			SphereData tmp_prog_v(sphereConfigRexi);

			SphereData accum_prog_phi(sphereConfigRexi);
			SphereData accum_prog_u(sphereConfigRexi);
			SphereData accum_prog_v(sphereConfigRexi);

			std::vector<SWERexi_SPHRobert> rexiSPHRobert_vector;
			std::vector<SWERexi_SPH> rexiSPH_vector;

			bool use_rexi_preallocaation = true;

			if (use_rexi_preallocaation)
			{
				if (simVars.misc.sphere_use_robert_functions)
					rexiSPHRobert_vector.resize(rexi.alpha.size());
				else
					rexiSPH_vector.resize(rexi.alpha.size());


				for (std::size_t i = 0; i < rexi.alpha.size(); i++)
				{
					std::complex<double> &alpha = rexi.alpha[i];
					std::complex<double> &beta = rexi.beta_re[i];

					if (simVars.misc.sphere_use_robert_functions)
					{
						rexiSPHRobert_vector[i].setup(
								sphereConfigRexi,
								alpha,
								beta,
								simVars.sim.earth_radius,
								simVars.sim.coriolis_omega, //*inv_sqrt_avg_geopo,
								simVars.sim.h0*simVars.sim.gravitation,
								simVars.timecontrol.current_timestep_size, //*sqrt_avg_geopo
								with_coriolis
						);
					}
					else
					{
						rexiSPH_vector[i].setup(
								sphereConfigRexi,
								alpha,
								beta,
								simVars.sim.earth_radius,
								simVars.sim.coriolis_omega, //*inv_sqrt_avg_geopo,
								simVars.sim.h0*simVars.sim.gravitation,
								simVars.timecontrol.current_timestep_size, //*sqrt_avg_geopo
								with_coriolis
						);
					}
				}
			}

			simVars.timecontrol.current_simulation_time = 0;
			while (simVars.timecontrol.current_simulation_time < simVars.timecontrol.max_simulation_time)
			{
				if (simVars.timecontrol.current_simulation_time >= simVars.misc.output_next_sim_seconds)
				{
					write_output();

					simVars.misc.output_next_sim_seconds += simVars.misc.output_each_sim_seconds;
					if (simVars.misc.output_next_sim_seconds < simVars.timecontrol.current_simulation_time)
						simVars.misc.output_next_sim_seconds = simVars.timecontrol.current_simulation_time;
				}


				{
					// convert to geopotential
					SphereData prog_phi_rexi(sphereConfigRexi);
					SphereData prog_u_rexi(sphereConfigRexi);
					SphereData prog_v_rexi(sphereConfigRexi);

					if (simVars.rexi.rexi_use_extended_modes == 0)
					{
						prog_phi_rexi = prog_h*simVars.sim.gravitation;
						prog_u_rexi = prog_u;
						prog_v_rexi = prog_v;
					}
					else
					{
						(prog_h*simVars.sim.gravitation).spectral_copyToDifferentModes(prog_phi_rexi);
						prog_u.spectral_copyToDifferentModes(prog_u_rexi);
						prog_v.spectral_copyToDifferentModes(prog_v_rexi);
					}

					accum_prog_phi.physical_set_zero();
					accum_prog_u.physical_set_zero();
					accum_prog_v.physical_set_zero();

					for (std::size_t i = 0; i < rexi.alpha.size(); i++)
					{
						std::complex<double> &alpha = rexi.alpha[i];
						std::complex<double> &beta = rexi.beta_re[i];

						if (simVars.misc.sphere_use_robert_functions)
						{
							if (use_rexi_preallocaation)
							{
								rexiSPHRobert_vector[i].solve(
										prog_phi_rexi, prog_u_rexi, prog_v_rexi,
										tmp_prog_phi, tmp_prog_u, tmp_prog_v,
										opComplex
									);
							}
							else
							{
								SWERexi_SPHRobert rexiSPHRobert;
								rexiSPHRobert.setup(
										sphereDataConfig,
										alpha,
										beta,
										simVars.sim.earth_radius,
										simVars.sim.coriolis_omega, //*inv_sqrt_avg_geopo,
										simVars.sim.h0*simVars.sim.gravitation,
										simVars.timecontrol.current_timestep_size, //*sqrt_avg_geopo
										with_coriolis
								);

								rexiSPHRobert.solve(
										prog_phi_rexi, prog_u_rexi, prog_v_rexi,
										tmp_prog_phi, tmp_prog_u, tmp_prog_v,
										opComplex
									);
							}
						}
						else
						{
							if (use_rexi_preallocaation)
							{
								rexiSPH_vector[i].solve(
										prog_phi_rexi, prog_u_rexi, prog_v_rexi,
										tmp_prog_phi, tmp_prog_u, tmp_prog_v,
										opComplex
									);
							}
							else
							{
								SWERexi_SPH rexiSPH;
								rexiSPH.setup(
										sphereDataConfig,
										alpha,
										beta,
										simVars.sim.earth_radius,
										simVars.sim.coriolis_omega,
										simVars.sim.h0*simVars.sim.gravitation,
										simVars.timecontrol.current_timestep_size,
										with_coriolis
								);

								rexiSPH.solve(
										prog_phi_rexi, prog_u_rexi, prog_v_rexi,
										tmp_prog_phi, tmp_prog_u, tmp_prog_v,
										opComplex
									);
							}
						}

						accum_prog_phi += tmp_prog_phi;
						accum_prog_u += tmp_prog_u;
						accum_prog_v += tmp_prog_v;
					}

					if (simVars.rexi.rexi_use_extended_modes == 0)
					{
						prog_h = accum_prog_phi*(1.0/simVars.sim.gravitation);
						prog_u = accum_prog_u;
						prog_v = accum_prog_v;
					}
					else
					{
						(accum_prog_phi*(1.0/simVars.sim.gravitation)).spectral_copyToDifferentModes(prog_h);
						accum_prog_u.spectral_copyToDifferentModes(prog_u);
						accum_prog_v.spectral_copyToDifferentModes(prog_v);
					}
				}


				std::cout << "." << std::flush;

				/*
				 * Add implicit viscosity
				 */
				if (simVars.sim.viscosity != 0)
				{
					double scalar = simVars.sim.viscosity*simVars.timecontrol.current_timestep_size;
					double r = simVars.sim.earth_radius;

					/*
					 * (1-dt*visc*D2)p(t+dt) = p(t)
					 */
					prog_h = prog_h.spectral_solve_helmholtz(1.0, -scalar, r);
					prog_u = prog_u.spectral_solve_helmholtz(1.0, -scalar, r);
					prog_v = prog_v.spectral_solve_helmholtz(1.0, -scalar, r);
				}

				if (prog_h.physical_isAnyNaNorInf())
				{
					std::cerr << "Instability detected (NaN value in H)" << std::endl;
					assert(false);
					exit(1);
				}

				simVars.timecontrol.current_simulation_time += simVars.timecontrol.current_timestep_size;
			}
			write_output();
			std::cout << std::endl;
		}
	}



	// Main routine for method to be used in case of finite differences
	void p_run_euler_timestep_update(
			const SphereData &i_h,	///< prognostic variables
			const SphereData &i_u,	///< prognostic variables
			const SphereData &i_v,	///< prognostic variables

			SphereData &o_h_t,	///< time updates
			SphereData &o_u_t,	///< time updates
			SphereData &o_v_t,	///< time updates

			double &o_dt,				///< time step restriction
			double i_fixed_dt = 0,		///< if this value is not equal to 0, use this time step size instead of computing one
			double i_simulation_timestamp = -1
	)
	{
		o_dt = simVars.timecontrol.current_timestep_size;

		if (!simVars.misc.use_nonlinear_equations)
		{
			if (!simVars.misc.sphere_use_robert_functions)
			{
				// linear equations
				o_h_t = -(op.div_lon(i_u)+op.div_lat(i_v))*(simVars.sim.h0/simVars.sim.earth_radius);

				o_u_t = -op.grad_lon(i_h)*(simVars.sim.gravitation/simVars.sim.earth_radius);
				o_v_t = -op.grad_lat(i_h)*(simVars.sim.gravitation/simVars.sim.earth_radius);

				if (simVars.sim.coriolis_omega != 0)
				{
					o_u_t += f(i_v);
					o_v_t -= f(i_u);
				}
			}
			else
			{
				// use Robert functions for velocity
				// linear equations
				o_h_t = -(op.robert_div_lon(i_u)+op.robert_div_lat(i_v))*(simVars.sim.h0/simVars.sim.earth_radius);

				o_u_t = -op.robert_grad_lon(i_h)*(simVars.sim.gravitation/simVars.sim.earth_radius);
				o_v_t = -op.robert_grad_lat(i_h)*(simVars.sim.gravitation/simVars.sim.earth_radius);

				if (simVars.sim.coriolis_omega != 0)
				{
					o_u_t += f(i_v);
					o_v_t -= f(i_u);
				}
			}
		}
		else
		{
			assert(simVars.sim.earth_radius > 0);
			assert(simVars.sim.gravitation);

			/*
			 * Height
			 */
			// non-linear equations
			o_h_t = -(op.div_lon(i_h*i_u)+op.div_lat(i_h*i_v))*(1.0/simVars.sim.earth_radius);

			/*
			 * Velocity
			 */
			// linear terms
			o_u_t = -op.grad_lon(i_h)*(simVars.sim.gravitation/simVars.sim.earth_radius);
			o_v_t = -op.grad_lat(i_h)*(simVars.sim.gravitation/simVars.sim.earth_radius);

			if (simVars.sim.coriolis_omega != 0)
			{
				o_u_t += f(i_v);
				o_v_t -= f(i_u);
			}

			// non-linear terms
			o_u_t -= (i_u*op.grad_lon(i_u) + i_v*op.grad_lat(i_u))*(1.0/simVars.sim.earth_radius);
			o_v_t -= (i_u*op.grad_lon(i_v) + i_v*op.grad_lat(i_v))*(1.0/simVars.sim.earth_radius);
		}

		assert(simVars.sim.viscosity_order == 2);
		if (simVars.sim.viscosity != 0)
		{
			double scalar = simVars.sim.viscosity/(simVars.sim.earth_radius*simVars.sim.earth_radius);

			o_h_t += op.laplace(i_h)*scalar;
			o_u_t += op.laplace(i_u)*scalar;
			o_v_t += op.laplace(i_v)*scalar;
		}
	}
};


int main2(int i_argc, char *i_argv[])
{
	MemBlockAlloc::setup();

	//input parameter names (specific ones for this program)
	const char *bogus_var_names[] = {
			nullptr
	};

	// default values for specific input (for general input see SimulationVariables.hpp)
	//simVars.bogus.var[0] = 0.2;

	// Help menu
	if (!simVars.setupFromMainParameters(i_argc, i_argv, bogus_var_names))
	{
#if SWEET_PARAREAL
		simVars.parareal.setup_printOptions();
#endif
		return -1;
	}


	sphereDataConfigInstance.setupAutoPhysicalSpace(
					simVars.disc.res_spectral[0],
					simVars.disc.res_spectral[1],
					&simVars.disc.res_physical[0],
					&simVars.disc.res_physical[1]
			);


	SWE_and_REXI test_swe(sphereDataConfig);
	test_swe.run();

	return 0;
}

int main(int i_argc, char *i_argv[])
{
	int retval = main2(i_argc, i_argv);

	return retval;
}


#endif /* SRC_TESTSWE_HPP_ */