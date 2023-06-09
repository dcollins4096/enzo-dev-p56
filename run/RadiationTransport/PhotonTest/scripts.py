from yt.mods import *
from matplotlib import use; use('Agg')
import matplotlib.pyplot as plt

# Calculate
# 1. Ionization front propagation and compare to Stroemgren solution
# 2. Slice of photo-ionization field
# 3. Deviation from 1/r^2 in the photo-ionization field in the last output

first = 1
last = 25

Myr = 3.1557e13
kpc = 3.086e21

def _MyRadius(field, data):
    center = data.get_field_parameter("center")
    dx = data["x"] - center[0]
    dy = data["y"] - center[1]
    dz = data["z"] - center[2]
    return na.sqrt(dx*dx + dy*dy + dz*dz)
add_field("Radius", function=_MyRadius, take_log=False, units='')

def _MyNeutralFrac(field, data):
    return data['HI_Fraction'] / 0.75908798
add_field("Neutral_Fraction", function=_MyNeutralFrac, take_log=False,
          units='')

center = [1e-3]*3
time = []
radius = []

for i in range(first, last+1):
    amrfile = "DD%4.4d/data%4.4d" % (i,i)
    pf = load(amrfile)

    x_bins_1d = 32
    r_min = pf.h.get_smallest_dx()
    r_max = 1.0 - 1.0/64
    sphere = pf.h.sphere(center, r_max)

    prof1d = BinnedProfile1D(sphere, x_bins_1d, "Radius", r_min, r_max,
                             log_space=False, lazy_reader=True)
    prof1d.add_fields("Neutral_Fraction")
    prof1d["Neutral_Fraction"][-1] = prof1d["Neutral_Fraction"][-2]

    # Find the radius of the I-front (f_HI=0.5)
    res = na.abs(prof1d["Neutral_Fraction"] - 0.5)
    ir = na.where(res == res.min())[0]
    r = na.interp(0.5, prof1d["Neutral_Fraction"], prof1d["Radius"])

    time.append(pf["InitialTime"] * pf["Time"])
    radius.append(r * pf["cm"])

    del pf
    del prof1d

time = na.array(time)
radius = na.array(radius)

# Calculate analytic Stroemgren radius
alpha = 2.59e-13  # Recombination rate at T=1e4 K
nH = 1e-3    # Hydrogen density
lum = 5e48   # Ionizing photon luminosity
trec = 1.0 / (alpha * nH)  # Recombination time
rs = ((3 * lum) / (4*na.pi*alpha*nH**2))**(1.0/3)
anyl_radius = rs * (1.0 - na.exp(-time / trec))**(1.0/3)
ratio = radius / anyl_radius
error = na.abs(1-ratio)
imax = na.where(error == error.max())[0]

p = plt.subplot(211)
p.plot(time/Myr, radius/kpc, 'ko')
p.plot(time/Myr, anyl_radius/kpc, 'k-')
p.set_ylabel(r'$r_{\rm IF}$ (kpc)')
p = plt.subplot(212)
p.plot(time/Myr, ratio, 'k-')
p.set_ylabel(r'$r_{\rm IF} / r_{\rm anyl}$')
p.set_xlabel("Time (Myr)")
plt.savefig("StroemgrenRadius.png")

########################################################################
# Some basic analysis on the final output
########################################################################

pf = load("DD%4.4d/data%4.4d" % (last,last))

pc = PlotCollection(pf, center=[0.5,0.5,1.0/64])
pc.add_slice('HI_kph',2)
pc.add_slice('Neutral_Fraction',2)
pc.add_slice('Electron_Fraction',2)
pc.save()
del pc

########################################################################
# Calculate deviation from 1/r^2 in the last output (inside 0.5*anyl_radius)
########################################################################

x_bins_1d = 32
r_min = 2*pf.h.get_smallest_dx()
r_max = 0.5*anyl_radius[-1] / pf['cm']
sphere = pf.h.sphere(center, r_max)

prof1d = BinnedProfile1D(sphere, x_bins_1d, "Radius", r_min, r_max,
                         log_space=False, lazy_reader=True)
prof1d.add_fields("HI_kph")
coeff, residual, tr1, tr2, tr3 = \
       na.polyfit(na.log(prof1d['Radius'][:-1]),
                  na.log(prof1d['HI_kph'][:-1]), 1, full=True)

print "="*72
print "Maximum error in ionization front radius = %g (at %f Myr)" % \
      (error.max(), time[imax]/Myr)
print "Average error in ionization front radius = %g" % (error.mean())
print "Inside 2*r_anyl: Radiation field slope = %f +/- %g" % \
      (coeff[0], residual)
print "="*72
