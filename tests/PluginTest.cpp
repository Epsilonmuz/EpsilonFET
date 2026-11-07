// Plugin-level checks without a host:  EpsilonPluginTest
// latency + dry/wet alignment, block-size invariance, unity at rest, aliasing,
// automation without clicks, presets/state, CPU per block.
#include "PluginProcessor.h"
#include <chrono>

static int failures = 0;
#define CHECK(c, ...) do { if (! (c)) { std::printf ("FAIL: " __VA_ARGS__); std::printf ("\n"); ++failures; } } while (0)

static void setP (EpsilonFetProcessor& p, const char* id, float v) { auto* r = p.apvts.getParameter (id); r->setValueNotifyingHost (r->convertTo0to1 (v)); }

static std::vector<float> render (const std::vector<float>& x, int block, double fs, std::function<void (EpsilonFetProcessor&)> setup,
                                  std::function<void (EpsilonFetProcessor&, int)> perBlock = {})
{
    EpsilonFetProcessor p;
    p.setPlayConfigDetails (2, 2, fs, block);
    setup (p);
    p.prepareToPlay (fs, block);
    std::vector<float> y (x.size());
    juce::AudioBuffer<float> b (2, block);
    juce::MidiBuffer m;
    for (size_t s = 0; s < x.size(); s += (size_t) block)
    {
        const int n = (int) std::min<size_t> ((size_t) block, x.size() - s);
        if (perBlock) perBlock (p, (int) s);
        for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) b.setSample (c, i, x[s + (size_t) i]);
        juce::AudioBuffer<float> v (b.getArrayOfWritePointers(), 2, n);
        p.processBlock (v, m);
        for (int i = 0; i < n; ++i) y[s + (size_t) i] = v.getSample (0, i);
    }
    return y;
}

static std::vector<float> sine (double f, double dbfs, double sec, double fs)
{
    std::vector<float> x ((size_t) (sec * fs));
    const double a = std::pow (10.0, dbfs / 20.0);
    for (size_t i = 0; i < x.size(); ++i) x[i] = (float) (a * std::sin (2.0 * M_PI * f * (double) i / fs));
    return x;
}
static double rmsDb (const std::vector<float>& x, size_t from)
{
    double s = 0; for (size_t i = from; i < x.size(); ++i) s += (double) x[i] * x[i];
    return 10.0 * std::log10 (s / (double) (x.size() - from) + 1e-30);
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    const double fs = 48000.0;

    // 1. unity at rest: Init preset, quiet signal, no compression
    {
        auto x = sine (1000, -40, 1.0, fs);
        auto y = render (x, 256, fs, [] (EpsilonFetProcessor&) {});
        const double g = rmsDb (y, 24000) - rmsDb (x, 24000);
        std::printf ("rest gain at -40 dBFS: %+.3f dB\n", g);
        CHECK (std::abs (g) < 0.3, "rest gain %.3f", g);
    }
    // 2. latency reported = actual, and 50 % mix does not comb-filter (dry aligned with wet)
    {
        EpsilonFetProcessor p; p.setPlayConfigDetails (2, 2, fs, 256); p.prepareToPlay (fs, 256);
        const int lat = p.getLatencySamples();
        std::vector<float> imp (4096, 0.0f); imp[100] = 0.25f;
        auto y = render (imp, 256, fs, [] (EpsilonFetProcessor& q) { setP (q, "input", -24.0f); });
        int pk = 0; for (int i = 0; i < (int) y.size(); ++i) if (std::abs (y[(size_t) i]) > std::abs (y[(size_t) pk])) pk = i;
        std::printf ("reported latency %d samples, measured impulse peak at +%d\n", lat, pk - 100);
        CHECK (std::abs ((pk - 100) - lat) <= 2, "latency mismatch");
        // white-ish noise through 50 % mix at rest: spectrum must stay flat (no comb notches)
        juce::Random r (7); std::vector<float> nz ((size_t) fs);
        for (auto& v : nz) v = 0.005f * (r.nextFloat() * 2.0f - 1.0f);           // quiet: no compression, unity wet
        auto wet = render (nz, 256, fs, [] (EpsilonFetProcessor& q) { setP (q, "mix", 50.0f); });
        // compare 1/3-octave-ish band energies between in and out at a few frequencies via Goertzel on chunks
        auto band = [&] (const std::vector<float>& v, double f)
        {
            double sum = 0; const int N = 2048;
            for (size_t st = 4096; st + N < v.size(); st += N)
            {
                double s1 = 0, s2 = 0; const double k = 2 * std::cos (2 * M_PI * f / fs);
                for (int i = 0; i < N; ++i) { const double s0 = v[st + (size_t) i] + k * s1 - s2; s2 = s1; s1 = s0; }
                sum += s1 * s1 + s2 * s2 - k * s1 * s2;
            }
            return 10 * std::log10 (sum + 1e-30);
        };
        double worst = 0;
        for (double f : { 1000.0, 3000.0, 6000.0, 9000.0, 12000.0, 16000.0 }) worst = std::max (worst, std::abs (band (wet, f) - band (nz, f)));
        std::printf ("50%% mix at rest, worst band deviation: %.2f dB \n", worst);
        CHECK (worst < 0.5, "comb filtering in mix");
    }
    // 3. block-size invariance (same output for 32 / 256 / 1024 sample blocks)
    {
        auto x = sine (220, -10, 1.0, fs);
        auto set = [] (EpsilonFetProcessor& q) { setP (q, "input", 10.0f); setP (q, "attack", 7.0f); setP (q, "release", 7.0f); };
        auto a = render (x, 32, fs, set), b = render (x, 256, fs, set), c = render (x, 1024, fs, set);
        double d1 = 0, d2 = 0; for (size_t i = 0; i < x.size(); ++i) { d1 = std::max (d1, (double) std::abs (a[i] - b[i])); d2 = std::max (d2, (double) std::abs (c[i] - b[i])); }
        std::printf ("block invariance: max diff 32 vs 256: %.2e, 1024 vs 256: %.2e\n", d1, d2);
        CHECK (d1 < 1e-3 && d2 < 1e-3, "block size changes the output");
    }
    // 4. aliasing: 15 kHz sine, heavy compression, look for energy at non-harmonic frequencies below 15 kHz
    {
        auto x = sine (15000, -6, 1.0, fs);
        auto y = render (x, 256, fs, [] (EpsilonFetProcessor& q) { setP (q, "input", 12.0f); setP (q, "ratio", 4.0f); setP (q, "attack", 7.0f); setP (q, "release", 7.0f); });
        // aliases of 15k harmonics at 48k: 2*15k -> 18k, 3*15k -> 3k, 4*15k -> 12k ...
        auto level = [&] (double f)
        {
            double s1 = 0, s2 = 0; const int N = 24000; const double k = 2 * std::cos (2 * M_PI * f / fs);
            for (int i = 0; i < N; ++i) { const double s0 = y[(size_t) (24000 + i)] + k * s1 - s2; s2 = s1; s1 = s0; }
            return 20 * std::log10 (std::sqrt (std::max (0.0, s1 * s1 + s2 * s2 - k * s1 * s2)) * 2.0 / N + 1e-12);
        };
        const double fund = level (15000), a3 = level (3000), a12 = level (12000), a9 = level (9000);
        std::printf ("aliasing (15 kHz in, all buttons): fundamental %.1f dBFS, 3k %.1f, 9k %.1f, 12k %.1f dBFS\n", fund, a3, a9, a12);
        CHECK (std::max ({ a3, a9, a12 }) < -75.0, "aliasing above -75 dBFS");
    }
    // 5. fast automation of every knob: no clicks (largest sample-to-sample jump stays near the signal's own)
    {
        auto x = sine (100, -12, 2.0, fs);
        double maxStep = 0;
        auto y = render (x, 64, fs, [] (EpsilonFetProcessor&) {}, [&] (EpsilonFetProcessor& q, int s)
        {
            const float t = (float) s / (float) fs;
            setP (q, "input", 12.0f * std::sin (6.0f * t));
            setP (q, "output", 6.0f * std::sin (5.0f * t));
            setP (q, "attack", 4.0f + 3.0f * std::sin (7.0f * t));
            setP (q, "release", 4.0f + 3.0f * std::sin (3.0f * t));
            setP (q, "mix", 50.0f + 50.0f * std::sin (4.0f * t));
            if ((s / 64) % 400 == 0) setP (q, "ratio", (float) ((s / 64 / 400) % 5));
        });
        for (size_t i = 1; i < y.size(); ++i) maxStep = std::max (maxStep, (double) std::abs (y[i] - y[i - 1]));
        const double sigStep = 2 * M_PI * 100 / fs * std::pow (10.0, (-12 + 12 + 6) / 20.0);
        bool finite = true; for (auto v : y) finite &= std::isfinite (v);
        std::printf ("automation: largest step %.4f (pure signal bound %.4f), finite %d\n", maxStep, sigStep, (int) finite);
        CHECK (finite && maxStep < 0.12, "click during automation");
    }
    // 6. presets and state round trip
    {
        EpsilonFetProcessor p; p.loadPreset (4);
        juce::MemoryBlock st; p.getStateInformation (st);
        EpsilonFetProcessor q; q.setStateInformation (st.getData(), (int) st.getSize());
        CHECK (q.currentPreset == 4, "preset index lost");
        for (auto* prm : p.getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm))
                CHECK (std::abs (r->getValue() - q.apvts.getParameter (r->getParameterID())->getValue()) < 1e-6, "param %s", r->getParameterID().toRawUTF8());
    }
    // 7. CPU: one stereo instance, 64-sample blocks at 44.1 kHz, Studio 4x
    for (int quality : { 1, 0 })
    {
        const double rate = 44100.0; const int block = 64;
        EpsilonFetProcessor p; p.setPlayConfigDetails (2, 2, rate, block);
        setP (p, "quality", (float) quality); setP (p, "input", 10.0f);
        p.prepareToPlay (rate, block);
        juce::AudioBuffer<float> b (2, block); juce::MidiBuffer m; juce::Random r (3);
        std::vector<double> t;
        for (int k = 0; k < 4000; ++k)
        {
            for (int c = 0; c < 2; ++c) for (int i = 0; i < block; ++i) b.setSample (c, i, 0.3f * std::sin (0.05f * (float) (k * block + i)) + 0.01f * r.nextFloat());
            const auto t0 = std::chrono::steady_clock::now();
            p.processBlock (b, m);
            t.push_back (std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count());
        }
        std::sort (t.begin(), t.end());
        const double budget = block / rate;
        std::printf ("CPU (%s): median %.1f%%, p99.9 %.1f%% of the %.3f ms block budget (this machine)\n", quality ? "Studio 4x" : "Eco 2x",
                     100 * t[t.size() / 2] / budget, 100 * t[(size_t) (t.size() * 0.999)] / budget, budget * 1000);
    }
    std::printf (failures == 0 ? "PLUGIN TEST PASSED\n" : "PLUGIN TEST FAILED (%d)\n", failures);
    return failures == 0 ? 0 : 1;
}
