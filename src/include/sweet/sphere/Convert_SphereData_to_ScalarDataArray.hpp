/*
 * SphereData_To_ScalarDataArray.cpp
 *
 *  Created on: 20 Oct 2016
 *      Author: Martin Schreiber <SchreiberX@gmail.com>
 */

#ifndef SRC_INCLUDE_SWEET_SPHERE_CONVERT_SPHEREDATA_TO_SCALARDATAARRAY_HPP_
#define SRC_INCLUDE_SWEET_SPHERE_CONVERT_SPHEREDATA_TO_SCALARDATAARRAY_HPP_

#include <sweet/sphere/SphereData.hpp>
#include <sweet/ScalarDataArray.hpp>

class Convert_SphereData_To_ScalarDataArray
{
public:
	static
	ScalarDataArray physical_convert(
			const SphereData &i_sphereData,
			bool i_raise_error_if_spectral = true
	)
	{
		ScalarDataArray out(i_sphereData.sphereDataConfig->physical_array_data_number_of_elements);

#if SWEET_USE_SPHERE_SPECTRAL_SPACE
		if (i_sphereData.spectral_space_data_valid && i_raise_error_if_spectral)
			FatalError("This data should be typically never converted to spectral space");
#endif

		i_sphereData.request_data_physical();

		for (std::size_t i = 0; i < out.number_of_elements; i++)
			out.scalar_data[i] = i_sphereData.physical_space_data[i];

		return out;
	}
};



#endif /* SRC_INCLUDE_SWEET_SPHERE_CONVERT_SPHEREDATA_TO_SCALARDATAARRAY_HPP_ */
