"""Fits the four ratio networks (sidechain drive, threshold offset) to target static curves.
   python3 tests/fit_ratios.py   -> paste the values into CircuitSpec::scDrive / scOffset."""
from fc import *
from scipy.optimize import minimize
import sys
R=[4,8,12,20]; T0=[-26,-24,-22,-20]
def target(L,r,t0,knee=8):
    x=L-t0
    over = np.where(x<-knee/2,0,np.where(x>knee/2,x,(x+knee/2)**2/(2*knee)))
    return -(1-1/r)*over
levels=np.arange(-36,1,4)
def curve(dr,off,ri,gg=1.0):
    out=[]
    for l in levels:
        c=FC({f'scDrive{ri}':dr, f'scOffset{ri}':off, f'gateGain{ri}':gg}); c.set(ratio=ri,att=3,rel=5)
        x=sine(1000,l,0.5); y=c.run(x); n=int(0.15*FS); out.append(rms_db(y[-n:])-rms_db(x[-n:]))   # real output change
    return np.array(out)
if __name__=='__main__':
    res={}
    for ri in range(4):
        def cost(p):
            dr,off,gg=p
            if dr<=0 or off<-0.5 or not (0.1<=gg<=1.5): return 1e3
            return np.sum((curve(dr,off,ri,gg)-target(levels,R[ri],T0[ri]))**2)
        start=[[12.0,0.3,0.3],[9.85,0.445,1.0],[10.95,0.754,1.0],[11.34,1.075,1.0]][ri]
        best=minimize(cost,start,method='Nelder-Mead',options={'maxiter':120,'xatol':1e-3,'fatol':1e-3})
        res[ri]=best.x; g=curve(*best.x[:2],ri,best.x[2])
        print('ratio',R[ri],'drive %.3f offset %.3f gateGain %.3f cost %.2f'%(*best.x,best.fun))
        print('   out', ' '.join('%+.1f'%v for v in g)); print('   tgt', ' '.join('%+.1f'%v for v in target(levels,R[ri],T0[ri])))
        sys.stdout.flush()
    print('scDrive', [round(res[i][0],3) for i in range(4)], 'scOffset', [round(res[i][1],3) for i in range(4)], 'gateGain', [round(res[i][2],3) for i in range(4)])
