#! /bin/bash

#SBATCH -o /home/hpc/pr63so/di69fol/workspace/SWEET_2016_01_16/benchmarks_performance/rexi_tests/2016_01_06_scalability_rexi_fd/run_rexi_fd_par_m8192_t001_n0128_r0002_a1.txt
###SBATCH -e /home/hpc/pr63so/di69fol/workspace/SWEET_2016_01_16/benchmarks_performance/rexi_tests/2016_01_06_scalability_rexi_fd/run_rexi_fd_par_m8192_t001_n0128_r0002_a1.err
#SBATCH -J rexi_fd_par_m8192_t001_n0128_r0002_a1
#SBATCH --get-user-env
#SBATCH --clusters=mpp2
#SBATCH --ntasks=2
#SBATCH --cpus-per-task=1
#SBATCH --exclusive
#SBATCH --export=NONE
#SBATCH --time=00:05:00

#declare -x NUMA_BLOCK_ALLOC_VERBOSITY=1
declare -x KMP_AFFINITY="granularity=thread,compact,1,0"
declare -x OMP_NUM_THREADS=1


echo "OMP_NUM_THREADS=$OMP_NUM_THREADS"
echo


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

cd /home/hpc/pr63so/di69fol/workspace/SWEET_2016_01_16/benchmarks_performance/rexi_tests/2016_01_06_scalability_rexi_fd
cd ../../../

. local_software/env_vars.sh



# force to use FFTW WISDOM data
declare -x SWEET_FFTW_LOAD_WISDOM_FROM_FILE="FFTW_WISDOM_nofreq_T0"

time -p mpiexec.hydra -genv OMP_NUM_THREADS 1 -envall -ppn 28 -n 2 ./build/rexi_fd_par_m_tno_a1  --initial-freq-x-mul=2.0 --initial-freq-y-mul=1.0 -f 1  -g 1 -H 1 -X 1 -Y 1 --compute-error 1 -t 50 -R 4 -C 0.3 -N 128 -U 0 -S 0 --use-specdiff-for-complex-array 0 --rexi-h 0.8 --timestepping-mode 1 --staggering 0 --rexi-m=8192 -C -5.0

