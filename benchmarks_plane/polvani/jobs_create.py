#! /usr/bin/env python3

import os
import sys
import stat
import math

from SWEETJobGeneration import *
p = SWEETJobGeneration()

#p.cluster.setupTargetMachine("cheyenne")
p.cluster.setupTargetMachine("")


#
# Run simulation on plane or sphere
#
p.compile.program = 'swe_plane_rexi'

prefix=p.compile.program

p.compile.plane_or_sphere = 'sphere'
p.compile.plane_spectral_space = 'enable'
p.compile.plane_spectral_dealiasing = 'enable'
p.compile.sphere_spectral_space = 'disable'
p.compile.sphere_spectral_dealiasing = 'disable'


p.compile.compiler = 'gnu'


#
# Use Intel MPI Compilers
#
#p.compile.compiler_c_exec = 'mpicc'
#p.compile.compiler_cpp_exec = 'mpicxx'
#p.compile.compiler_fortran_exec = 'mpif90'


#
# Activate Fortran source
#
#p.compile.fortran_source = 'enable'


#
# MPI?
#
#p.compile.sweet_mpi = 'enable'


# Verbosity mode
p.runtime.verbosity = 2

#
# Mode and Physical resolution
#
p.runtime.mode_res = 256
p.runtime.phys_res = -1

#
# Benchmark ID
# 4: Gaussian breaking dam
# 100: Galewski
#
p.runtime.benchmark_name = 'polvani'

#
# Compute error
#
p.runtime.compute_error = 0

#
# Preallocate the REXI matrices
#
p.runtime.rexi_sphere_preallocation = 0

#
# Deactivate stability checks
#
p.stability_checks = 0

#
# Threading accross all REXI terms

if True:
	p.compile.threading = 'omp'
	p.compile.rexi_thread_parallel_sum = 'disable'

else:
	#
	# WARNING: rexi_thread_par does not work yet!!!
	# MPI Ranks are clashing onthe same node with OpenMP Threads!
	#rexi_thread_par = True
	rexi_thread_par = False

	if rexi_thread_par:
		# OMP parallel for over REXI terms
		p.compile.threading = 'off'
		p.compile.rexi_thread_parallel_sum = 'enable'
	else:
		p.compile.threading = 'omp'
		p.compile.rexi_thread_parallel_sum = 'disable'


p.runtime.timestep_size = 0.001
p.runtime.simtime = 1000
p.runtime.output_timestep_size = 10




#
# allow including this file
#
if __name__ == "__main__":
	p.runtime.timestepping_method = 'ln_erk'
	p.runtime.timestepping_order = 4

	for [p.runtime.polvani_rossby, p.runtime.polvani_froude] in [[0.01, 0.04], [0.05, 0.05], [0.05, 0.075], [0.025, 0.025]]:
		p.gen_script('script_'+prefix+p.runtime.getUniqueID(p.compile)+'_'+p.cluster.getUniqueID(), 'run.sh')

