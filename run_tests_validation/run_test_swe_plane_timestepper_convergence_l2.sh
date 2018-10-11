#! /bin/bash


echo "***********************************************"
echo "Running convergence tests"
echo "***********************************************"

# set close affinity of threads
export OMP_PROC_BIND=close


BASEDIR=`pwd`

cd "$BASEDIR/run_test_swe_plane_timestepper_convergence"


./cleanup.sh

# encoding: group | tsm | order1 | order2 | rexi_direct_solution

#if true; then
if false; then
	# 4th order accurate scheme
#	./jobs_create.py ln4 ln_erk 4 4 0
	./jobs_create.py ln4 l_rexi_n_etdrk 4 4 1 || exit 1
else
	# 2nd order linear
	./jobs_create.py l2 l_cn 2 0 0 || exit 1
	./jobs_create.py l2 l_erk 2 0 0 || exit 1
fi


#./compile.sh || exit 1

./jobs_run.sh || exit 1



echo "***********************************************"
echo " POSTPROCESSING "
echo "***********************************************"
./postprocessing.py || exit 1


./cleanup.sh

echo "***********************************************"
echo "***************** FIN *************************"
echo "***********************************************"

