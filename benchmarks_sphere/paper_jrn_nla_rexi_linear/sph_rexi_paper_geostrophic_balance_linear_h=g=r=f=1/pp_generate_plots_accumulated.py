#! /usr/bin/env python2

import matplotlib
matplotlib.use('Agg')

import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
import numpy as np
import sys
import os

plt.figure(figsize=(9, 5))
fontsize = 12
logscale = True

# 2: height
# 3: velocity u
# 4: velocity v
column=2

files = sys.argv[1:]


output_filename = 'output.pdf'
if files[0] == 'output':
	output_filename = files[1]
	files = files[2:]

num_plots = len(files)

# set color map
colormap = plt.cm.gist_ncar
plt.gca().set_color_cycle([colormap(i) for i in np.linspace(0, 0.9, num_plots)])

# markers
markers = []
for m in Line2D.markers:
    try:
        if len(m) == 1 and m != ' ':
            markers.append(m)
    except TypeError:
        pass


title = "Geostrophic balance benchmark"
subtitles = []

#plt.xticks(labelsx, fontsize=fontsize)
plt.xlabel("Simulation time", fontsize=fontsize)

#plt.yticks(labelsy, fontsize=fontsize)
plt.ylabel("Lmax error in height", fontsize=fontsize)
if logscale:
	plt.yscale("log", nonposy='clip')

legend_labels=[]
for i in range(0, len(files)):
	f = files[i]

	data = np.loadtxt(f, skiprows=0, usecols=(1,2), delimiter='\t')

	# remove first data point which would be zero
	if logscale:
		data = data[1:]

	data = data.transpose()

	marker = markers[i % len(markers)]

	plt.plot(data[0], data[1], linewidth=1.0, linestyle='-', marker=marker)

	# RK2
	f = f.replace('script_modes064_bench10_nonlin0_g1_h1_f1_a1_u0_pdeid1_tsm1_tso2_rexim00256_rexih0.15_rexinorm0_rexihalf1_rexiextmodes02_rexipar1_C00000.01_t000000.1_o00000.01_robert1.err', 'Runge-Kutta order 2')

	f = f.replace('script_modes064_bench10_nonlin0_g1_h1_f1_a1_u0_pdeid0_', '')
	f = f.replace('_rexih0.15', '')

	f = f.replace('_rexim', 'REXI M=')
	f = f.replace('M=0', 'M=')
	f = f.replace('M=0', 'M=')
	f = f.replace('M=0', 'M=')
	f = f.replace('M=0', 'M=')

	f = f.replace('_rexinorm0', '')
	f = f.replace('_rexinorm1', '')

	if False:
		f = f.replace('_rexihalf0', ', half=0')
		f = f.replace('_rexihalf1', ', half=1')
	else:
		f = f.replace('_rexihalf0', '')
		f = f.replace('_rexihalf1', '')

	f = f.replace('_rexinorm0', ', norm=0')
	f = f.replace('_rexinorm1', ', norm=1')

	f = f.replace('_rexihalf0', ', half=0')
	f = f.replace('_rexihalf1', ', half=1')

	f = f.replace('_rexiextmodes02_rexipar1_C00000.01_t000000.1_o00000.01_robert1', '')
	f = f.replace('.err', '')

	legend_labels.append(f)



if 'rexinorm0' in files[0]:
	subtitles.append('without normalization for geostrophic modes')
else:
	subtitles.append('with normalization for geostrophic modes')

if 'rexihalf0' in files[0]:
	subtitles.append('without halving REXI poles')
else:
	subtitles.append('with halving REXI poles')


# Title
plt.suptitle(title)
plt.title(', '.join(subtitles), fontsize=fontsize*0.7)

plt.legend(legend_labels, ncol=1, loc='lower right'
#           bbox_to_anchor=[0.5, 1.1], 
#           columnspacing=1.0, labelspacing=0.0,
#           handletextpad=0.0, handlelength=1.5,
#           fancybox=True, shadow=True
	)

print(output_filename)

if '.png' in output_filename:
	plt.savefig(output_filename, dpi=200)
else:
	plt.savefig(output_filename)
plt.close()

