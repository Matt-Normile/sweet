#! /bin/bash

FILE="prog_h_t00000000000.00111111.csv"

for i in linear_gaussian_*; do
	./compute_norm.py "linear_gaussian_dam_rk4_robert_t0.0001_stable/$FILE" "$i/$FILE"
done