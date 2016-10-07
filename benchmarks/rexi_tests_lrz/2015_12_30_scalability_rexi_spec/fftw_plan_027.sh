#! /bin/bash

#SBATCH -o fftw_plan_027.txt
#SBATCH -J fftw_plan_027
#SBATCH --get-user-env
#SBATCH --clusters=mpp2
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --exclusive
#SBATCH --export=NONE
#SBATCH --time=03:00:00

#declare -x NUMA_BLOCK_ALLOC_VERBOSITY=1
declare -x KMP_AFFINITY="granularity=thread,compact,1,0"
declare -x OMP_NUM_THREADS=1


. /etc/profile.d/modules.sh

module unload gcc
module unload fftw

module unload python
module load python/2.7_anaconda_nompi


module unload intel
module load intel/16.0

module unload mpi.intel
module load mpi.intel/5.1

module load gcc/5

cd /home/hpc/pr63so/di69fol/workspace/SWEET_2015_12_26/benchmarks_performance/rexi_tests_lrz_freq_waves/2015_12_30_scalability_rexi_spec
cd ../../../

. local_software/env_vars.sh



mpiexec.hydra -genv OMP_NUM_THREADS 27 -envall -ppn 1 ./fftw_gen_wisdoms_all.sh 27 FFTW_WISDOM_nofreq_T27
