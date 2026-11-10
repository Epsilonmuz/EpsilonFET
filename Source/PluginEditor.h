#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "../Shared/epsilon_ui/EpsilonHeader.h"
#include "../Shared/epsilon_ui/EpsilonDialFont.h"
#include "../Shared/epsilon_ui/EpsilonSignature.h"

namespace epsui
{
namespace col
{
    // graphite body, champagne-gold metal, cream meter; amber is reserved for live things
    const juce::Colour body0 { 0xff26282d }, body1 { 0xff17181b }, well { 0xff111214 }, line { 0xff3a3d43 };
    const juce::Colour text { 0xfff1ebdf }, quiet { 0xffb4ada0 }, faint { 0xff7c776e };
    const juce::Colour champ { 0xffe3c99a }, champDeep { 0xff9c7c4c }, cream { 0xfff6e7c4 };
    const juce::Colour amber { 0xffffa63a }, amberDeep { 0xffe07b1a }, red { 0xffe8553f }, steel { 0xffd2d5da };
    const juce::Colour glass { 0xff0b0e13 };
}
juce::Font font (float size, bool bold = false, float tracking = 0.0f);

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();
    float scale = 1.0f;
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return font (14.0f * scale, true); }
    juce::Font getPopupMenuFont() override { return font (14.0f * scale); }
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
};
} // namespace epsui

// base layout (window units, 1080 x 520): shared by the controls and the X-ray circuit so the
// circuit parts sit right behind the controls that drive them
namespace layout
{
    constexpr float W = 1080, H = 500;
    const juce::Rectangle<float> vu { 40, 78, 360, 296 }, input { 418, 74, 224, 300 }, output { 638, 74, 224, 300 };
    const juce::Rectangle<float> mix { 884, 74, 92, 128 }, attack { 884, 262, 92, 136 }, release { 974, 262, 92, 136 };
    const juce::Rectangle<float> ratioRow { 436, 398, 408, 38 };
    constexpr float yMain = 226, ySide = 444, yBus = 474;
}

class FetKnob : public juce::Component
{
public:
    enum class Scale { decibels, sevens, percent };
    FetKnob (juce::AudioProcessorValueTreeState&, const juce::String& id, const juce::String& name, Scale, bool large, const juce::String& tip);
    void paint (juce::Graphics&) override;
    void resized() override;
    juce::Slider slider;
    float s = 1.0f;
private:
    juce::String name; Scale scale; bool large;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

class VuMeter : public juce::Component
{
public:
    explicit VuMeter (EpsilonFetProcessor& p) : proc (p) {}
    void tick();
    void paint (juce::Graphics&) override;
    void resized() override { face = {}; }
    int mode = 0;               // 0 GR, 1 +4, 2 +8, 3 off
    float s = 1.0f;
private:
    EpsilonFetProcessor& proc;
    float needle = 0.0f, vel = 0.0f;
    juce::Image face;
    void renderFace();
    float angleFor (float vu) const;
    juce::Rectangle<float> window() const;
    juce::Point<float> pivot() const;
    float radius() const;
};

// X-ray: the simulated circuit behind a smoked-glass faceplate, every part placed behind the control
// that drives it. Software-rendered into a bitmap per frame, then blitted (light on an Intel Mac).
class XrayView : public juce::Component
{
public:
    explicit XrayView (EpsilonFetProcessor& p) : proc (p) {}
    void tick();
    void paint (juce::Graphics&) override;
    void resized() override { staticLayer = {}; frameImg = {}; }
    float s = 1.0f;
    const juce::Image* ghostPlate = nullptr;     // the faceplate, drawn faintly on top as glass
private:
    EpsilonFetProcessor& proc;
    juce::Image staticLayer, frameImg;
    float level = 0, gr = 0, gate = 0, ohms = 0, cap = 0, flux = 0, phaseMain = 0, phaseSide = 0;
    int ratioIdx = 0;
    void renderStatic();
    void drawFrame (juce::Graphics&);
    juce::Point<float> P (float x, float y) const { return { x * s, y * s }; }
};

class EpsilonFetEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit EpsilonFetEditor (EpsilonFetProcessor&);
    ~EpsilonFetEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    juce::Component* findPart (const juce::String& id);     // manual pictures and tests
    void tickForSnapshot (int frames) { for (int i = 0; i < frames; ++i) timerCallback(); }
    void setXray (bool on);
    static constexpr float kW = layout::W, kH = layout::H;

private:
    void timerCallback() override;
    juce::Rectangle<int> sc (float x, float y, float w, float h) const;
    juce::Rectangle<int> sc (juce::Rectangle<float> r) const { return sc (r.getX(), r.getY(), r.getWidth(), r.getHeight()); }
    void setMeterMode (int m);
    void renderPlate();

    EpsilonFetProcessor& proc;
    epsui::LookAndFeel lnf;
    juce::TooltipWindow tips { this, 700 };
    float s = 1.0f;
    int frame = 0;
    bool xray = false;
    juce::Image plate;

    XrayView circuit;
    VuMeter vu;
    FetKnob input, output, attack, release, mix;
    juce::TextButton ratio[5], meterBtn[4], linkBtn { "LINK" }, qualityBtn, bypassBtn { "POWER" }, xrayBtn { "X-RAY" };
    juce::ComboBox presetBox;
    std::unique_ptr<juce::ParameterAttachment> ratioAtt, qualityAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> linkAtt, bypassAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EpsilonFetEditor)
};
