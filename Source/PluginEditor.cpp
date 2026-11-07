#include "PluginEditor.h"

using namespace epsui;

juce::Font epsui::font (float size, bool bold, float tracking)
{
    juce::Font f { juce::FontOptions ("Inter", size, bold ? juce::Font::bold : juce::Font::plain) };
    if (tracking != 0.0f) f.setExtraKerningFactor (tracking);
    return f;
}
static juce::String utf8 (const char* s) { return juce::String::fromUTF8 (s); }

// ================================================================ look and feel
epsui::LookAndFeel::LookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, col::well);
    setColour (juce::ComboBox::textColourId, col::text);
    setColour (juce::ComboBox::outlineColourId, col::line);
    setColour (juce::ComboBox::arrowColourId, col::champ);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1d1e22));
    setColour (juce::PopupMenu::textColourId, col::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::champ.withAlpha (0.2f));
    setColour (juce::PopupMenu::highlightedTextColourId, col::text);
    setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xff1d1e22));
    setColour (juce::TooltipWindow::textColourId, col::text);
    setColour (juce::TooltipWindow::outlineColourId, col::line);
}

// champagne-gold machined knob on a black skirt; the value ring is champagne (amber means "live")
void epsui::LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& sl)
{
    const auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float rad = juce::jmin (r.getWidth(), r.getHeight()) * 0.5f;
    const auto c = r.getCentre();
    const float ang = a0 + pos * (a1 - a0);
    const bool bipolar = (bool) sl.getProperties().getWithDefault ("bipolar", false);

    const float ringR = rad * 0.97f, ringW = juce::jmax (2.5f, rad * 0.06f);
    juce::Path track; track.addCentredArc (c.x, c.y, ringR, ringR, 0, a0, a1, true);
    g.setColour (juce::Colour (0xff0b0c0e));
    g.strokePath (track, juce::PathStrokeType (ringW + 2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    const float from = bipolar ? (a0 + a1) * 0.5f : a0;
    if (std::abs (ang - from) > 0.01f)
    {
        juce::Path val; val.addCentredArc (c.x, c.y, ringR, ringR, 0, juce::jmin (from, ang), juce::jmax (from, ang), true);
        g.setColour (col::champ.withAlpha (0.18f));
        g.strokePath (val, juce::PathStrokeType (ringW + 5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (col::champ);
        g.strokePath (val, juce::PathStrokeType (ringW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    const float body = rad * 0.82f;
    juce::ColourGradient shadow (juce::Colours::black.withAlpha (0.65f), c.x, c.y + body * 0.25f, juce::Colours::transparentBlack, c.x, c.y + body * 1.4f, true);
    g.setGradientFill (shadow);
    g.fillEllipse (c.x - body * 1.25f, c.y - body * 1.0f, body * 2.5f, body * 2.5f);
    juce::ColourGradient skirt (juce::Colour (0xff3b3d42), c.x, c.y - body, juce::Colour (0xff0a0a0c), c.x, c.y + body, false);
    g.setGradientFill (skirt);
    g.fillEllipse (c.x - body, c.y - body, body * 2, body * 2);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    for (int i = 0; i < 60; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * (float) i / 60.0f;
        g.drawLine (c.x + std::sin (a) * body * 0.86f, c.y - std::cos (a) * body * 0.86f, c.x + std::sin (a) * body * 0.99f, c.y - std::cos (a) * body * 0.99f, 1.0f);
    }
    const float cap = body * 0.72f;
    juce::ColourGradient gold (juce::Colour (0xfff8ead0), c.x - cap * 0.6f, c.y - cap, juce::Colour (0xff8a6b40), c.x + cap * 0.4f, c.y + cap, false);
    gold.addColour (0.45, col::champ);
    g.setGradientFill (gold);
    g.fillEllipse (c.x - cap, c.y - cap, cap * 2, cap * 2);
    for (float k = 0.16f; k < 0.98f; k += 0.075f)
    {
        g.setColour (juce::Colours::white.withAlpha (k < 0.6f ? 0.08f : 0.05f));
        g.drawEllipse (c.x - cap * k, c.y - cap * k, cap * 2 * k, cap * 2 * k, 0.8f);
    }
    juce::ColourGradient sheen (juce::Colours::white.withAlpha (0.4f), c.x - cap * 0.45f, c.y - cap * 0.75f, juce::Colours::transparentWhite, c.x, c.y, true);
    g.setGradientFill (sheen);
    g.fillEllipse (c.x - cap, c.y - cap, cap * 2, cap * 2);
    g.setColour (juce::Colour (0xff4a3a22).withAlpha (0.7f));
    g.drawEllipse (c.x - cap, c.y - cap, cap * 2, cap * 2, 1.0f);

    const juce::Point<float> u (std::sin (ang), -std::cos (ang));
    g.setColour (juce::Colour (0xff1e1710));
    g.drawLine (c.x + u.x * cap * 0.18f, c.y + u.y * cap * 0.18f, c.x + u.x * cap * 0.94f, c.y + u.y * cap * 0.94f, juce::jmax (2.2f, rad * 0.05f));
    g.setColour (col::cream);
    g.fillEllipse (juce::Rectangle<float> (rad * 0.09f, rad * 0.09f).withCentre (c + u * body * 0.90f));
}

void epsui::LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.5f);
    const bool invert = (bool) b.getProperties().getWithDefault ("invert", false);
    const bool live = (bool) b.getProperties().getWithDefault ("live", false);
    const bool on = b.getToggleState() != invert;
    const float rad = r.getHeight() * 0.5f;
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (r.expanded (1.5f), rad + 1.5f);
    juce::ColourGradient cap (juce::Colour (0xff36383d).brighter (over ? 0.07f : 0.0f), r.getX(), r.getY(),
                              juce::Colour (0xff1c1d20).darker (down ? 0.25f : 0.0f), r.getX(), r.getBottom(), false);
    g.setGradientFill (cap);
    g.fillRoundedRectangle (r, rad);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + rad, r.getRight() - rad);
    if (on)
    {
        const auto c = live ? col::amber : col::champ;
        g.setColour (c.withAlpha (live ? 0.16f : 0.10f));
        g.fillRoundedRectangle (r, rad);
        g.setColour (c.withAlpha (0.85f));
        g.drawRoundedRectangle (r, rad, 1.3f);
        if (live)
        {
            const auto lamp = juce::Rectangle<float> (7.0f * scale, 7.0f * scale).withCentre ({ r.getX() + rad + 2.0f * scale, r.getCentreY() });
            g.setColour (col::amber.withAlpha (0.3f)); g.fillEllipse (lamp.expanded (4.0f * scale));
            g.setColour (col::amber); g.fillEllipse (lamp);
        }
    }
}

void epsui::LookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    const bool invert = (bool) b.getProperties().getWithDefault ("invert", false);
    const bool live = (bool) b.getProperties().getWithDefault ("live", false);
    const bool on = b.getToggleState() != invert;
    const float fs = (float) b.getProperties().getWithDefault ("fontSize", 12.0f);
    g.setColour (on ? (live ? col::amber : col::cream) : col::quiet);
    g.setFont (font (fs * scale, true, 0.1f));
    g.drawText (b.getButtonText(), b.getLocalBounds().withTrimmedLeft (on && live ? (int) (10 * scale) : 0), juce::Justification::centred);
}

void epsui::LookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& cb)
{
    const auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (1.0f);
    const float rad = r.getHeight() * 0.5f;
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (r, rad);
    g.setColour (col::well);
    g.fillRoundedRectangle (r.reduced (2.0f), rad - 2.0f);
    g.setColour (cb.isMouseOver (true) ? col::champ.withAlpha (0.6f) : col::champ.withAlpha (0.25f));
    g.drawRoundedRectangle (r.reduced (2.0f), rad - 2.0f, 1.0f);
    juce::Path a;
    const float ax = (float) w - 22.0f * scale, ay = (float) h * 0.5f;
    a.addTriangle (ax - 5 * scale, ay - 2.5f * scale, ax + 5 * scale, ay - 2.5f * scale, ax, ay + 3.5f * scale);
    g.setColour (col::champ);
    g.fillPath (a);
}

void epsui::LookAndFeel::positionComboBoxText (juce::ComboBox& cb, juce::Label& l)
{
    l.setBounds (cb.getLocalBounds().withTrimmedRight ((int) (34 * scale)).withTrimmedLeft ((int) (14 * scale)));
    l.setFont (getComboBoxFont (cb));
    l.setJustificationType (juce::Justification::centred);
}

// ================================================================ knob
FetKnob::FetKnob (juce::AudioProcessorValueTreeState& st, const juce::String& id, const juce::String& n, Scale sc, bool big, const juce::String& tip)
    : name (n), scale (sc), large (big)
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setTooltip (tip);
    slider.setMouseDragSensitivity (big ? 320 : 240);
    slider.getProperties().set ("bipolar", sc == Scale::decibels);
    addAndMakeVisible (slider);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (st, id, slider);
    if (auto* prm = st.getParameter (id))
        slider.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
    slider.onValueChange = [this] { repaint (getLocalBounds().removeFromBottom ((int) (26 * s))); };
}

void FetKnob::resized()
{
    const int top = (int) ((large ? 30.0f : 24.0f) * s), bottom = (int) (26.0f * s);
    auto r = getLocalBounds().withTrimmedTop (top).withTrimmedBottom (bottom);
    const int side = juce::jmin (r.getWidth(), r.getHeight());
    const int k = (int) (side * (large ? 0.72f : 0.60f));
    slider.setBounds (r.withSizeKeepingCentre (k, k));
}

void FetKnob::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour (col::text);
    g.setFont (font ((large ? 15.0f : 12.5f) * s, true, 0.22f));
    g.drawText (name.toUpperCase(), b.withHeight ((large ? 26.0f : 20.0f) * s), juce::Justification::centred);

    const auto kb = slider.getBounds().toFloat();
    const auto c = kb.getCentre();
    const float rad = kb.getWidth() * 0.5f;
    const auto rp = slider.getRotaryParameters();
    const int ticks = scale == Scale::sevens ? 7 : 9;
    for (int i = 0; i < ticks; ++i)
    {
        const float t = (float) i / (float) (ticks - 1);
        const float a = rp.startAngleRadians + t * (rp.endAngleRadians - rp.startAngleRadians);
        const juce::Point<float> u (std::sin (a), -std::cos (a));
        const bool major = i % 2 == 0 || scale == Scale::sevens;
        g.setColour (col::quiet.withAlpha (major ? 0.9f : 0.5f));
        g.drawLine (c.x + u.x * rad * 1.07f, c.y + u.y * rad * 1.07f, c.x + u.x * rad * (major ? 1.17f : 1.13f), c.y + u.y * rad * (major ? 1.17f : 1.13f), 1.4f * s);
        juce::String lab;
        if (scale == Scale::sevens) lab = juce::String (i + 1);
        else if (scale == Scale::decibels) lab = major ? ((i > 4 ? "+" : "") + juce::String (-24 + i * 6)) : juce::String();
        else lab = major ? juce::String (i * 100 / 8) : juce::String();
        if (lab.isNotEmpty())
        {
            const float rl = rad * (large ? 1.30f : 1.40f);
            g.setColour (col::quiet);
            g.setFont (font ((large ? 11.5f : 10.0f) * s, true));
            g.drawText (lab, juce::Rectangle<float> (30 * s, 13 * s).withCentre ({ c.x + u.x * rl, c.y + u.y * rl }), juce::Justification::centred);
        }
    }
    juce::String v;
    const double val = slider.getValue();
    if (scale == Scale::decibels) v = (val > 0.05 ? "+" : "") + juce::String (val, 1) + " dB";
    else if (scale == Scale::sevens) v = juce::String (val, 1);
    else v = juce::String (juce::roundToInt (val)) + " %";
    g.setColour (col::cream);
    g.setFont (font ((large ? 15.0f : 12.5f) * s, true));
    g.drawText (v, b.withTrimmedTop (b.getHeight() - 24 * s), juce::Justification::centred);
}

// ================================================================ VU meter (the hero)
static juce::Path archShape (juce::Rectangle<float> r, float corner)
{
    juce::Path p;
    const float rise = r.getHeight() * 0.13f;
    p.startNewSubPath (r.getX(), r.getY() + rise);
    p.quadraticTo (r.getCentreX(), r.getY() - rise, r.getRight(), r.getY() + rise);
    p.lineTo (r.getRight(), r.getBottom() - corner);
    p.quadraticTo (r.getRight(), r.getBottom(), r.getRight() - corner, r.getBottom());
    p.lineTo (r.getX() + corner, r.getBottom());
    p.quadraticTo (r.getX(), r.getBottom(), r.getX(), r.getBottom() - corner);
    p.closeSubPath();
    return p;
}

juce::Rectangle<float> VuMeter::window() const { return getLocalBounds().toFloat().reduced (18 * s); }
juce::Point<float> VuMeter::pivot() const { const auto w = window(); return { w.getCentreX(), w.getBottom() + w.getHeight() * 0.42f }; }
float VuMeter::radius() const { return window().getHeight() * 1.10f; }

float VuMeter::angleFor (float vu) const
{
    const float v = juce::jlimit (-22.0f, 4.0f, vu);
    const float lo = std::pow (10.0f, -22.0f / 20.0f), hi = std::pow (10.0f, 4.0f / 20.0f);
    const float t = (std::pow (10.0f, v / 20.0f) - lo) / (hi - lo);
    return juce::degreesToRadians (-36.0f + 72.0f * t);
}

void VuMeter::renderFace()
{
    const int w = getWidth(), h = getHeight();
    if (w <= 0 || h <= 0) return;
    face = juce::Image (juce::Image::ARGB, w * 2, h * 2, true);
    juce::Graphics g (face);
    g.addTransform (juce::AffineTransform::scale (2.0f));
    const auto r = getLocalBounds().toFloat();
    const auto win = window();

    juce::ColourGradient bezel (juce::Colour (0xfff5e6c8), r.getX(), r.getY(), juce::Colour (0xff6e5533), r.getX(), r.getBottom(), false);
    bezel.addColour (0.5, col::champDeep);
    g.setGradientFill (bezel);
    g.fillPath (archShape (r.reduced (3 * s), 34 * s));
    g.setColour (juce::Colour (0xff070708));
    g.fillPath (archShape (r.reduced (9 * s), 29 * s));

    const auto winPath = archShape (win, 22 * s);
    juce::ColourGradient lamp (juce::Colour (0xfffff4d8), win.getCentreX(), win.getBottom(), juce::Colour (0xffe0a24a), win.getCentreX(), win.getY() - win.getHeight() * 0.5f, true);
    g.setGradientFill (lamp);
    g.fillPath (winPath);
    g.reduceClipRegion (winPath);
    juce::Random rng (11);
    for (int i = 0; i < 1200; ++i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.015f + 0.03f * rng.nextFloat()));
        g.fillRect (win.getX() + rng.nextFloat() * win.getWidth(), win.getY() + rng.nextFloat() * win.getHeight(), 1.0f, 1.0f);
    }

    const auto pv = pivot(); const float R = radius();
    const juce::Colour inkC (0xff2a2016);
    const float marks[] = { -20, -10, -7, -5, -3, -2, -1, 0, 1, 2, 3 };
    for (float m : marks)
    {
        const float a = angleFor (m);
        const juce::Point<float> u (std::sin (a), -std::cos (a));
        g.setColour (m > 0 ? col::red : inkC);
        g.drawLine (pv.x + u.x * R * 0.80f, pv.y + u.y * R * 0.80f, pv.x + u.x * R * 0.89f, pv.y + u.y * R * 0.89f, 1.8f * s);
        g.setFont (font (13.0f * s, true));
        g.drawText (m > 0 ? "+" + juce::String ((int) m) : juce::String ((int) m),
                    juce::Rectangle<float> (30 * s, 15 * s).withCentre ({ pv.x + u.x * R * 0.97f, pv.y + u.y * R * 0.97f }), juce::Justification::centred);
    }
    for (float m = -20.0f; m <= 3.0f; m += 0.5f)
    {
        const float a = angleFor (m);
        const juce::Point<float> u (std::sin (a), -std::cos (a));
        g.setColour ((m > 0 ? col::red : inkC).withAlpha (0.6f));
        g.drawLine (pv.x + u.x * R * 0.80f, pv.y + u.y * R * 0.80f, pv.x + u.x * R * 0.835f, pv.y + u.y * R * 0.835f, 0.9f * s);
    }
    juce::Path arc; arc.addCentredArc (pv.x, pv.y, R * 0.80f, R * 0.80f, 0, angleFor (-22), angleFor (0), true);
    g.setColour (inkC); g.strokePath (arc, juce::PathStrokeType (1.6f * s));
    juce::Path red; red.addCentredArc (pv.x, pv.y, R * 0.80f, R * 0.80f, 0, angleFor (0), angleFor (4), true);
    g.setColour (col::red); g.strokePath (red, juce::PathStrokeType (5.0f * s));
    g.setColour (inkC);
    g.setFont (font (22.0f * s, true, 0.12f));
    g.drawText ("VU", win.withTrimmedTop (win.getHeight() * 0.55f).withHeight (26 * s), juce::Justification::centred);
}

void VuMeter::tick()
{
    float target = 0.0f;
    if (mode == 0) target = proc.readout().grDb.load();
    else if (mode == 1 || mode == 2)
    {
        const float rms = juce::Decibels::gainToDecibels (proc.outLevel.load(), -100.0f);
        target = rms + 21.0f - (mode == 2 ? 4.0f : 0.0f);                       // +4 dBu sine = -21 dBFS RMS
    }
    else target = -40.0f;
    const float dt = 1.0f / 60.0f, w0 = 2.0f * juce::MathConstants<float>::pi * 2.1f, z = 0.78f;
    const float acc = w0 * w0 * (juce::jlimit (-26.0f, 6.0f, target) - needle) - 2.0f * z * w0 * vel;
    vel += acc * dt; needle += vel * dt;
    repaint (window().toNearestInt());
}

void VuMeter::paint (juce::Graphics& g)
{
    if (face.isNull()) renderFace();
    g.drawImage (face, getLocalBounds().toFloat());
    const auto win = window();
    const auto pv = pivot(); const float R = radius();
    const float a = angleFor (needle);
    const juce::Point<float> u (std::sin (a), -std::cos (a));
    g.saveState();
    g.reduceClipRegion (archShape (win, 22 * s));
    g.setColour (juce::Colours::black.withAlpha (0.18f));
    g.drawLine (pv.x + u.x * R * 0.30f + 4 * s, pv.y + u.y * R * 0.30f + 4 * s, pv.x + u.x * R * 0.91f + 4 * s, pv.y + u.y * R * 0.91f + 4 * s, 2.2f * s);
    g.setColour (juce::Colour (0xff15100c));
    g.drawLine (pv.x + u.x * R * 0.30f, pv.y + u.y * R * 0.30f, pv.x + u.x * R * 0.92f, pv.y + u.y * R * 0.92f, 1.8f * s);
    juce::Path glare;
    glare.startNewSubPath (win.getX(), win.getY());
    glare.lineTo (win.getRight(), win.getY());
    glare.lineTo (win.getRight(), win.getY() + win.getHeight() * 0.20f);
    glare.quadraticTo (win.getCentreX(), win.getY() + win.getHeight() * 0.42f, win.getX(), win.getY() + win.getHeight() * 0.30f);
    glare.closeSubPath();
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.fillPath (glare);
    g.restoreState();
    g.setColour (juce::Colour (0xff2a2016).withAlpha (0.7f));
    g.setFont (font (10.5f * s, true, 0.25f));
    g.drawText (juce::StringArray { "GAIN REDUCTION", "OUTPUT  +4 dBu = 0 VU", "OUTPUT  +8 dBu = 0 VU", "METER OFF" }[mode],
                win.withTrimmedTop (win.getHeight() * 0.78f).withHeight (16 * s), juce::Justification::centred);
}

// ================================================================ X-ray: the live circuit behind the controls
namespace xr
{
    using layout::yMain; using layout::ySide; using layout::yBus;
    struct Seg { float x0, y0, x1, y1; };
    // Main path; gaps are where the parts sit. Input pot under the Input knob, Output pot under the
    // Output knob, the FET between them, detector, attack R and C22 under Attack, release R under Release.
    const Seg mainW[] = { { 30, yMain, 100, yMain }, { 160, yMain, 496, yMain }, { 552, yMain, 566, yMain }, { 626, yMain, 652, yMain },
                          { 694, yMain, 722, yMain }, { 778, yMain, 796, yMain }, { 836, yMain, 856, yMain }, { 916, yMain, 1000, yMain } };
    const Seg sideW[] = { { 706, yMain, 706, ySide }, { 706, ySide, 860, ySide }, { 876, ySide, 900, ySide }, { 960, ySide, 1040, ySide } };
    const Seg gateW[] = { { 1040, ySide + 34, 1040, yBus }, { 1040, yBus, 618, yBus }, { 618, yBus, 618, 290 }, { 618, 290, 632, 290 } };
    constexpr float fetX = 640, c22X = 978;
}

void XrayView::renderStatic()
{
    const int w = getWidth(), h = getHeight();
    if (w <= 0 || h <= 0) return;
    staticLayer = juce::Image (juce::Image::ARGB, w * 2, h * 2, true, juce::SoftwareImageType());
    juce::Graphics g (staticLayer);
    g.addTransform (juce::AffineTransform::scale (2.0f));
    using namespace xr;

    g.fillAll (juce::Colour (0xff08090a));
    const auto bodyR = juce::Rectangle<float> (6 * s, 4 * s, (layout::W - 12) * s, (layout::H - 8) * s);
    juce::Path body; body.addRoundedRectangle (bodyR, 34 * s);
    juce::ColourGradient bg (juce::Colour (0xff141a24), 0, 0, col::glass, 0, (float) h, false);
    g.setGradientFill (bg);
    g.fillPath (body);
    g.saveState(); g.reduceClipRegion (body);
    g.setColour (juce::Colours::white.withAlpha (0.03f));
    for (float x = 12; x < layout::W; x += 12) g.drawVerticalLine ((int) (x * s), 0, (float) h);
    for (float y = 12; y < layout::H; y += 12) g.drawHorizontalLine ((int) (y * s), 0, (float) w);
    g.restoreState();

    const juce::Colour wire = col::amber.withAlpha (0.30f), sym = col::amber.withAlpha (0.62f);
    const float lw = 1.8f * s;
    auto line = [&] (float x0, float y0, float x1, float y1, juce::Colour c) { g.setColour (c); g.drawLine (x0 * s, y0 * s, x1 * s, y1 * s, lw); };
    auto text = [&] (const juce::String& t, float x, float y, float wdt, juce::Justification j, float size = 10.5f, juce::Colour c = col::steel.withAlpha (0.7f))
    {
        g.setColour (c); g.setFont (font (size * s, true, 0.12f));
        g.drawText (t, juce::Rectangle<float> (x * s, y * s, wdt * s, 14 * s), j);
    };
    auto zigzag = [&] (float a0, float a1, float at, bool vertical = false)
    {
        juce::Path p; const int n = 6; const float a = 6.0f;
        if (! vertical)
        {
            p.startNewSubPath (a0 * s, at * s);
            for (int i = 0; i < n; ++i) p.lineTo ((a0 + (a1 - a0) * ((float) i + 0.5f) / n) * s, (at + (i % 2 ? a : -a)) * s);
            p.lineTo (a1 * s, at * s);
        }
        else
        {
            p.startNewSubPath (at * s, a0 * s);
            for (int i = 0; i < n; ++i) p.lineTo ((at + (i % 2 ? a : -a)) * s, (a0 + (a1 - a0) * ((float) i + 0.5f) / n) * s);
            p.lineTo (at * s, a1 * s);
        }
        g.setColour (sym); g.strokePath (p, juce::PathStrokeType (lw, juce::PathStrokeType::mitered));
    };
    auto coil = [&] (float x, float y0, float y1, bool right)
    {
        juce::Path p; const int n = 4; const float hgt = (y1 - y0) / n;
        for (int i = 0; i < n; ++i)
            p.addCentredArc (x * s, (y0 + hgt * ((float) i + 0.5f)) * s, 7 * s, hgt * 0.5f * s, 0,
                             right ? 0.0f : juce::MathConstants<float>::pi, right ? juce::MathConstants<float>::pi : juce::MathConstants<float>::twoPi, true);
        g.setColour (sym); g.strokePath (p, juce::PathStrokeType (lw));
    };
    auto transformer = [&] (float x, const juce::String& name)
    {
        coil (x + 8, yMain - 26, yMain + 26, false); coil (x + 52, yMain - 26, yMain + 26, true);
        line (x + 27, yMain - 28, x + 27, yMain + 28, sym); line (x + 33, yMain - 28, x + 33, yMain + 28, sym);
        text (name, x - 20, yMain + 32, 100, juce::Justification::centred);
    };
    auto amp = [&] (float x0, float x1, const juce::String& name)
    {
        juce::Path p; p.addTriangle (x0 * s, (yMain - 20) * s, x0 * s, (yMain + 20) * s, x1 * s, yMain * s);
        g.setColour (col::glass); g.fillPath (p);
        g.setColour (sym); g.strokePath (p, juce::PathStrokeType (lw));
        text (name, x0 - 30, yMain - 40, (x1 - x0) + 60, juce::Justification::centred, 9.5f);
    };
    auto ground = [&] (float x, float y) { line (x - 9, y, x + 9, y, sym); line (x - 6, y + 4, x + 6, y + 4, sym); line (x - 3, y + 8, x + 3, y + 8, sym); };
    auto dot = [&] (float x, float y) { g.setColour (sym); g.fillEllipse (juce::Rectangle<float> (7 * s, 7 * s).withCentre (P (x, y))); };

    for (auto& sg : mainW) line (sg.x0, sg.y0, sg.x1, sg.y1, wire);
    for (auto& sg : sideW) line (sg.x0, sg.y0, sg.x1, sg.y1, wire);
    {
        juce::Path gw; gw.startNewSubPath (P (gateW[0].x0, gateW[0].y0));
        for (auto& sg : gateW) gw.lineTo (P (sg.x1, sg.y1));
        g.setColour (wire.withAlpha (0.25f));
        g.strokePath (gw.createPathWithRoundedCorners (22 * s), juce::PathStrokeType (lw));
    }
    text ("IN", 30, yMain - 22, 30, juce::Justification::centredLeft, 12.0f, col::steel);
    transformer (100, "INPUT XFMR");
    zigzag (496, 552, yMain); line (524, yMain + 22, 524, yMain + 8, sym);
    zigzag (566, 626, yMain);
    text ("R5 27k", 562, yMain - 30, 68, juce::Justification::centred, 9.5f);
    dot (fetX, yMain);
    line (fetX, yMain, fetX, 262, wire); line (fetX, 298, fetX, 318, wire); ground (fetX, 318);
    g.setColour (sym); g.drawLine (632 * s, 260 * s, 632 * s, 300 * s, 2.6f * s);
    line (632, 264, fetX, 264, sym); line (632, 296, fetX, 296, sym);
    { juce::Path ar; ar.addTriangle (P (625, 286), P (625, 294), P (632, 290)); g.setColour (sym); g.fillPath (ar); }
    text ("Q1 FET", 650, 300, 80, juce::Justification::centredLeft, 9.5f);
    amp (652, 694, "PRE");
    dot (706, yMain);
    zigzag (722, 778, yMain); line (750, yMain + 22, 750, yMain + 8, sym);
    amp (796, 836, "Q6");
    transformer (856, "OUTPUT XFMR");
    text ("OUT", 1004, yMain - 7, 50, juce::Justification::centredLeft, 12.0f, col::steel);
    { juce::Path d; d.addTriangle (P (860, ySide - 9), P (860, ySide + 9), P (876, ySide)); g.setColour (sym); g.fillPath (d);
      g.drawLine (876 * s, (ySide - 9) * s, 876 * s, (ySide + 9) * s, lw); }
    zigzag (900, 960, ySide);
    dot (c22X, ySide);
    line (c22X, ySide, c22X, ySide + 10, wire);
    g.setColour (sym); g.drawLine ((c22X - 13) * s, (ySide + 10) * s, (c22X + 13) * s, (ySide + 10) * s, 2.6f * s);
    g.drawLine ((c22X - 13) * s, (ySide + 18) * s, (c22X + 13) * s, (ySide + 18) * s, 2.6f * s);
    line (c22X, ySide + 18, c22X, ySide + 26, wire); ground (c22X, ySide + 26);
    zigzag (ySide, ySide + 34, 1040, true);
    text ("SIDECHAIN", 714, 300, 90, juce::Justification::centredLeft, 9.5f, col::amber.withAlpha (0.55f));
    text ("GATE CONTROL", 650, yBus - 16, 140, juce::Justification::centredLeft, 9.5f, col::amber.withAlpha (0.45f));
}

void XrayView::tick()
{
    const auto& r = proc.readout();
    auto ease = [] (float& v, float t, float k = 0.25f) { v += k * (t - v); };
    ease (level, juce::jlimit (0.0f, 1.0f, (juce::Decibels::gainToDecibels (proc.outLevel.load(), -80.0f) + 60.0f) / 54.0f));
    ease (gr, r.grDb.load()); ease (gate, r.gateV.load()); ease (cap, r.capV.load()); ease (flux, r.inFluxPct.load(), 0.15f);
    ohms = r.fetOhms.load();
    ratioIdx = juce::roundToInt (proc.apvts.getRawParameterValue ("ratio")->load());
    phaseMain = std::fmod (phaseMain + (0.4f + 3.0f * level), 40.0f);
    phaseSide = std::fmod (phaseSide + (0.2f + 4.0f * juce::jlimit (0.0f, 1.0f, cap / 3.0f)), 30.0f);
    if (getWidth() > 0 && getHeight() > 0)
    {
        if (frameImg.isNull() || frameImg.getWidth() != getWidth() * 2)
            frameImg = juce::Image (juce::Image::RGB, getWidth() * 2, getHeight() * 2, true, juce::SoftwareImageType());
        juce::Graphics fg (frameImg);
        fg.addTransform (juce::AffineTransform::scale (2.0f));
        drawFrame (fg);
    }
    repaint();
}

void XrayView::paint (juce::Graphics& g)
{
    if (frameImg.isNull() || frameImg.getWidth() != getWidth() * 2)
    {
        frameImg = juce::Image (juce::Image::RGB, juce::jmax (1, getWidth() * 2), juce::jmax (1, getHeight() * 2), true, juce::SoftwareImageType());
        juce::Graphics fg (frameImg);
        fg.addTransform (juce::AffineTransform::scale (2.0f));
        drawFrame (fg);
    }
    g.drawImage (frameImg, getLocalBounds().toFloat());
}

void XrayView::drawFrame (juce::Graphics& g)
{
    using namespace xr;
    if (staticLayer.isNull()) renderStatic();
    g.drawImage (staticLayer, getLocalBounds().toFloat());
    const float grAmt = juce::jlimit (0.0f, 1.0f, -gr / 20.0f), capAmt = juce::jlimit (0.0f, 1.0f, cap / 3.0f);

    auto glow = [&] (const Seg* sg, int n, juce::Colour c, float amt)
    {
        if (amt < 0.02f) return;
        for (int k = 3; k >= 0; --k)
        {
            g.setColour (c.withAlpha (amt * (k == 0 ? 0.9f : 0.12f)));
            for (int i = 0; i < n; ++i) g.drawLine (sg[i].x0 * s, sg[i].y0 * s, sg[i].x1 * s, sg[i].y1 * s, (1.8f + (float) k * 2.8f) * s);
        }
    };
    glow (mainW, (int) std::size (mainW), col::amber, 0.25f + 0.65f * level);
    glow (sideW, (int) std::size (sideW), col::amberDeep, 0.15f + 0.7f * capAmt);
    {
        juce::Path gw; gw.startNewSubPath (P (gateW[0].x0, gateW[0].y0));
        for (auto& sg : gateW) gw.lineTo (P (sg.x1, sg.y1));
        const auto rounded = gw.createPathWithRoundedCorners (22 * s);
        if (grAmt > 0.02f)
            for (int k = 3; k >= 0; --k)
            {
                g.setColour (juce::Colour (0xffffd27a).withAlpha (grAmt * (k == 0 ? 0.8f : 0.1f)));
                g.strokePath (rounded, juce::PathStrokeType ((1.8f + (float) k * 2.8f) * s));
            }
    }
    auto dots = [&] (const Seg* sg, int n, float spacing, float phase, juce::Colour c, float amt)
    {
        if (amt < 0.05f) return;
        float dist = 0;
        for (int i = 0; i < n; ++i)
        {
            const float len = std::hypot (sg[i].x1 - sg[i].x0, sg[i].y1 - sg[i].y0);
            for (float d = std::fmod (spacing - std::fmod (dist, spacing) + phase, spacing); d < len; d += spacing)
            {
                const float t = d / len;
                const auto p = P (sg[i].x0 + (sg[i].x1 - sg[i].x0) * t, sg[i].y0 + (sg[i].y1 - sg[i].y0) * t);
                g.setColour (c.withAlpha (0.25f * amt)); g.fillEllipse (juce::Rectangle<float> (10 * s, 10 * s).withCentre (p));
                g.setColour (c.withAlpha (amt)); g.fillEllipse (juce::Rectangle<float> (4 * s, 4 * s).withCentre (p));
            }
            dist += len;
        }
    };
    dots (mainW, (int) std::size (mainW), 40.0f, phaseMain, juce::Colour (0xffffe0a0), 0.3f + 0.7f * level);
    dots (sideW, (int) std::size (sideW), 30.0f, phaseSide, col::amber, capAmt);
    dots (gateW, (int) std::size (gateW), 30.0f, phaseSide, juce::Colour (0xffffd27a), grAmt);

    const auto fet = P (fetX - 4, 280);
    juce::ColourGradient fg (col::amber.withAlpha (0.15f + 0.75f * grAmt), fet.x, fet.y, col::amber.withAlpha (0.0f), fet.x + 54 * s, fet.y, true);
    g.setGradientFill (fg); g.fillEllipse (juce::Rectangle<float> (108 * s, 108 * s).withCentre (fet));
    g.setColour (col::amber.withAlpha (0.5f + 0.5f * grAmt)); g.drawLine (632 * s, 260 * s, 632 * s, 300 * s, 2.8f * s);

    g.setColour (col::amber.withAlpha (0.2f + 0.7f * capAmt));
    g.fillRect (juce::Rectangle<float> ((c22X - 11) * s, (layout::ySide + 11 + 6 * (1.0f - capAmt)) * s, 22 * s, 6 * capAmt * s));
    if (capAmt > 0.05f)
    {
        juce::ColourGradient cg (col::amber.withAlpha (0.45f * capAmt), c22X * s, (layout::ySide + 14) * s, col::amber.withAlpha (0.0f), (c22X + 32) * s, (layout::ySide + 14) * s, true);
        g.setGradientFill (cg); g.fillEllipse (juce::Rectangle<float> (64 * s, 64 * s).withCentre (P (c22X, layout::ySide + 14)));
    }
    const float fx = juce::jlimit (0.0f, 1.0f, flux / 100.0f);
    if (fx > 0.01f)
    {
        juce::ColourGradient cg (col::red.withAlpha (0.55f * fx), 130 * s, layout::yMain * s, col::red.withAlpha (0.0f), 166 * s, layout::yMain * s, true);
        g.setGradientFill (cg); g.fillEllipse (juce::Rectangle<float> (80 * s, 80 * s).withCentre (P (130, layout::yMain)));
    }

    // the faceplate as smoked glass over the circuit
    if (ghostPlate != nullptr && ghostPlate->isValid())
    {
        g.setOpacity (0.14f);
        g.drawImage (*ghostPlate, getLocalBounds().toFloat());
        g.setOpacity (1.0f);
    }

    auto val = [&] (const juce::String& t, float x, float y, float w, juce::Justification j, float size = 13.0f, juce::Colour c = col::amber)
    {
        g.setFont (font (size * s, true));
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawText (t, juce::Rectangle<float> (x * s + 1, y * s + 1, w * s, 18 * s), j);
        g.setColour (c);
        g.drawText (t, juce::Rectangle<float> (x * s, y * s, w * s, 18 * s), j);
    };
    auto fmtOhms = [] (float o)
    {
        if (o >= 1.0e6f) return juce::String (o / 1.0e6f, 1) + utf8 (" M\xce\xa9");
        if (o >= 1.0e3f) return juce::String (o / 1.0e3f, 1) + utf8 (" k\xce\xa9");
        return juce::String (juce::roundToInt (o)) + utf8 (" \xce\xa9");
    };
    val ("CORE " + juce::String (juce::roundToInt (flux)) + " %", 80, layout::yMain + 50, 100, juce::Justification::centred, 12.0f, fx > 0.3f ? col::red : col::amber);
    val ("GATE " + juce::String (gate, 2) + " V", 648, 316, 120, juce::Justification::centredLeft);
    val (fmtOhms (ohms), 650, 334, 120, juce::Justification::centredLeft, 12.0f, col::steel);
    val ("C22 " + juce::String (cap, 2) + " V", 856, layout::ySide + 12, 106, juce::Justification::centredRight, 12.0f);
    val ("RATIO " + juce::StringArray { "4:1", "8:1", "12:1", "20:1", "ALL" }[juce::jlimit (0, 4, ratioIdx)], 714, 352, 120, juce::Justification::centredLeft, 11.0f);
    val ("GR " + juce::String (gr, 1) + " dB", 40, 434, 200, juce::Justification::centredLeft, 18.0f);
    g.setColour (col::steel.withAlpha (0.6f));
    g.setFont (font (10.5f * s, true, 0.28f));
    g.drawText (utf8 ("X-RAY  \xc2\xb7  LIVE CIRCUIT SIMULATION"), juce::Rectangle<float> (40 * s, 458 * s, 400 * s, 14 * s), juce::Justification::centredLeft);
}

// ================================================================ editor
EpsilonFetEditor::EpsilonFetEditor (EpsilonFetProcessor& p)
    : AudioProcessorEditor (&p), proc (p), circuit (p), vu (p),
      input  (p.apvts, "input", "Input", FetKnob::Scale::decibels, true,
              "Input: drives the signal into the FET. Turn it up for more compression (the threshold is fixed, as on the hardware)."),
      output (p.apvts, "output", "Output", FetKnob::Scale::decibels, true, "Output: level after the compressor, into the class-A output stage and transformer."),
      attack (p.apvts, "attack", "Attack", FetKnob::Scale::sevens, false, utf8 ("Attack: 7 is fastest (about 20 \xc2\xb5s detector time), 1 is slowest (about 0.8 ms).")),
      release (p.apvts, "release", "Release", FetKnob::Scale::sevens, false, "Release: 7 is fastest (about 50 ms), 1 is slowest (about 1.1 s)."),
      mix (p.apvts, "mix", "Mix", FetKnob::Scale::percent, false, "Mix: blend the compressed and the original signal (parallel compression), time-aligned.")
{
    setLookAndFeel (&lnf);
    addChildComponent (circuit);                    // behind everything; shown in X-ray mode
    circuit.ghostPlate = &plate;
    for (auto* c : std::initializer_list<juce::Component*> { &vu, &input, &output, &attack, &release, &mix })
        addAndMakeVisible (c);

    const char* ratios[] = { "4", "8", "12", "20", "ALL" };
    for (int i = 0; i < 5; ++i)
    {
        ratio[i].setButtonText (ratios[i]);
        ratio[i].getProperties().set ("fontSize", 14.0f);
        ratio[i].getProperties().set ("live", true);
        ratio[i].setTooltip (i < 4 ? juce::String ("Ratio ") + ratios[i] + ":1. Higher ratios also raise the threshold, as on the hardware."
                                   : juce::String ("All buttons in: the ratio networks load each other. Lower threshold, faster attack, heavy and aggressive."));
        addAndMakeVisible (ratio[i]);
    }
    ratioAtt = std::make_unique<juce::ParameterAttachment> (*p.apvts.getParameter ("ratio"), [this] (float v)
    {
        for (int i = 0; i < 5; ++i) ratio[i].setToggleState (juce::roundToInt (v) == i, juce::dontSendNotification);
    });
    for (int i = 0; i < 5; ++i) ratio[i].onClick = [this, i] { ratioAtt->setValueAsCompleteGesture ((float) i); };
    ratioAtt->sendInitialUpdate();

    const char* meters[] = { "GR", "+4", "+8", "OFF" };
    for (int i = 0; i < 4; ++i)
    {
        meterBtn[i].setButtonText (meters[i]);
        meterBtn[i].onClick = [this, i] { setMeterMode (i); };
        addAndMakeVisible (meterBtn[i]);
    }
    meterBtn[0].setTooltip ("Meter shows gain reduction.");
    meterBtn[1].setTooltip ("Meter shows output level, 0 VU = +4 dBu (-18 dBFS peak sine).");
    meterBtn[2].setTooltip ("Meter shows output level, 0 VU = +8 dBu.");
    meterBtn[3].setTooltip ("Meter off.");
    setMeterMode ((int) proc.apvts.state.getProperty ("meter", 0));

    linkBtn.setClickingTogglesState (true);
    linkBtn.setTooltip ("Stereo link: both channels share one detector capacitor, so the stereo image stays put.");
    linkAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "link", linkBtn);
    qualityBtn.setTooltip ("Circuit rate: Studio runs the circuit at 4x the host rate (default), Eco at 2x for big sessions.");
    qualityAtt = std::make_unique<juce::ParameterAttachment> (*p.apvts.getParameter ("quality"), [this] (float v)
    {
        qualityBtn.setButtonText (juce::roundToInt (v) == 0 ? "ECO 2X" : "STUDIO 4X");
        qualityBtn.setToggleState (true, juce::dontSendNotification);
    });
    qualityBtn.onClick = [this] { qualityAtt->setValueAsCompleteGesture (juce::roundToInt (proc.apvts.getRawParameterValue ("quality")->load()) == 1 ? 0.0f : 1.0f); };
    qualityAtt->sendInitialUpdate();
    bypassBtn.setClickingTogglesState (true);
    bypassBtn.getProperties().set ("invert", true);
    bypassBtn.getProperties().set ("live", true);
    bypassBtn.setTooltip ("Power: lamp lit = processing. Off = bypass (latency is kept, so A/B stays in time).");
    bypassAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "bypass", bypassBtn);
    xrayBtn.setClickingTogglesState (true);
    xrayBtn.getProperties().set ("live", true);
    xrayBtn.setTooltip ("X-ray: see through the faceplate to the circuit being simulated, live. Every part sits behind the control that drives it.");
    xrayBtn.onClick = [this] { setXray (xrayBtn.getToggleState()); };
    for (auto* b : std::initializer_list<juce::Component*> { &linkBtn, &qualityBtn, &bypassBtn, &xrayBtn }) addAndMakeVisible (b);

    for (int i = 0; i < proc.getNumPrograms(); ++i) presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedItemIndex (proc.currentPreset, juce::dontSendNotification);
    presetBox.onChange = [this] { proc.loadPreset (presetBox.getSelectedItemIndex()); };
    presetBox.setTooltip ("Factory presets.");
    addAndMakeVisible (presetBox);

    setResizable (true, true);
    setResizeLimits ((int) (kW * 0.7f), (int) (kH * 0.7f), (int) (kW * 1.6f), (int) (kH * 1.6f));
    getConstrainer()->setFixedAspectRatio (kW / kH);
    setSize ((int) kW, (int) kH);
    setXray ((bool) proc.apvts.state.getProperty ("xray", false));
    startTimerHz (60);
}

EpsilonFetEditor::~EpsilonFetEditor() { stopTimer(); setLookAndFeel (nullptr); }

void EpsilonFetEditor::setXray (bool on)
{
    xray = on;
    proc.apvts.state.setProperty ("xray", on, nullptr);
    xrayBtn.setToggleState (on, juce::dontSendNotification);
    circuit.setVisible (on);
    // the controls turn to glass so the circuit behind them shows through
    for (auto* c : std::initializer_list<juce::Component*> { &input, &output, &attack, &release, &mix })
        c->setAlpha (on ? 0.5f : 1.0f);
    for (auto* c : std::initializer_list<juce::Component*> { &ratio[0], &ratio[1], &ratio[2], &ratio[3], &ratio[4], &linkBtn, &qualityBtn,
                                                             &meterBtn[0], &meterBtn[1], &meterBtn[2], &meterBtn[3] })
        c->setAlpha (on ? 0.7f : 1.0f);
    vu.setAlpha (on ? 0.22f : 1.0f);
    if (on) circuit.tick();
    repaint();
}

void EpsilonFetEditor::setMeterMode (int m)
{
    vu.mode = juce::jlimit (0, 3, m);
    proc.apvts.state.setProperty ("meter", vu.mode, nullptr);
    for (int i = 0; i < 4; ++i) meterBtn[i].setToggleState (i == vu.mode, juce::dontSendNotification);
    vu.repaint();
}

void EpsilonFetEditor::timerCallback()
{
    // meter and X-ray take turns at 30 fps each; with X-ray off the circuit view costs nothing
    // (X-ray redraws the whole window, so it runs at 20 fps)
    ++frame;
    if (frame & 1) vu.tick();
    else if (xray && frame % 6 == 0) circuit.tick();
    if (presetBox.getSelectedItemIndex() != proc.currentPreset)
        presetBox.setSelectedItemIndex (proc.currentPreset, juce::dontSendNotification);
}

juce::Rectangle<int> EpsilonFetEditor::sc (float x, float y, float w, float h) const
{
    return juce::Rectangle<float> (x * s, y * s, w * s, h * s).toNearestInt();
}

void EpsilonFetEditor::resized()
{
    s = (float) getWidth() / kW;
    lnf.scale = s;
    vu.s = circuit.s = s;
    for (auto* k : { &input, &output, &attack, &release, &mix }) k->s = s;
    plate = {};

    circuit.setBounds (getLocalBounds());
    presetBox.setBounds (sc (392, 22, 300, 36));
    xrayBtn.setBounds (sc (744, 22, 116, 36));
    bypassBtn.setBounds (sc (874, 22, 166, 36));

    vu.setBounds (sc (layout::vu));
    for (int i = 0; i < 4; ++i) meterBtn[i].setBounds (sc (56 + i * 84.0f, 392, 76, 34));

    input.setBounds (sc (layout::input));
    output.setBounds (sc (layout::output));
    for (int i = 0; i < 5; ++i) ratio[i].setBounds (sc (layout::ratioRow.getX() + i * 82.0f, layout::ratioRow.getY(), 74, layout::ratioRow.getHeight()));

    mix.setBounds (sc (layout::mix));
    linkBtn.setBounds (sc (980, 92, 82, 34));
    qualityBtn.setBounds (sc (980, 140, 82, 34));
    attack.setBounds (sc (layout::attack));
    release.setBounds (sc (layout::release));
    for (auto* k : { &input, &output, &attack, &release, &mix }) k->resized();
}

void EpsilonFetEditor::renderPlate()
{
    const int w = getWidth(), h = getHeight();
    plate = juce::Image (juce::Image::RGB, w * 2, h * 2, true);
    juce::Graphics g (plate);
    g.addTransform (juce::AffineTransform::scale (2.0f));

    g.fillAll (juce::Colour (0xff08090a));
    const auto bodyR = sc (6, 4, kW - 12, kH - 8).toFloat();
    juce::Path body; body.addRoundedRectangle (bodyR, 34 * s);
    juce::ColourGradient metal (col::body0, 0, bodyR.getY(), col::body1, 0, bodyR.getBottom(), false);
    g.setGradientFill (metal);
    g.fillPath (body);
    g.saveState(); g.reduceClipRegion (body);
    juce::Random rng (5);
    for (float y = 0; y < (float) h; y += 0.75f)
    {
        g.setColour ((rng.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (0.008f + 0.016f * rng.nextFloat()));
        g.drawHorizontalLine ((int) y, 0, (float) w);
    }
    juce::ColourGradient sheen (juce::Colours::white.withAlpha (0.06f), (float) w * 0.3f, -80 * s, juce::Colours::transparentWhite, (float) w * 0.3f, 280 * s, true);
    g.setGradientFill (sheen); g.fillRect (bodyR);
    g.restoreState();
    g.setColour (juce::Colours::white.withAlpha (0.10f)); g.strokePath (body, juce::PathStrokeType (1.0f));
    g.setColour (col::champ.withAlpha (0.25f)); g.drawHorizontalLine ((int) (68 * s), 40 * s, (kW - 40) * s);

    auto well = [&] (const juce::Path& p)
    {
        g.setColour (juce::Colours::black.withAlpha (0.32f)); g.fillPath (p);
        g.setColour (juce::Colours::black.withAlpha (0.55f)); g.strokePath (p, juce::PathStrokeType (1.5f * s), juce::AffineTransform::translation (0, -1.0f * s));
        g.setColour (juce::Colours::white.withAlpha (0.06f)); g.strokePath (p, juce::PathStrokeType (1.0f * s), juce::AffineTransform::translation (0, 1.2f * s));
    };
    { juce::Path pod; pod.addRoundedRectangle (sc (26, 76, 388, 362).toFloat(), 40 * s); well (pod); }
    { juce::Path lens; lens.addRoundedRectangle (sc (426, 80, 428, 364).toFloat(), 120 * s); well (lens); }
    { juce::Path col2; col2.addRoundedRectangle (sc (872, 80, 196, 330).toFloat(), 36 * s); well (col2); }

    g.setColour (col::champ);
    g.setFont (font (12.0f * s, true, 0.55f));
    g.drawText ("EPSILON", sc (40, 22, 120, 36).toFloat(), juce::Justification::centredLeft);
    g.setColour (col::text);
    g.setFont (font (26.0f * s, true, 0.12f));
    g.drawText ("FET", sc (156, 20, 80, 40).toFloat(), juce::Justification::centredLeft);
    g.setFont (font (11.0f * s, true, 0.3f));
    g.drawText ("RATIO", sc (436, 440, 408, 16).toFloat(), juce::Justification::centred);
    g.drawText ("METER", sc (40, 428, 360, 16).toFloat(), juce::Justification::centred);

    const auto vb = sc (layout::vu).toFloat();
    juce::ColourGradient bloom (col::amber.withAlpha (0.10f), vb.getCentreX(), vb.getCentreY(), col::amber.withAlpha (0.0f), vb.getCentreX(), vb.getY() - 30 * s, true);
    g.setGradientFill (bloom); g.fillEllipse (vb.expanded (24 * s));
}

void EpsilonFetEditor::paint (juce::Graphics& g)
{
    if (plate.isNull()) renderPlate();
    if (xray) return;                                    // the X-ray view covers the window
    g.drawImage (plate, getLocalBounds().toFloat());
    g.setColour (col::faint);
    g.setFont (font (11.0f * s));
    const double latMs = proc.getRate() > 0 ? 1000.0 * proc.getLatencySamples() / proc.getRate() : 0.0;
    g.drawText ("v" JucePlugin_VersionString "   |   latency " + juce::String (latMs, 2) + " ms   |   double-click a knob to reset",
                sc (40, 468, 1000, 18), juce::Justification::centredRight);
   #if (defined (__clang__) || defined (__GNUC__)) && ! defined (__OPTIMIZE__)
    g.setColour (col::red);
    g.setFont (font (12.0f * s, true));
    g.drawText ("SLOW UNOPTIMISED BUILD", sc (40, 468, 1000, 18), juce::Justification::centredLeft);
   #endif
}

juce::Component* EpsilonFetEditor::findPart (const juce::String& id)
{
    if (id == "vu") return &vu;
    if (id == "circuit") return &circuit;
    if (id == "input") return &input;
    if (id == "output") return &output;
    if (id == "attack") return &attack;
    if (id == "release") return &release;
    if (id == "mix") return &mix;
    if (id == "link") return &linkBtn;
    if (id == "quality") return &qualityBtn;
    if (id == "power") return &bypassBtn;
    if (id == "xray") return &xrayBtn;
    if (id == "preset") return &presetBox;
    for (int i = 0; i < 5; ++i) if (id == "ratio" + juce::String (i)) return &ratio[i];
    for (int i = 0; i < 4; ++i) if (id == "meter" + juce::String (i)) return &meterBtn[i];
    return nullptr;
}
