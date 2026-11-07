// Epsilon FET - component-level model of a 1176 Rev D family FET compressor.
//
// Every stage is solved as a small electrical circuit in volts, amps and ohms, at the
// oversampled rate, with implicit (trapezoidal) integration and bounded Newton iterations:
//
//   input transformer -> Input pot -> R5 + JFET shunt (gate feedback) -> preamp
//        |                                                    |
//        |                                  sidechain: ratio network -> diodes -> C22
//        |                                    (attack R into C22, release R out of it)
//        |                                                    |  gate voltage
//   output transformer <- class-A output stage <- Output pot <-
//
// Target circuit: Hairball FET/RACK D v1.11 (a documented Rev D derivative). Nominal parts
// from the Epsilon Analog Research dossier (05 Nov 2026): R5 27 k, Q1 2N5457, C22 0.22 uF,
// attack 25 k pot, release 5 M pot, FDH333 detector diodes, Q6 2N3053 output stage.
// Values marked FITTED were set so the model meets the published 1176 behaviour
// (attack 20-800 us, release 50 ms-1.1 s, ratios 4/8/12/20); they are not measurements
// of a physical unit. Header-only, no JUCE, no allocation after prepare().
#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>

namespace epsilon
{

// ------------------------------------------------------------------ the circuit, as specified
struct CircuitSpec
{
    // level convention (dossier section 09): a 1 kHz sine at -18 dBFS peak = +4 dBu RMS
    double voltsPerUnit = 13.798018;            // peak volts per normalised sample

    // input transformer (600 ohm line in). Magnetics are unmeasured: nominal fields, kept explicit.
    double inRs = 650.0, inRload = 10000.0;     // source + winding resistance, reflected load
    double inL0 = 12.0;                         // magnetising inductance at low flux (H)
    double inFluxKnee = 0.055;                  // flux (V*s) where the inductance has halved
    double inHfHz = 60000.0;                    // leakage-L / winding-C roll-off

    // FET attenuator (R5 / Q1)
    double R5 = 27000.0;                        // series resistance into the shunt FET
    double preRin = 100000.0;                   // preamp input impedance loading the node
    double idss = 3.0e-3, vp = -1.6;            // 2N5457 class: Idss 1-5 mA, Vp -0.5..-6 V (spread)
    double lambda = 0.02;                       // channel-length modulation (1/V)
    double gateFeedback = 0.485;                // drain-to-gate linearising feedback (ideal 0.5)
    double softV = 0.025;                       // smoothing of the pinch-off corner (V)
    double fetDrive = 0.004;                    // FITTED: circuit volts reaching the FET per input volt
    double restGrDb = 1.0;                      // UA Q-bias procedure: 1 dB reduction at rest

    // detector (ratio network, FDH333 diodes, C22, attack/release pots)
    double C22 = 0.22e-6;
    double diodeIs = 2.0e-9, diodeN = 1.9, vt = 0.025852;
    double attackMinR = 90.0, attackMaxR = 11000.0;     // 20 us RC .. FITTED so knob 1 ~ 0.8 ms (within the 25 k R55)
    double releaseMinR = 227000.0, releaseMaxR = 5.0e6; // 50 ms .. 1.1 s measured; 5 M = the R56 pot itself
    // FITTED (tests/fit_ratios.py, on the real output level): knee at -26/-24/-22/-20 dBFS
    // (Input 0), slopes 4/8/12/20:1. Higher ratios also have a higher threshold, as on the hardware.
    double scDrive[5] = { 12.542, 9.956, 11.039, 11.324, 22.0 };   // ratio 4, 8, 12, 20, all buttons
    double scOffset[5] = { 0.338, 0.433, 0.749, 1.097, 0.45 };      // threshold networks (V)
    double gateGain[5] = { 0.197, 0.855, 0.884, 1.241, 1.0 }; // C22 voltage to gate, per ratio network
    double gateClamp = 0.35;                    // gate junction starts conducting (V)

    // amplifiers: preamp and Q6 class-A output stage (Ebers-Moll with emitter degeneration)
    double preHeadroom = 40.0, outHeadroom = 24.0;   // volts of swing before the stage limits
    double outAsym = 0.035;                          // class-A asymmetry (2nd harmonic)

    // output transformer
    double outRs = 60.0, outRload = 600.0, outL0 = 5.0, outFluxKnee = 0.040, outHfHz = 45000.0;
    double couplingHz = 4.0;                    // coupling capacitors
};

// ------------------------------------------------------------------ controls (hardware ranges)
struct FetControls
{
    double inputDb = 0.0;        // Input pot, -24..+24 dB (0 = calibrated unity)
    double outputDb = 0.0;       // Output pot, -24..+24 dB
    double attack = 3.0;         // 1..7, 7 = fastest (as on the hardware)
    double release = 7.0;        // 1..7, 7 = fastest
    int ratio = 0;               // 0: 4, 1: 8, 2: 12, 3: 20, 4: all buttons
    bool compress = true;        // attack fully off = no compression, amplifiers only
};

// what the editor shows: live circuit readouts (written by the audio thread)
struct CircuitReadout
{
    std::atomic<float> grDb { 0 }, gateV { 0 }, fetOhms { 0 }, capV { 0 }, inFluxPct { 0 }, outV { 0 };
};

// ------------------------------------------------------------------ helpers
inline double softplus (double x, double s) { return x > 30.0 * s ? x : s * std::log1p (std::exp (x / s)); }
inline double sigmoid (double x, double s) { return x > 30.0 * s ? 1.0 : 1.0 / (1.0 + std::exp (-x / s)); }

// Transformer: series R into a node loaded by a flux-dependent magnetising inductance and the
// reflected load. Flux is the integral of the core voltage, so saturation depends on frequency
// and level together (bass saturates first), as in a real core.
struct Transformer
{
    double Rs = 600, RL = 10000, L0 = 10, knee = 0.05;
    double T = 1.0 / 192000.0, flux = 0, vPrev = 0;
    double hfA = 0, hfZ = 0;

    void prepare (double fs, double rs, double rl, double l0, double kn, double hfHz)
    {
        T = 1.0 / fs; Rs = rs; RL = rl; L0 = l0; knee = kn;
        hfA = 1.0 - std::exp (-2.0 * M_PI * hfHz / fs);
        reset();
    }
    void reset() { flux = vPrev = hfZ = 0; }
    // magnetising current for a flux; inductance falls as the core saturates
    double im (double f) const { const double r = f / knee; return f / L0 * (1.0 + r * r); }
    double dim (double f) const { const double r = f / knee; return (1.0 + 3.0 * r * r) / L0; }

    double process (double vs)
    {
        double v = vPrev;
        for (int it = 0; it < 3; ++it)
        {
            const double f = flux + 0.5 * T * (v + vPrev);
            const double F = (vs - v) / Rs - im (f) - v / RL;
            const double dF = -1.0 / Rs - dim (f) * 0.5 * T - 1.0 / RL;
            const double step = F / dF;
            v -= step;
            if (std::abs (step) < 1e-9) break;
        }
        flux += 0.5 * T * (v + vPrev);
        flux *= 0.999999;                       // core remanence decays (keeps flux bounded on DC)
        vPrev = v;
        const double unloaded = v * (Rs + RL) / RL;       // undo the static divider: unity mid-band
        hfZ += hfA * (unloaded - hfZ);
        return hfZ;
    }
};

// Common-emitter class-A stage with emitter degeneration: the collector current follows
// Ebers-Moll, so the stage compresses one polarity before the other (even harmonics), then
// meets its supply rail. Normalised so the small-signal gain is exactly 1.
struct ClassAStage
{
    double headroom = 17.0, asym = 0.2, state = 0;
    double process (double x)
    {
        // y = H * tanh-like curve with asymmetric bias: solve y = g(x) in closed form
        const double h = headroom;
        const double u = x / h;
        const double a = asym;
        // exponential-junction asymmetry, then rail limiting
        const double e = (std::exp (a * u) - 1.0) / a;                // = u + a u^2/2 + ...
        const double e2 = e * e;
        return h * e / std::sqrt (std::sqrt (1.0 + e2 * e2));         // supply rails (firm knee)
    }
};

// ------------------------------------------------------------------ one channel of the circuit
class FetChannel
{
public:
    // static control mapping, recomputed when a knob moves (never per sample)
    struct ChannelSetup
    {
        double inGain = 1, outGain = 1, Ra = 1000, Rr = 1e6, relA = 0, scDrive = 1, scOffset = 0, biasShift = 0, gateGain = 1;
    };
    void prepare (double sampleRate, const CircuitSpec& s)
    {
        spec = &s; fs = sampleRate; T = 1.0 / fs;
        tIn.prepare (fs, s.inRs, s.inRload, s.inL0, s.inFluxKnee, s.inHfHz);
        tOut.prepare (fs, s.outRs, s.outRload, s.outL0, s.outFluxKnee, s.outHfHz);
        beta = s.idss / (s.vp * s.vp);
        pre.headroom = s.preHeadroom; pre.asym = 0.01;
        outStage.headroom = s.outHeadroom; outStage.asym = s.outAsym;
        hpA = std::exp (-2.0 * M_PI * s.couplingHz / fs);
        swA = 1.0 - std::exp (-T / 0.003);
        calibrateBias();
        reset();
    }

    void reset()
    {
        tIn.reset(); tOut.reset();
        vd = 0; vCap = 0; hpX = hpY = 0; hpX2 = hpY2 = 0; ggSm = -1.0; bsSm = 0.0;
    }

    // per-sample, at the internal rate. x: normalised sample. Returns normalised sample.
    // capShared: when the channels are linked, the caller averages vCap between samples.
    double process (double x, const FetControls& c, const ChannelSetup& k)
    {
        const auto& s = *spec;
        // 1. line input into the input transformer (volts)
        const double vin = tIn.process (x * s.voltsPerUnit);
        // 2. Input pot, then R5 into the FET node
        const double va = vin * k.inGain * s.fetDrive;
        const double vg = gateVoltage (c, k);
        vd = solveFetNode (va, vg);
        // 3. preamp makes up the R5/FET loss at rest (fixed gain, set at calibration)
        const double vpre = pre.process (vd * preGain);
        // 4. sidechain from the preamp output, before the Output pot (feedback topology)
        if (c.compress) updateDetector (vpre, k); else vCap += (0.0 - vCap) * k.relA;
        // 5. Output pot, class-A output stage, output transformer, coupling
        double vo = outStage.process (vpre * k.outGain);
        vo = tOut.process (vo);
        vo = hp (vo);
        lastGate = vg;
        return vo / s.voltsPerUnit;
    }

    ChannelSetup setup (const FetControls& c) const
    {
        const auto& s = *spec;
        ChannelSetup k;
        k.inGain = std::pow (10.0, c.inputDb / 20.0);
        k.outGain = std::pow (10.0, c.outputDb / 20.0);
        // pots are log taper: knob 7 = minimum resistance (fastest)
        const double ta = std::clamp ((7.0 - c.attack) / 6.0, 0.0, 1.0);
        const double tr = std::clamp ((7.0 - c.release) / 6.0, 0.0, 1.0);
        k.Ra = s.attackMinR * std::pow (s.attackMaxR / s.attackMinR, ta);
        k.Rr = s.releaseMinR * std::pow (s.releaseMaxR / s.releaseMinR, tr);
        k.relA = 1.0 - std::exp (-T / (k.Rr * s.C22));
        const int r = std::clamp (c.ratio, 0, 4);
        k.scDrive = s.scDrive[r];
        k.scOffset = s.scOffset[r];
        k.gateGain = s.gateGain[r];
        k.biasShift = r == 4 ? 0.012 : 0.0;      // all buttons: the ratio network pulls the bias
        if (r == 4) k.Ra *= 0.6;                 //   and the attack speeds up
        return k;
    }

    double gainReductionDb() const
    {
        // node gain now vs node gain at rest (FET resistance from the last solve)
        const double now = std::abs (lastGate - spec->vp) > 0 ? smallSignalGain (lastGate) : 1.0;
        return 20.0 * std::log10 (std::max (now, 1e-9) / restGain);
    }
    double fetResistance() const { return smallSignalRds (lastGate); }
    double gate() const { return lastGate; }
    double capVoltage() const { return vCap; }
    double inputFluxPct() const { return 100.0 * std::abs (tIn.flux) / spec->inFluxKnee; }
    double& cap() { return vCap; }

private:
    const CircuitSpec* spec = nullptr;
    double fs = 192000, T = 1.0 / 192000.0, beta = 1e-3;
    Transformer tIn, tOut;
    ClassAStage pre, outStage;
    double vd = 0, vCap = 0, lastGate = -2.0;
    double vBias = -2.0, preGain = 1.0, restGain = 1.0;
    double ggSm = 1.0, bsSm = 0.0, swA = 0.001;
    double hpA = 0.9999, hpX = 0, hpY = 0, hpX2 = 0, hpY2 = 0;

    // JFET drain current, symmetric in drain and source (works for both signal polarities):
    // Id = beta * [ f(Vgs - Vp) - f(Vgd - Vp) ] * (1 + lambda |Vds|),  f(x) = softplus(x)^2.
    // In the triode region this is beta * [2 (Vgs - Vp) Vds - Vds^2], the dossier's relation;
    // with half the drain voltage fed back to the gate the Vds^2 term cancels (the 1176 trick),
    // so only the mismatch (gateFeedback != 0.5) and lambda leave a little distortion.
    double drainCurrent (double v, double vgCtl, double& dI) const
    {
        const auto& s = *spec;
        const double vg = vgCtl + s.gateFeedback * v;
        const double a = vg - s.vp, b = vg - v - s.vp;
        const double sa = softplus (a, s.softV), sb = softplus (b, s.softV);
        const double da = sigmoid (a, s.softV), db = sigmoid (b, s.softV);
        const double core = sa * sa - sb * sb;
        const double dcore = 2.0 * sa * da * s.gateFeedback - 2.0 * sb * db * (s.gateFeedback - 1.0);
        const double lm = 1.0 + s.lambda * std::abs (v);
        dI = beta * (dcore * lm + core * s.lambda * (v >= 0 ? 1.0 : -1.0));
        return beta * core * lm;
    }

    // node: (va - v)/R5 = Id(v) + v/Rin. Newton from the previous sample's solution.
    double solveFetNode (double va, double vgCtl)
    {
        const auto& s = *spec;
        double v = vd;
        for (int it = 0; it < 4; ++it)
        {
            double dI;
            const double I = drainCurrent (v, vgCtl, dI);
            const double F = (va - v) / s.R5 - I - v / s.preRin;
            const double dF = -1.0 / s.R5 - dI - 1.0 / s.preRin;
            double step = F / dF;
            step = std::clamp (step, -0.5, 0.5);             // damped: bounded work, no blow-ups
            v -= step;
            if (std::abs (step) < 1e-10) break;
        }
        return v;
    }

    double smallSignalRds (double vgCtl) const
    {
        double dI;
        drainCurrent (0.0, vgCtl, dI);
        return 1.0 / std::max (dI, 1e-12);
    }
    double smallSignalGain (double vgCtl) const
    {
        const auto& s = *spec;
        const double rp = 1.0 / (1.0 / smallSignalRds (vgCtl) + 1.0 / s.preRin);
        return rp / (s.R5 + rp);
    }

    // Q-bias trim, as in the UA procedure: set the gate so the FET takes exactly restGrDb at rest.
    void calibrateBias()
    {
        const auto& s = *spec;
        const double open = s.preRin / (s.R5 + s.preRin);
        const double target = open * std::pow (10.0, -s.restGrDb / 20.0);
        double lo = s.vp - 1.0, hi = 0.0;                    // gain falls as the gate rises
        for (int i = 0; i < 80; ++i)
        {
            const double mid = 0.5 * (lo + hi);
            if (smallSignalGain (mid) > target) lo = mid; else hi = mid;
        }
        vBias = 0.5 * (lo + hi);
        restGain = smallSignalGain (vBias);
        preGain = 1.0 / (restGain * s.fetDrive);              // unity through the chain at rest
        lastGate = vBias;
    }

    double gateVoltage (const FetControls&, const ChannelSetup& k)
    {
        const auto& s = *spec;
        // the ratio switch changes the gate network: glide over ~3 ms instead of jumping (no click)
        if (ggSm < 0.0) ggSm = k.gateGain;
        ggSm += swA * (k.gateGain - ggSm);
        bsSm += swA * (k.biasShift - bsSm);
        const double g = vBias + bsSm + ggSm * vCap;
        // gate junction: past ~gateClamp it conducts and stops the gate rising further
        return g - softplus (g - s.gateClamp, 0.03);
    }

    // Detector: the ratio network scales the preamp output into the sidechain amplifier;
    // full-wave diode rectifier charges C22 through the attack resistance; the release
    // resistance discharges it. Trapezoidal integration of C22, Newton on the diode current.
    void updateDetector (double vpre, const ChannelSetup& k)
    {
        const auto& s = *spec;
        const double drive = std::max (0.0, std::abs (vpre) * k.scDrive * 0.1 - k.scOffset);
        const double nvt = s.diodeN * s.vt;
        auto charge = [&] (double vc, double& dI)
        {
            // diode in series with Ra: solve I from  drive - vc = I*Ra + nVt*ln(1 + I/Is)
            const double over = drive - vc;
            if (over <= 0) { dI = 0; return 0.0; }
            double I = over / k.Ra;
            for (int it = 0; it < 3; ++it)
            {
                const double vdio = nvt * std::log1p (I / s.diodeIs);
                const double G = I * k.Ra + vdio - over;
                const double dG = k.Ra + nvt / (s.diodeIs + I);
                I = std::max (0.0, I - G / dG);
            }
            dI = -1.0 / (k.Ra + nvt / (s.diodeIs + I));
            return I;
        };
        // C dv/dt = I_charge(v) - v/Rr   (trapezoid)
        double dI0;
        const double f0 = charge (vCap, dI0) - vCap / k.Rr;
        double v = vCap;
        for (int it = 0; it < 3; ++it)
        {
            double dI;
            const double f1 = charge (v, dI) - v / k.Rr;
            const double G = v - vCap - 0.5 * T / s.C22 * (f0 + f1);
            const double dG = 1.0 - 0.5 * T / s.C22 * (dI - 1.0 / k.Rr);
            const double step = G / dG;
            v -= step;
            if (std::abs (step) < 1e-10) break;
        }
        vCap = std::max (0.0, v);
    }

    double hp (double x)
    {   // two coupling capacitors in the chain (first order each)
        const double y = hpA * (hpY + x - hpX); hpX = x; hpY = y;
        const double y2 = hpA * (hpY2 + y - hpX2); hpX2 = y; hpY2 = y2;
        return y2;
    }

    friend class FetCircuit;
};

// ------------------------------------------------------------------ stereo unit
class FetCircuit
{
public:
    static constexpr int kMaxCh = 2;
    CircuitSpec spec;
    CircuitReadout readout;

    void prepare (double internalRate, int channels)
    {
        nch = std::clamp (channels, 1, kMaxCh);
        for (auto& c : ch) c.prepare (internalRate, spec);
        setControls (ctl, true);
    }
    void reset() { for (auto& c : ch) c.reset(); }

    void setControls (const FetControls& c, bool force = false)
    {
        if (! force && same (c, ctl)) return;
        ctl = c;
        k = ch[0].setup (ctl);
    }
    void setLink (bool l) { link = l; }

    // in place, planar, at the internal rate
    void process (double* const* x, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            for (int c = 0; c < nch; ++c) x[c][i] = ch[(size_t) c].process (x[c][i], ctl, k);
            if (link && nch == 2)
            {   // stereo link: the two C22 capacitors tied together share one voltage
                const double m = std::max (ch[0].vCap, ch[1].vCap) * 0.75 + 0.25 * 0.5 * (ch[0].vCap + ch[1].vCap);
                ch[0].vCap = ch[1].vCap = m;
            }
        }
        // readouts for the editor (channel 0, or the more compressed channel)
        const auto& a = (nch == 2 && ch[1].vCap > ch[0].vCap) ? ch[1] : ch[0];
        readout.grDb.store ((float) a.gainReductionDb(), std::memory_order_relaxed);
        readout.gateV.store ((float) a.gate(), std::memory_order_relaxed);
        readout.fetOhms.store ((float) a.fetResistance(), std::memory_order_relaxed);
        readout.capV.store ((float) a.capVoltage(), std::memory_order_relaxed);
        readout.inFluxPct.store ((float) a.inputFluxPct(), std::memory_order_relaxed);
    }

    FetChannel& channel (int c) { return ch[(size_t) c]; }

private:
    std::array<FetChannel, kMaxCh> ch;
    FetControls ctl;
    FetChannel::ChannelSetup k;
    int nch = 2;
    bool link = true;
    static bool same (const FetControls& a, const FetControls& b)
    {
        return a.inputDb == b.inputDb && a.outputDb == b.outputDb && a.attack == b.attack && a.release == b.release
            && a.ratio == b.ratio && a.compress == b.compress;
    }
};

} // namespace epsilon
