// C wrapper so Python can drive the circuit engine at its internal rate (tests and calibration).
#include "../Source/FetCircuit.h"
#include <cstring>
extern "C" {
void* fc_create (double fs, int ch) { auto* c = new epsilon::FetCircuit(); c->prepare (fs, ch); return c; }
void fc_destroy (void* p) { delete static_cast<epsilon::FetCircuit*> (p); }
void fc_set (void* p, double in, double out, double att, double rel, int ratio, int compress, int link)
{
    auto* c = static_cast<epsilon::FetCircuit*> (p);
    epsilon::FetControls k; k.inputDb = in; k.outputDb = out; k.attack = att; k.release = rel; k.ratio = ratio; k.compress = compress != 0;
    c->setControls (k); c->setLink (link != 0);
}
// set a spec field by name (calibration sweeps); call before fc_prepare
void fc_spec (void* p, const char* name, double v)
{
    auto& s = static_cast<epsilon::FetCircuit*> (p)->spec;
    if (! std::strcmp (name, "fetDrive")) s.fetDrive = v;
    else if (! std::strcmp (name, "attackMinR")) s.attackMinR = v;
    else if (! std::strcmp (name, "attackMaxR")) s.attackMaxR = v;
    else if (! std::strcmp (name, "gateFeedback")) s.gateFeedback = v;
    else if (! std::strncmp (name, "gateGain", 8)) s.gateGain[name[8] - '0'] = v;
    else if (! std::strncmp (name, "scDrive", 7)) s.scDrive[name[7] - '0'] = v;
    else if (! std::strncmp (name, "scOffset", 8)) s.scOffset[name[8] - '0'] = v;
}
void fc_prepare (void* p, double fs, int ch) { static_cast<epsilon::FetCircuit*> (p)->prepare (fs, ch); }
// planar double, in place
void fc_process (void* p, double* x, int ch, int n)
{
    double* ptr[2] = { x, ch > 1 ? x + n : x };
    static_cast<epsilon::FetCircuit*> (p)->process (ptr, n);
}
// per-sample gain reduction trace of channel 0 (dB)
void fc_process_gr (void* p, double* x, double* gr, int n)
{
    auto* c = static_cast<epsilon::FetCircuit*> (p);
    for (int i = 0; i < n; ++i) { double* ptr[1] = { x + i }; c->process (ptr, 1); gr[i] = c->channel (0).gainReductionDb(); }
}
double fc_gr (void* p) { return static_cast<epsilon::FetCircuit*> (p)->channel (0).gainReductionDb(); }
}
