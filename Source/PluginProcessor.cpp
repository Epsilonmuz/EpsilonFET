#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

// ------------------------------------------------------------------ presets
struct Preset { const char* name; float input, output, attack, release; int ratio; float mix; };
static const Preset kPresets[] = {
    { "Init",                 0.0f,  0.0f, 3.0f, 7.0f, 0, 100.0f },
    { "Vocal - Classic",      8.0f, -3.0f, 3.0f, 7.0f, 0, 100.0f },
    { "Vocal - Upfront",     12.0f, -6.0f, 5.0f, 6.0f, 1, 100.0f },
    { "Bass - Even",          6.0f, -2.0f, 2.0f, 4.0f, 0, 100.0f },
    { "Drums - Parallel Smash",14.0f,-8.0f, 7.0f, 7.0f, 4,  45.0f },
    { "Room Mics - All In",  14.0f, -9.0f, 7.0f, 7.0f, 4, 100.0f },
    { "Snare - Snap",         6.0f, -2.0f, 1.0f, 7.0f, 1, 100.0f },
    { "Mix Bus - Gentle",     3.0f, -1.0f, 1.0f, 3.0f, 0,  60.0f },
    { "Peak Limiter",         6.0f, -2.0f, 7.0f, 5.0f, 3, 100.0f },
};
int EpsilonFetProcessor::getNumPrograms() { return (int) std::size (kPresets); }
const juce::String EpsilonFetProcessor::getProgramName (int i) { return kPresets[juce::jlimit (0, getNumPrograms() - 1, i)].name; }

void EpsilonFetProcessor::loadPreset (int index)
{
    currentPreset = juce::jlimit (0, getNumPrograms() - 1, index);
    const auto& p = kPresets[currentPreset];
    auto set = [this] (const char* id, float v) { if (auto* r = apvts.getParameter (id)) r->setValueNotifyingHost (r->convertTo0to1 (v)); };
    set ("input", p.input); set ("output", p.output); set ("attack", p.attack); set ("release", p.release);
    set ("ratio", (float) p.ratio); set ("mix", p.mix);
    apvts.state.setProperty ("preset", currentPreset, nullptr);
}

// ------------------------------------------------------------------ parameters
APVTS::ParameterLayout EpsilonFetProcessor::createLayout()
{
    using namespace juce;
    APVTS::ParameterLayout l;
    auto db = AudioParameterFloatAttributes().withLabel ("dB")
                .withStringFromValueFunction ([] (float v, int) { return (v > 0 ? "+" : "") + String (v, 1) + " dB"; });
    auto knob = AudioParameterFloatAttributes()
                .withStringFromValueFunction ([] (float v, int) { return String (v, 1); });
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "input", 1 }, "Input", NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f, db));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "output", 1 }, "Output", NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f, db));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "attack", 1 }, "Attack", NormalisableRange<float> (1.0f, 7.0f, 0.01f), 3.0f, knob));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "release", 1 }, "Release", NormalisableRange<float> (1.0f, 7.0f, 0.01f), 7.0f, knob));
    l.add (std::make_unique<AudioParameterChoice>(ParameterID { "ratio", 1 }, "Ratio", StringArray { "4", "8", "12", "20", "All" }, 0));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix", NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f,
               AudioParameterFloatAttributes().withLabel ("%").withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " %"; })));
    l.add (std::make_unique<AudioParameterBool>  (ParameterID { "link", 1 }, "Stereo Link", true));
    l.add (std::make_unique<AudioParameterChoice>(ParameterID { "quality", 1 }, "Quality", StringArray { "Eco 2x", "Studio 4x" }, 1));
    l.add (std::make_unique<AudioParameterBool>  (ParameterID { "bypass", 1 }, "Bypass", false));
    return l;
}

EpsilonFetProcessor::EpsilonFetProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "EpsilonFET", createLayout())
{
    pInput = apvts.getRawParameterValue ("input");   pOutput = apvts.getRawParameterValue ("output");
    pAttack = apvts.getRawParameterValue ("attack"); pRelease = apvts.getRawParameterValue ("release");
    pRatio = apvts.getRawParameterValue ("ratio");   pMix = apvts.getRawParameterValue ("mix");
    pLink = apvts.getRawParameterValue ("link");     pQuality = apvts.getRawParameterValue ("quality");
    pBypass = apvts.getRawParameterValue ("bypass");
    bypassParam = apvts.getParameter ("bypass");
}

bool EpsilonFetProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    return layouts.getMainInputChannelSet() == out;
}

void EpsilonFetProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    rate = sampleRate;
    maxBlock = juce::jmax (1, samplesPerBlock);
    const int ch = juce::jmax (1, getTotalNumOutputChannels());
    // polyphase IIR half-band filters: a few samples of latency, fine for tracking
    for (int i = 0; i < 2; ++i)
    {
        os[i] = std::make_unique<juce::dsp::Oversampling<double>> ((size_t) ch, (size_t) (i + 1),
                    juce::dsp::Oversampling<double>::filterHalfBandPolyphaseIIR, true, true);
        os[i]->initProcessing ((size_t) maxBlock);
        osDry[i] = std::make_unique<juce::dsp::Oversampling<double>> ((size_t) ch, (size_t) (i + 1),
                    juce::dsp::Oversampling<double>::filterHalfBandPolyphaseIIR, true, true);
        osDry[i]->initProcessing ((size_t) maxBlock);
    }
    work.setSize (ch, maxBlock, false, true, true);
    dry.setSize (ch, maxBlock, false, true, true);
    mixSm.reset (sampleRate, 0.03);
    mixSm.setCurrentAndTargetValue (pBypass->load() > 0.5f ? 0.0f : pMix->load() / 100.0f);
    inSm.reset (sampleRate, 0.05); inSm.setCurrentAndTargetValue (pInput->load());
    outSm.reset (sampleRate, 0.05); outSm.setCurrentAndTargetValue (pOutput->load());
    activeQuality = -1;
    setQuality ((int) pQuality->load());
}

void EpsilonFetProcessor::setQuality (int q)
{
    q = juce::jlimit (0, 1, q);
    if (q == activeQuality) return;
    activeQuality = q;
    os[q]->reset();
    osDry[q]->reset();
    circuit.prepare (rate * (q == 0 ? 2.0 : 4.0), juce::jmax (1, getTotalNumOutputChannels()));
    setLatencySamples (juce::roundToInt (os[q]->getLatencyInSamples()));
}

void EpsilonFetProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int ch = juce::jmin (buffer.getNumChannels(), work.getNumChannels());
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c) buffer.clear (c, 0, n);
    if (n == 0 || ch == 0) return;

    setQuality ((int) pQuality->load());
    const bool bypassed = pBypass->load() > 0.5f;

    // input level for the meter
    float inSum = 0.0f;
    for (int c = 0; c < ch; ++c) inSum += buffer.getRMSLevel (c, 0, n);
    inLevel.store (inSum / (float) ch);

    // float -> double for the circuit (and the matching dry path)
    for (int c = 0; c < ch; ++c)
    {
        auto* d = work.getWritePointer (c); auto* e = dry.getWritePointer (c); auto* s = buffer.getReadPointer (c);
        for (int i = 0; i < n; ++i) d[i] = e[i] = s[i];
    }
    {
        juce::dsp::AudioBlock<double> db (dry.getArrayOfWritePointers(), (size_t) ch, (size_t) n);
        osDry[activeQuality]->processSamplesUp (db);
        osDry[activeQuality]->processSamplesDown (db);
    }
    juce::dsp::AudioBlock<double> block (work.getArrayOfWritePointers(), (size_t) ch, (size_t) n);
    auto& o = *os[activeQuality];
    auto up = o.processSamplesUp (block);
    const int factor = (int) o.getOversamplingFactor();
    const int nu = (int) up.getNumSamples();

    circuit.setLink (pLink->load() > 0.5f);
    inSm.setTargetValue (pInput->load());
    outSm.setTargetValue (pOutput->load());
    epsilon::FetControls k;
    k.attack = pAttack->load(); k.release = pRelease->load();
    k.ratio = juce::roundToInt (pRatio->load());

    // knobs move in steps of 16 internal samples, smoothed: no zipper noise, no per-sample setup
    constexpr int kStep = 16;
    double* ptr[2];
    for (int s = 0; s < nu; s += kStep)
    {
        const int len = juce::jmin (kStep, nu - s);
        const int hostSteps = juce::jmax (1, len / factor);
        k.inputDb = inSm.skip (hostSteps); k.outputDb = outSm.skip (hostSteps);
        circuit.setControls (k);
        for (int c = 0; c < ch; ++c) ptr[c] = up.getChannelPointer ((size_t) c) + s;
        circuit.process (ptr, len);
    }
    o.processSamplesDown (block);

    // Mix (and bypass, as Mix 0 %): wet and dry carry the same oversampling phase
    mixSm.setTargetValue (bypassed ? 0.0f : pMix->load() / 100.0f);
    double sq[2] = { 0.0, 0.0 };
    for (int i = 0; i < n; ++i)
    {
        const double m = mixSm.getNextValue();
        for (int c = 0; c < ch; ++c)
        {
            const double v = m * work.getSample (c, i) + (1.0 - m) * dry.getSample (c, i);
            buffer.setSample (c, i, (float) v);
            sq[c] += v * v;
        }
    }
    float outSum = 0.0f;
    for (int c = 0; c < ch; ++c) outSum += (float) std::sqrt (sq[c] / n);
    outLevel.store (outSum / (float) ch);
}

juce::AudioProcessorEditor* EpsilonFetProcessor::createEditor() { return new EpsilonFetEditor (*this); }

void EpsilonFetProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    apvts.state.setProperty ("preset", currentPreset, nullptr);
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}

void EpsilonFetProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            currentPreset = apvts.state.getProperty ("preset", 0);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new EpsilonFetProcessor(); }
