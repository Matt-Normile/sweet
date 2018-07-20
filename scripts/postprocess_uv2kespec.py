#! /usr/bin/python3
# Plot unstable jet fields
# 
#--------------------------------------

import matplotlib
matplotlib.use('Agg')

import matplotlib.pyplot as plt
import matplotlib.colors as colors
import matplotlib.ticker as ticker


import scipy
from scipy.interpolate import RectBivariateSpline

import numpy as np
import sys
from SWEETParameters import *

if len(sys.argv) <= 1:
	print("Argument 1 must be the zonal velocities")
	print("Argument 2 must be the meridional velocities")
	sys.exit()
    
    
#Figure definitions
fontsize=18
figsize=(9, 7)

#for filename in sys.argv[1:]:

#Load data
ufile=sys.argv[1]
vfile=sys.argv[2]

if 'prog_u' not in ufile:
	print("Argument 1 must be the zonal velocities")
	sys.exit()
if 'prog_v' not in vfile:
	print("Argument 2 must be the meridional velocities")
	sys.exit()
	
udata = np.loadtxt(ufile)
vdata = np.loadtxt(vfile)
print("Dimensions (u,v)")
print(udata.shape, vdata.shape)

uspec=np.fft.fft2(udata)/udata.shape[0]/udata.shape[1]
vspec=np.fft.fft2(vdata)/vdata.shape[0]/vdata.shape[1]

print("Spectral Dimensions (u,v)")
print(uspec.shape, vspec.shape)

#see https://journals.ametsoc.org/doi/10.1175/1520-0469%282001%29058<0329%3ATHKESA>2.0.CO%3B2
data=0.25*(np.multiply(uspec,np.conjugate(uspec)))+0.25*(np.multiply(vspec,np.conjugate(vspec)))
data=data.real

n=data.shape[0]
print(n)

data=data[0:int(n/2)-1, 0:int(n/2)-1]


#Get max/min
cmin = np.amin(data)
cmax = np.amax(data)

earth = EarthMKSDimensions()
benchpar = Unstablejet()

#Domain
xL_min = benchpar.x_min
xL_max = benchpar.x_max
yL_min = benchpar.y_min
yL_max = benchpar.y_max

#Set physical grid for axis
n = data.shape[1]
#m=int(n/2)+1
m=int(2*n/3)-1 #anti-aliasing cut
#m=10
x_min = 0
x_max = int(m)
y_min = 0
y_max = int(m)
x = np.linspace(x_min, x_max, m+1)
y = np.linspace(y_min, y_max, m+1)

#Labels
labelsx = np.linspace(x_min, x_max, 10)
labelsy = np.linspace(y_min, y_max, 10)

#Set tittle

title="Kinetic Energy Spectrum "

filename=ufile
#Method
print("Methods")
pos1 = filename.find('_tsm_')
pos2 = filename.find('_tso')
method1 = filename[pos1+5:pos2]
print(method1)

if method1 == "l_cn_na_sl_nd_settls":
	method1 = "SL-SI-SETTLS"
elif method1 == "l_rexi_na_sl_nd_settls":
	method1 = "SL-EXP-SETTLS"
elif method1 == "l_rexi_na_sl_nd_etdrk":
	method1 = "SL-ETD2RK"
elif method1 == "l_rexi_n_etdrk":
	method1 = "ETD2RK"
elif method1 == "ln_erk":
	if 'ref' in filename:
		method1 = "REF"
	else:
		method1 = "RK-FDC"
		
title+= method1
title+= " "
		
title += 't='
pos1 = filename.find('output')
name = filename[pos1:]
pos2 = name.find('_t')
pos3 = filename.find('.csv')
time = filename[pos1+pos2+2:pos3]
time = float(time)
time = time / 86400
title += str(time)
title += ' days '

if time > 0 :
	title+=" dt="
	pos1 = filename.find('_C')
	pos2 = filename.find('_R')
	title += filename[pos1+2:pos2]
	
print(title)


#Start plotting 2d figure
plt.figure(1, figsize=figsize)

#2D plot
datalog=data[0:m,0:m]+1
datalog=np.log(datalog)
cmin = np.amin(datalog)
cmax = np.amax(datalog)
extent = (labelsx[0], labelsx[-1], labelsy[0], labelsy[-1])
plt.imshow(datalog, interpolation='nearest', extent=extent, origin='lower', aspect='auto', cmap=plt.get_cmap('jet'))
plt.clim(cmin, cmax)	
cbar = plt.colorbar()	
		
if 'ke' in filename:	
	cbar.set_label('Kinetic Energy ($m^2s^{-2}$)', rotation=270, labelpad=+20, size=fontsize)
	cbar.ax.tick_params(labelsize=fontsize) 


plt.title(title, fontsize=fontsize)

#Axis
ax = plt.gca()
ax.xaxis.set_label_coords(0.5, -0.075)

plt.xticks(fontsize=fontsize)
plt.yticks(fontsize=fontsize)

#plt.xticks(labelsx, fontsize=fontsize)
plt.xlabel("x mode", fontsize=fontsize)

#plt.yticks(labelsy, fontsize=fontsize)
plt.ylabel("y mode", fontsize=fontsize)


#Save file as eps
outfilename = filename.replace('.csv', '_2D.eps')

outfilename = outfilename.replace('/output', '')
print(outfilename)
plt.savefig(outfilename, dpi=300, transparent=True, bbox_inches='tight', \
					pad_inches=0)

plt.close()


#Start plotting 1d figure
plt.figure(2, figsize=figsize)

#Calculate energy per shell
r=np.arange(0, m+1, 1) #radius
energy=np.zeros(m+1)
shell_pattern=np.zeros((m+1, m+1))
print("Generating energy in shells (Each x is 1/", m, ")")
for i in range(0,m):
	for j in range(0,m):
		k=np.sqrt(pow(float(i),2)+pow(float(j),2))
		intk=int(k)
		if intk < m :
			energy[intk]=energy[intk]+data[i,j]
			shell_pattern[i,j]=intk
	print(".", end='', flush=True)
	#print(i, j, k, intk, data[i,j], energy[intk], data.shape, energy.shape)
			
print(".")
#Quick check to see if things match
print("Energy in shells: ", energy[0:10])
print("Energy in first column of data: ", data[0:10,0])
#print("Shell modes: ", r[0:10])
#print("Pattern:\n", shell_pattern[0:10,0:10])



#r_ref53=r[-300:-200]

#offsetx=10
#offsety=0.01
#en_ref53=np.array([])
#for tmp in r_ref53:
#	ytmp=np.power(offsety*tmp, -float(5.0/3.0))*offsetx
	#print(tmp, ytmp)
#	en_ref53=np.append(en_ref53, [ytmp])
	#print(en_ref53)

r_ref3=r[-int(m/2):-1]

offsetx=m*1000
offsety=0.005
en_ref3=np.array([])
en_ref53=np.array([])
i=r_ref3[0]
iref=(energy[1]/10.0)/np.power(float(i), -3)
for tmp in r_ref3:
	ytmp=np.power(tmp, -float(3.0))*iref
	en_ref3=np.append(en_ref3, [ytmp])

iref=(energy[1]/10.0)/np.power(float(i), -float(5.0/3.0))	
for tmp in r_ref3:
	ytmp=np.power(tmp, -float(5.0/3.0))*iref
	en_ref53=np.append(en_ref53, [ytmp])
	
plt.title(title, fontsize=fontsize, y=1.02)

#Convert wavenumber to wavelength
r=xL_max*1000/r
r_ref3=xL_max*1000/r_ref3

plt.loglog(r, energy)
#plt.loglog(r_ref53, en_ref53, '-.', color='black')
plt.loglog(r_ref3, en_ref3, '-.', color='black')
plt.loglog(r_ref3, en_ref53, '-.', color='black')

#Axis
ax = plt.gca()
ax.annotate("$k^{-5/3}$", xy=(r_ref3[-1]+5, en_ref53[-1]), fontsize=fontsize)
ax.annotate("$k^{-3}$", xy=(r_ref3[-1]+5, en_ref3[-1]), fontsize=fontsize)
ax.spines['top'].set_visible(False)
ax.spines['right'].set_visible(False)

ax.xaxis.set_label_coords(0.5, -0.075)

plt.gca().invert_xaxis()

plt.xticks(fontsize=fontsize)
plt.yticks(fontsize=fontsize)

#plt.xticks(labelsx, fontsize=fontsize)
plt.xlabel("Horizontal wavenumber", fontsize=fontsize)

plt.xlabel("Horizontal wavelength (km)", fontsize=fontsize)

#plt.yticks(labelsy, fontsize=fontsize)
plt.ylabel("Kinetic Energy $(m^2s^{-2})$", fontsize=fontsize)


#Save file as eps
outfilename = filename.replace('prog_u', 'kenergyspec')
outfilename = outfilename.replace('.csv', '_1D.eps')

outfilename = outfilename.replace('/output', '')
print(outfilename)
plt.savefig(outfilename, dpi=300, transparent=True, bbox_inches='tight', \
					pad_inches=0)

plt.close()
