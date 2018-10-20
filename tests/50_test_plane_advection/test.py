#! /usr/bin/env python3

import sys

from SWEET import *
from itertools import product
from mule.exec_program import *

exec_program('mule.benchmark.cleanup_all', catch_output=False)

jg = SWEETJobGeneration()
jg.compile.unit_test="test_plane_advection"

jg.compile.plane_spectral_space="enable"
jg.runtime.benchmark_name = "gaussian_bump_advection"

jg.runtime.simtime = 20
jg.runtime.verbosity = 5

params_domain_size_scales = [1, 2]
params_domain_size_scales = [2]

#params_compile_mode = ['release', 'debug']
params_compile_mode = ['release']
#params_compile_plane_spectral_dealiasing = ['enable', 'disable']
params_compile_plane_spectral_dealiasing = ['enable']

params_runtime_spectral_derivs = [0, 1]

#params_runtime_mode_res_x = [64, 128]
#params_runtime_mode_res_y = [64, 128]
params_runtime_mode_res_x = [128, 192]
params_runtime_mode_res_y = [128, 192]

params_runtime_timestep_sizes = [0.05, 0.1]

params_rotation_velocity = [1, 20]
params_advection_velocity_u = [0.1, -0.2]
params_advection_velocity_v = [-0.1, 0.2]

params_runtime_ts_methods = [
		# [ method_id, order ]
		["na_sl", 1],
		["na_sl", 2],
		["nl_erk", 2],
		["nl_erk", 4],
	]


for (res_x, res_y) in product(params_runtime_mode_res_x, params_runtime_mode_res_y):
	jg.runtime.mode_res = (res_x, res_y)

	# Try out different variants of domain size
	for jg.runtime.domain_size in product(params_domain_size_scales, params_domain_size_scales):

		# Iterate over time stepping methods and the order
		for ts_method in params_runtime_ts_methods:
			jg.runtime.timestepping_method = ts_method[0]
			jg.runtime.timestepping_order = ts_method[1]

			for vel in product(
				params_advection_velocity_u,
				params_advection_velocity_v,
				params_rotation_velocity,
			):
				jg.runtime.advection_velocity = ",".join(str(x) for x in vel)

				for (
					jg.compile.mode,
					jg.compile.plane_spectral_dealiasing,
					jg.runtime.spectralderiv,
					jg.runtime.timestep_size,
				) in product(
					params_compile_mode,
					params_compile_plane_spectral_dealiasing,
					params_runtime_spectral_derivs,
					params_runtime_timestep_sizes,
				):
					jg.gen_jobscript_directory()

exitcode = exec_program('mule.benchmark.jobs_run_directly', catch_output=False)
if exitcode != 0:
	sys.exit(exitcode)

print("Benchmarks successfully finished")

exec_program('mule.benchmark.cleanup_all', catch_output=False)