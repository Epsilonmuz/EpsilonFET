"""ctypes driver for the circuit engine at its internal rate (used by the measurement scripts)."""
import ctypes, os, subprocess, numpy as np
HERE=os.path.dirname(os.path.abspath(__file__))
_lib=os.path.join(HERE,'libfc.so'); _src=os.path.join(HERE,'engine_capi.cpp'); _hdr=os.path.join(HERE,'..','Source','FetCircuit.h')
if not os.path.exists(_lib) or os.path.getmtime(_lib) < max(os.path.getmtime(_src), os.path.getmtime(_hdr)):
    subprocess.run(['g++','-O2','-std=c++17','-shared','-fPIC',_src,'-o',_lib],check=True)
L=ctypes.CDLL(_lib)
D=ctypes.c_double; P=ctypes.c_void_p
L.fc_create.restype=P; L.fc_create.argtypes=[D,ctypes.c_int]
L.fc_set.argtypes=[P,D,D,D,D,ctypes.c_int,ctypes.c_int,ctypes.c_int]
L.fc_spec.argtypes=[P,ctypes.c_char_p,D]; L.fc_prepare.argtypes=[P,D,ctypes.c_int]
L.fc_process.argtypes=[P,ctypes.POINTER(D),ctypes.c_int,ctypes.c_int]
L.fc_process_gr.argtypes=[P,ctypes.POINTER(D),ctypes.POINTER(D),ctypes.c_int]
L.fc_destroy.argtypes=[P]
FS=176400.0
class FC:
    def __init__(s,spec=None,ch=1,fs=FS):
        s.p=L.fc_create(fs,ch); s.fs=fs; s.ch=ch
        if spec:
            for k,v in spec.items(): L.fc_spec(s.p,k.encode(),v)
            L.fc_prepare(s.p,fs,ch)
    def set(s,inp=0,out=0,att=3,rel=7,ratio=0,comp=1,link=1): L.fc_set(s.p,inp,out,att,rel,ratio,comp,link)
    def run(s,x):
        x=np.ascontiguousarray(x,dtype=np.float64).copy(); L.fc_process(s.p,x.ctypes.data_as(ctypes.POINTER(D)),1,len(x)); return x
    def run_gr(s,x):
        x=np.ascontiguousarray(x,dtype=np.float64).copy(); g=np.zeros_like(x)
        L.fc_process_gr(s.p,x.ctypes.data_as(ctypes.POINTER(D)),g.ctypes.data_as(ctypes.POINTER(D)),len(x)); return x,g
    def __del__(s): L.fc_destroy(s.p)
def sine(f,dbfs,dur,fs=FS): t=np.arange(int(dur*fs))/fs; return 10**(dbfs/20)*np.sin(2*np.pi*f*t)
def rms_db(x): return 20*np.log10(np.sqrt(np.mean(x**2))+1e-20)
def static_curve(spec=None,ratio=0,levels=range(-40,1,4),att=3,rel=7,inp=0):
    out=[]
    for l in levels:
        c=FC(spec); c.set(inp=inp,att=att,rel=rel,ratio=ratio)
        y,g=c.run_gr(sine(1000,l,0.6))
        n=int(0.2*FS); out.append((l, rms_db(y[-n:])-rms_db(sine(1000,l,0.2)), g[-n:].mean()))
    return out
