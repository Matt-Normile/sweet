/*
 * SPHOperators.hpp
 *
 *  Created on: 12 Aug 2016
 *      Author: Martin Schreiber <schreiberx@gmail.com>
 */

#ifndef SPHOPERATORS_HPP_
#define SPHOPERATORS_HPP_

#include <sweet/MemBlockAlloc.hpp>
#include "../sphere/SphereData.hpp"
#include "../sphere/SphereSPHIdentities.hpp"
#include <sweet/sphere/app_swe/SWESphBandedMatrixPhysicalReal.hpp>


class SphereOperators	:
	public SphereSPHIdentities
{
	friend SphereDataConfig;

public:
	const SphereDataConfig *sphereDataConfig;

private:

	double r;
	double ir;


public:
	SphereOperators(
		SphereDataConfig *i_sphereDataConfig,
		double i_earth_radius
	)
	{
		setup(i_sphereDataConfig, i_earth_radius);
	}


public:
	SphereOperators()
	{
	}



public:
	void setup(
		const SphereDataConfig *i_sphereDataConfig,
		double i_earth_radius
	)
	{
		sphereDataConfig = i_sphereDataConfig;

		r = i_earth_radius;
		ir = 1.0/r;

#if 1
		double *mx = new double[2*sphereDataConfig->shtns->nlm];
		st_dt_matrix(sphereDataConfig->shtns, mx);

		for (int m = 0; m <= sphereDataConfig->spectral_modes_m_max; m++)
		{
			int idx = sphereDataConfig->getArrayIndexByModes(m, m);

			for (int n = m; n <= sphereDataConfig->spectral_modes_n_max; n++)
			{
				double a = (-n+1.0)*R(n-1,m);
				double b = (n+2.0)*S(n+1,m);

				if (n+1 > sphereDataConfig->spectral_modes_n_max)
					b = 0;

				//std::cout << idx << ": " << a << "\t" << mx[idx*2+0] << std::endl;
				//std::cout << idx << ": " << b << "\t" << mx[idx*2+1] << std::endl;

				double errora = std::abs(a+mx[idx*2+0]);
				double errorb = std::abs(b+mx[idx*2+1]);

				if (errora > 1e-12 || errorb > 1e-12)
				{
					std::cout << idx << ": n=" << n << ", m=" << m << " | "<< errora << "\t" << errorb << std::endl;
					FatalError("SAFETY CHECK NOT SUCCESSFUL");
				}

				idx++;
			}
		}
		delete [] mx;
#endif

	}



#if 0

public:
	/**
	 * Compute differential along longitude
	 *
	 * d/d lambda f(lambda,mu)
	 */
	SphereData diff_lon(
			const SphereData &i_sph_data
	)	const
	{
		i_sph_data.request_data_spectral();

		SphereData out_sph_data(i_sph_data.sphereDataConfig);

		// compute d/dlambda in spectral space
		SWEET_THREADING_SPACE_PARALLEL_FOR
		for (int m = i_sph_data.sphereDataConfig->spectral_modes_m_max; m >= 0; m--)
		{
			int idx = i_sph_data.sphereDataConfig->getArrayIndexByModes(m, m);

			for (int n = m; n <= i_sph_data.sphereDataConfig->spectral_modes_n_max; n++)
			{
				out_sph_data.spectral_space_data[idx] = i_sph_data.spectral_space_data[idx]*std::complex<double>(0, m);
				idx++;
			}
		}

		out_sph_data.spectral_space_data_valid = true;
		out_sph_data.physical_space_data_valid = false;

		return out_sph_data;
	}
#endif



	/**
	 * Convert vorticity/divergence field to u,v velocity field
	 */
	void robert_vortdiv_to_uv(
			const SphereData &i_vrt,
			const SphereData &i_div,
			SphereDataPhysical &o_u,
			SphereDataPhysical &o_v

	)	const
	{
		i_vrt.request_data_spectral();
		i_div.request_data_spectral();

		SphereData psi = inv_laplace(i_vrt)*ir;
		SphereData chi = inv_laplace(i_div)*ir;

		SHsphtor_to_spat(
				sphereDataConfig->shtns,
				psi.spectral_space_data,
				chi.spectral_space_data,
				o_u.physical_space_data,
				o_v.physical_space_data
		);
	}


	/**
	 * Convert vorticity/divergence field to u,v velocity field
	 */
	void robert_grad_to_vec(
			const SphereData &i_phi,
			SphereDataPhysical &o_u,
			SphereDataPhysical &o_v,
			double i_radius

	)	const
	{
		double ir = 1.0/i_radius;

		i_phi.request_data_spectral();

		SphereData psi(sphereDataConfig);
		psi.spectral_set_zero();

		SphereDataPhysical u(sphereDataConfig);
		SphereDataPhysical v(sphereDataConfig);

		SHsphtor_to_spat(
						sphereDataConfig->shtns,
						psi.spectral_space_data,
						i_phi.spectral_space_data,
						o_u.physical_space_data,
						o_v.physical_space_data
				);

		o_u *= ir;
		o_v *= ir;
	}



	/**
	 * Convert vorticity/divergence field to u,v velocity field
	 */
	void vortdiv_to_uv(
			const SphereData &i_vrt,
			const SphereData &i_div,
			SphereDataPhysical &o_u,
			SphereDataPhysical &o_v

	)	const
	{

		i_vrt.request_data_spectral();
		i_div.request_data_spectral();

		SphereData psi = inv_laplace(i_vrt)*ir;
		SphereData chi = inv_laplace(i_div)*ir;

		#if SWEET_DEBUG
			#if SWEET_THREADING_SPACE || SWEET_THREADING_TIME_REXI
				if (omp_in_parallel())
					FatalError("IN PARALLEL REGION!!!");
			#endif
		#endif

		shtns_robert_form(sphereDataConfig->shtns, 0);
		SHsphtor_to_spat(
				sphereDataConfig->shtns,
				psi.spectral_space_data,
				chi.spectral_space_data,
				o_u.physical_space_data,
				o_v.physical_space_data
		);
		shtns_robert_form(sphereDataConfig->shtns, 1);
	}



	SphereData robert_uv_to_vort(
			const SphereDataPhysical &i_u,
			const SphereDataPhysical &i_v

	)	const
	{
		SphereData tmp(sphereDataConfig);
		SphereData vort(sphereDataConfig);

		SphereDataPhysical ug = i_u;
		SphereDataPhysical vg = i_v;

		shtns_robert_form(sphereDataConfig->shtns, 1);
		spat_to_SHsphtor(
				sphereDataConfig->shtns,
				ug.physical_space_data,
				vg.physical_space_data,
				vort.spectral_space_data,
				tmp.spectral_space_data
		);

		vort.physical_space_data_valid = false;
		vort.spectral_space_data_valid = true;

		return laplace(vort)*r;
	}



	SphereData uv_to_vort(
			const SphereDataPhysical &i_u,
			const SphereDataPhysical &i_v

	)	const
	{
		SphereData tmp(sphereDataConfig);
		SphereData vort(sphereDataConfig);

		#if SWEET_DEBUG
			#if SWEET_THREADING_SPACE || SWEET_THREADING_TIME_REXI
				if (omp_in_parallel())
					FatalError("IN PARALLEL REGION!!!");
			#endif
		#endif

		shtns_robert_form(sphereDataConfig->shtns, 0);
		spat_to_SHsphtor(
				sphereDataConfig->shtns,
				i_u.physical_space_data,
				i_v.physical_space_data,
				vort.spectral_space_data,
				tmp.spectral_space_data
		);
		shtns_robert_form(sphereDataConfig->shtns, 1);

		vort.physical_space_data_valid = false;
		vort.spectral_space_data_valid = true;

		return laplace(vort)*r;
	}



	void robert_uv_to_vortdiv(
			const SphereDataPhysical &i_u,
			const SphereDataPhysical &i_v,
			SphereData &o_vort,
			SphereData &o_div

	)	const
	{
		SphereDataPhysical ug = i_u;
		SphereDataPhysical vg = i_v;

		spat_to_SHsphtor(
				sphereDataConfig->shtns,
				ug.physical_space_data,
				vg.physical_space_data,
				o_vort.spectral_space_data,
				o_div.spectral_space_data
		);

		o_vort.physical_space_data_valid = false;
		o_vort.spectral_space_data_valid = true;

		o_div.physical_space_data_valid = false;
		o_div.spectral_space_data_valid = true;

		o_vort = laplace(o_vort)*r;
		o_div = laplace(o_div)*r;
	}




	void uv_to_vortdiv(
			const SphereDataPhysical &i_u,
			const SphereDataPhysical &i_v,
			SphereData &o_stream,
			SphereData &o_potential

	)	const
	{

		#if SWEET_DEBUG
			#if SWEET_THREADING_SPACE || SWEET_THREADING_TIME_REXI
				if (omp_in_parallel())
					FatalError("IN PARALLEL REGION!!!");
			#endif
		#endif

		shtns_robert_form(sphereDataConfig->shtns, 0);
		spat_to_SHsphtor(
				sphereDataConfig->shtns,
				i_u.physical_space_data,
				i_v.physical_space_data,
				o_stream.spectral_space_data,
				o_potential.spectral_space_data
		);
		shtns_robert_form(sphereDataConfig->shtns, 1);

		o_stream.physical_space_data_valid = false;
		o_stream.spectral_space_data_valid = true;

		o_potential.physical_space_data_valid = false;
		o_potential.spectral_space_data_valid = true;

		o_stream = laplace(o_stream)*r;
		o_potential = laplace(o_potential)*r;
	}




	SphereData spectral_one_minus_sinphi_squared_diff_lat_mu(
			const SphereData &i_sph_data
	)	const
	{
		return spectral_one_minus_mu_squared_diff_lat_mu(i_sph_data);
	}



	SphereData spectral_cosphi2_diff_lat_mu(
			const SphereData &i_sph_data
	)	const
	{
		return spectral_one_minus_mu_squared_diff_lat_mu(i_sph_data);
	}



	/**
	 * (1-mu^2) d/dmu ()
	 */
	SphereData spectral_one_minus_mu_squared_diff_lat_mu(
			const SphereData &i_sph_data
	)	const
	{
		i_sph_data.request_data_spectral();
		const SphereDataConfig *sphereDataConfig = i_sph_data.sphereDataConfig;

		SphereData out_sph_data(sphereDataConfig);

		SWEET_THREADING_SPACE_PARALLEL_FOR
		for (int m = 0; m <= i_sph_data.sphereDataConfig->spectral_modes_m_max; m++)
		{
			int idx = i_sph_data.sphereDataConfig->getArrayIndexByModes(m, m);

			for (int n = m; n <= i_sph_data.sphereDataConfig->spectral_modes_n_max; n++)
			{
				out_sph_data.spectral_space_data[idx] =
						((double)(-n+1.0)*R(n-1,m))*i_sph_data.spectral_get(n-1, m) +
						((double)(n+2.0)*S(n+1,m))*i_sph_data.spectral_get(n+1, m);

				idx++;
			}
		}

		out_sph_data.physical_space_data_valid = false;
		out_sph_data.spectral_space_data_valid = true;

		return out_sph_data;
	}



	/**
	 * Compute
	 * mu*F(\lambda,\mu)
	 */
	SphereData mu(
			const SphereData &i_sphere_data
	)	const
	{
		const SphereDataConfig *sphereDataConfig = i_sphere_data.sphereDataConfig;
		i_sphere_data.request_data_spectral();

		SphereData out_sph_data = SphereData(sphereDataConfig);


		SWEET_THREADING_SPACE_PARALLEL_FOR
		for (int m = 0; m <= i_sphere_data.sphereDataConfig->spectral_modes_m_max; m++)
		{
			int idx = i_sphere_data.sphereDataConfig->getArrayIndexByModes(m, m);

			for (int n = m; n <= i_sphere_data.sphereDataConfig->spectral_modes_n_max; n++)
			{
				out_sph_data.spectral_space_data[idx] =
							R(n-1,m)*i_sphere_data.spectral_get(n-1, m)
							+ S(n+1,m)*i_sphere_data.spectral_get(n+1, m);

				idx++;
			}
		}

		out_sph_data.physical_space_data_valid = false;
		out_sph_data.spectral_space_data_valid = true;

		return out_sph_data;
	}

	/**
	 * Compute
	 * mu*F(\lambda,\mu)
	 */
	SphereData mu2(
			const SphereData &i_sph_data
	)	const
	{
		const SphereDataConfig *sphereDataConfig = i_sph_data.sphereDataConfig;
		i_sph_data.request_data_spectral();

		SphereData out_sph_data = SphereData(sphereDataConfig);


		SWEET_THREADING_SPACE_PARALLEL_FOR
		for (int m = 0; m <= i_sph_data.sphereDataConfig->spectral_modes_m_max; m++)
		{
			int idx = i_sph_data.sphereDataConfig->getArrayIndexByModes(m, m);

			for (int n = m; n <= i_sph_data.sphereDataConfig->spectral_modes_n_max; n++)
			{
				out_sph_data.spectral_space_data[idx] =
						+A(n-2,m)*i_sph_data.spectral_get(n-2, m)
						+B(n+0,m)*i_sph_data.spectral_get(n+0, m)
						+C(n+2,m)*i_sph_data.spectral_get(n+2, m)
						;
				idx++;
			}
		}

		out_sph_data.physical_space_data_valid = false;
		out_sph_data.spectral_space_data_valid = true;

		return out_sph_data;
	}

#if 0
	/**
	 * Compute gradient component along latitude
	 */
	SphereData grad_lat(
			const SphereData &i_sph_data
	)	const
	{
		/*
		 * compute sin(theta)*d/d theta
		 * theta is the colatitude
		 *
		 * Hence, we have to
		 * 	first divide by sin(M_PI*0.5-phi) and
		 * 	second multiply by sqrt(1-mu*mu)
		 */
		SphereData out_sph_data = spectral_one_minus_mu_squared_diff_lat_mu(i_sph_data);

		out_sph_data.request_data_physical();
		out_sph_data.physical_update_lambda_gaussian_grid(
				[](double lambda, double mu, double &o_data)
				{
					//double phi = asin(mu);

					//o_data /= sin(M_PI*0.5-phi);
					//o_data /= ::cos(phi);
					o_data /= std::sqrt(1.0-mu*mu);
				}
		);

		return out_sph_data;

#if 0
		/**
		 * WARNING: Leave this code here
		 * We can see that the following operations would cancel out.
		 * Therefore this was commented.
		 */
		// undo the sin(theta) and multiply with sqrt(1-mu*mu)
		out_sph_data.request_data_physical();
		out_sph_data.physical_update_lambda_gaussian_grid(
				[this](double lambda, double mu, double &o_data)
				{
					double phi = asin(mu);

					//o_data /= sin(M_PI*0.5-phi);
					o_data /= ::cos(phi);

					double cos_phi = std::sqrt((double)(1.0-mu*mu));
					o_data *= cos_phi;
				}
			);
		return out_sph_data;
#endif

	}



	/**
	 * Divergence Operator along longitude
	 *
	 * Identical to gradient operator along longitude
	 */
	SphereData div_lon(
			const SphereData &i_sph_data
	)	const
	{
		return grad_lon(i_sph_data);
	}



	/**
	 * Divergence Operator along latitude
	 *
	 * d(sqrt(1-mu*mu)*F)/dmu
	 */
	SphereData div_lat(
			const SphereData &i_sph_data
	)	const
	{
		SphereData out_sph_data(i_sph_data);



		// TODO: replace this with a recurrence identity if possible
		out_sph_data.physical_update_lambda_cogaussian_grid(
				[](double lambda, double comu, double &o_data)
				{
					//o_data *= cos(phi);
					o_data *= comu;
				}
			);

		// grad_lat = diff_lat_phi
		out_sph_data = grad_lat(out_sph_data);

		// undo the sin(theta) which is cos(phi)
		out_sph_data.physical_update_lambda_cogaussian_grid(
				[](double lambda, double comu, double &o_data)
				{
					o_data /= comu;
					//o_data /= cos(phi);
				}
			);

		return out_sph_data;
	}
#endif



	/**
	 * Laplace operator
	 */
	SphereData laplace(
			const SphereData &i_sph_data
	)	const
	{
		SphereData out_sph_data(i_sph_data);
		out_sph_data.request_data_spectral();

		out_sph_data.spectral_update_lambda(
				[&](int n, int m, std::complex<double> &o_data)
				{
					o_data *= -(double)n*((double)n+1.0)*ir*ir;
				}
			);

		return out_sph_data;
	}


	/**
	 * Laplace operator
	 */
	SphereData inv_laplace(
			const SphereData &i_sph_data
	)	const
	{
		SphereData out(i_sph_data);

		out.spectral_update_lambda(
				[&](int n, int m, std::complex<double> &o_data)
				{
					if (n != 0)
						o_data /= -(double)n*((double)n+1.0)*ir*ir;
					else
						o_data = 0;
				}
			);

		out.spectral_space_data_valid = true;
		out.physical_space_data_valid = false;

		return out;
	}


};



#endif /* SPHOPERATORS_HPP_ */
