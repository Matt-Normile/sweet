#! /bin/bash


cd ../../ || exit 1

make clean
SCONS="scons --program=swe_rexi --gui=disable"
echo "$SCONS"
$SCONS || exit 1

H0=1
CFL=0.1
SIM_TIME=0.2
SIM_OUTPUT_TIME=0.1

BENCH_ID=1

RK=4

echo
echo "*********************************************************"
echo "* Running benchmarks                                    *"
echo "*********************************************************"
echo

for i in `seq 2 12`; do
	N=$((2**i))
	echo -en "$N:\t"
	
	EXEC="./build/swe_rexi_* -R $RK -C $CFL -N $N -s $BENCH_ID -H $H0 -g 1 -f 1 -t $SIM_TIME -o $SIM_OUTPUT_TIME -v 1 -G 0 --nonlinear=0 -S 1 --compute-error 1"
#	echo "$EXEC"
	echo -en "N=$N:\t"
	$EXEC | grep "DIAGNOSTICS ANALYTICAL MAXABS H"
done
