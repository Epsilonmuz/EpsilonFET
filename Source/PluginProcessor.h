#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "FetCircuit.h"

class EpsilonFetProcessor : public juce::AudioProcessor
{
public:
    EpsilonFetProcessor();
    ~EpsilonFetProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;
    juce::AudioProcessorParameter* getBypassParameter() const override { return bypassParam; }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Epsilon FET"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override { loadPreset (index); }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    void loadPreset (int index);
    int currentPreset = 0;

    // for the editor
    const epsilon::CircuitReadout& readout() const { return circuit.readout; }
    std::atomic<float> outLevel { 0.0f }, inLevel { 0.0f };    // RMS of the block, linear
    double getRate() const { return rate; }

private:
    epsilon::FetCircuit circuit;
    std::unique_ptr<juce::dsp::Oversampling<double>> os[2];   // 2x, 4x: the circuit's path
    // The dry path goes through an identical up/down pair (no circuit), so dry and wet carry the
    // same filter phase: Mix blends without comb filtering and without adding latency.
    std::unique_ptr<juce::dsp::Oversampling<double>> osDry[2];
    juce::AudioBuffer<double> work, dry;
    juce::SmoothedValue<float> mixSm;
    double rate = 44100.0;
    int activeQuality = -1, maxBlock = 512;

    std::atomic<float>* pInput = nullptr; std::atomic<float>* pOutput = nullptr;
    std::atomic<float>* pAttack = nullptr; std::atomic<float>* pRelease = nullptr;
    std::atomic<float>* pRatio = nullptr; std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pLink = nullptr; std::atomic<float>* pQuality = nullptr;
    std::atomic<float>* pBypass = nullptr;
    juce::AudioProcessorParameter* bypassParam = nullptr;
    juce::SmoothedValue<double> inSm, outSm;

    void setQuality (int q);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EpsilonFetProcessor)
};
