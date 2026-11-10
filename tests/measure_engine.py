"""Measures the circuit engine and writes docs/measurements.md:
static curves for every ratio, attack/release times, distortion, rate invariance.
   python3 tests/measure_engine.py"""
import os, numpy as np
from fc import *
HERE=os.path.dirname(os.path.abspath(__file__))
def thd(y,f0,fs=FS):
    n=len(y); Y=np.abs(np.fft.rfft(y*np.blackman(n))); f=np.fft.rfftfreq(n,1/fs)
    h=[Y[np.argmin(abs(f-f0*k))] for k in range(1,10)]
    return 100*np.sqrt(sum(x*x for x in h[1:]))/h[0], 20*np.log10(h[1]/h[0]), 20*np.log10(h[2]/h[0])
def step(att,rel,ratio,lo,hi,dur_hi,dur_lo,f=1000):
    a=sine(f,lo,0.3); b=sine(f,hi,dur_hi); c=sine(f,lo,dur_lo); x=np.concatenate([a,b,c])
    k=FC(); k.set(att=att,rel=rel,ratio=ratio); y,g=k.run_gr(x); return g,len(a),len(a)+len(b)
out=['# Epsilon FET - engine measurements','','Measured by `tests/measure_engine.py` on the circuit engine at 176.4 kHz (4x 44.1 kHz). '
     'Signals are 1 kHz sines unless stated; levels are dBFS at the plugin input, with -18 dBFS peak = +4 dBu. '
     'These are measurements of the model against the published 1176 specifications, not of a hardware unit.','']
out+=['## Static compression (gain reduction in dB, Input 0 dB, attack 3, release 5)','','| Input level | '+' | '.join(['4:1','8:1','12:1','20:1','All']) +' |','|---|---|---|---|---|---|']
levels=list(range(-36,1,6)); cols=[]
for r in range(5): cols.append([gr for _,_,gr in static_curve(ratio=r,levels=levels,rel=5)])
for i,l in enumerate(levels): out.append(f'| {l} dBFS | '+' | '.join('%.1f'%cols[r][i] for r in range(5))+' |')
out+=['','Slope above the knee (output dB per input dB, from +6 to +18 dB above threshold):','']
for r,name in enumerate(['4:1','8:1','12:1','20:1']):
    lv=np.arange(-30,1,2); c=static_curve(ratio=r,levels=lv,rel=5); gain=np.array([g for _,g,_ in c]); outl=lv+gain
    t0=[-26,-24,-22,-20][r]; m=(lv>=t0+6)&(lv<=t0+18); sl=np.polyfit(lv[m],outl[m],1)[0]
    out.append(f'- {name}: measured {sl:.3f} dB/dB = {1/sl:.1f}:1')
out+=['','## Timing (step -40 to -12 dBFS and back, ratio 4:1)','','| Knob | Attack, 63 % of final GR | Release, 63 % recovery |','|---|---|---|']
for kn in [7,6,5,4,3,2,1]:
    g,i0,i1=step(kn,4,0,-40,-12,0.3,0.1); fin=g[i1-int(0.02*FS):i1].mean(); ga=g[i0:]; at=np.argmax(ga<=0.632*fin)/FS*1e6
    g2,i0b,i1b=step(4,kn,0,-40,-6,0.6,3.0); g0=g2[i1b-int(0.02*FS):i1b].mean(); gr=g2[i1b:]; fin2=gr[-int(0.1*FS):].mean(); rt=np.argmax(gr>=g0+0.632*(fin2-g0))/FS*1e3
    out.append(f'| {kn} | {at:.0f} us | {rt:.0f} ms |')
out+=['','Published 1176 ranges: attack 20 us - 800 us, release 50 ms - 1.1 s. At 1 kHz the fastest attack settings are limited by the waveform itself '
      '(the detector cannot charge before the sine rises); the detector time constant at knob 7 is 20 us (90 ohm x 0.22 uF).','']
out+=['## Distortion (THD, harmonics 2-9)','','| Condition | THD | H2 | H3 |','|---|---|---|---|']
cases=[('1 kHz -18 dBFS, compressor off',dict(comp=0),1000,-18),('1 kHz -6 dBFS, compressor off',dict(comp=0),1000,-6),
       ('1 kHz -18 dBFS, 4:1, ~4 dB GR',dict(ratio=0),1000,-18),('1 kHz -6 dBFS, 20:1, ~13 dB GR',dict(ratio=3),1000,-6),
       ('1 kHz -6 dBFS, all buttons',dict(ratio=4),1000,-6),('100 Hz -10 dBFS, 4:1, attack 7 / release 7',dict(ratio=0,att=7,rel=7),100,-10),
       ('100 Hz -10 dBFS, 4:1, attack 3 / release 3',dict(ratio=0,att=3,rel=3),100,-10),('50 Hz -10 dBFS, 4:1, attack 7 / release 7',dict(ratio=0,att=7,rel=7),50,-10)]
for name,kw,f,l in cases:
    k=FC(); k.set(**kw); y=k.run(sine(f,l,1.0)); n=int(0.5*FS); t,h2,h3=thd(y[-n:],f)
    out.append(f'| {name} | {t:.2f} % | {h2:.0f} dB | {h3:.0f} dB |')
out+=['','Fast release on bass ripples the FET gate within each cycle: that is the hardware\'s known low-frequency grit, and it disappears with slower settings.','']
out+=['## Sample-rate invariance (GR at -10 dBFS, 4:1)','']
for fs in [88200,96000,176400,192000,384000]:
    k=FC(None,1,fs); k.set(ratio=0); x=sine(1000,-10,0.5,fs); y,g=k.run_gr(x); out.append(f'- {fs/1000:g} kHz internal: {g[-int(0.1*fs):].mean():.2f} dB')
os.makedirs(os.path.join(HERE,'..','docs'),exist_ok=True)
open(os.path.join(HERE,'..','docs','measurements.md'),'w').write('\n'.join(out)+'\n')
print('\n'.join(out))
