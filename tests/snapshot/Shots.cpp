// Renders the editor and a close-up of every control, straight from the real window:
//   EpsilonShots <signal.wav> <out_dir> [preset]
// Writes full.png, one <part>.png per control, and parts.json (positions for the manual).
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <juce_audio_formats/juce_audio_formats.h>

static void save (const juce::Image& img, const juce::File& f)
{
    f.deleteFile();
    juce::FileOutputStream os (f);
    juce::PNGImageFormat().writeImageToStream (img, os);
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    if (argc < 3) { std::puts ("usage: EpsilonShots signal.wav out_dir [preset]"); return 2; }
    EpsilonFetProcessor proc;
    proc.setPlayConfigDetails (2, 2, 48000.0, 512);
    proc.setNonRealtime (true);
    if (argc > 3) proc.loadPreset (juce::String (argv[3]).getIntValue());
    proc.prepareToPlay (48000.0, 512);

    juce::AudioFormatManager afm; afm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> rd (afm.createReaderFor (juce::File (juce::String (argv[1]))));
    const int n = (int) std::min<juce::int64> (rd->lengthInSamples, 48000 * 3);
    juce::AudioBuffer<float> buf (2, n); rd->read (&buf, 0, n, 0, true, true);
    juce::MidiBuffer midi;
    std::unique_ptr<EpsilonFetEditor> ed (static_cast<EpsilonFetEditor*> (proc.createEditor()));
    for (int s = 0; s + 512 <= n; s += 512)
    {
        juce::AudioBuffer<float> v (buf.getArrayOfWritePointers(), 2, s, 512);
        proc.processBlock (v, midi);
        ed->tickForSnapshot (1);                   // ~one GUI frame per block: meter ballistics settle
    }
    const float scale = 2.0f;
    const juce::File out { juce::String (argv[2]) };
    out.createDirectory();
    const auto full = ed->createComponentSnapshot (ed->getLocalBounds(), true, scale);
    save (full, out.getChildFile ("full.png"));

    auto crop = [&] (juce::Rectangle<int> r, int pad, const juce::String& name)
    {
        r = r.expanded (pad).getIntersection (ed->getLocalBounds());
        save (full.getClippedImage ((r.toFloat() * scale).toNearestInt()), out.getChildFile (name + ".png"));
    };
    auto b = [&] (const juce::String& id) { return ed->findPart (id)->getBoundsInParent(); };

    juce::DynamicObject::Ptr json = new juce::DynamicObject();
    const juce::StringArray ids { "vu", "circuit", "input", "output", "attack", "release", "mix", "link", "quality", "power", "xray", "preset",
                                  "ratio0", "ratio1", "ratio2", "ratio3", "ratio4", "meter0", "meter1", "meter2", "meter3" };
    for (auto& id : ids)
    {
        const auto r = b (id);
        json->setProperty (id, juce::Array<juce::var> { r.getX(), r.getY(), r.getWidth(), r.getHeight() });
    }
    json->setProperty ("width", ed->getWidth()); json->setProperty ("height", ed->getHeight());
    out.getChildFile ("parts.json").replaceWithText (juce::JSON::toString (juce::var (json.get())));

    for (auto* id : { "input", "output", "attack", "release", "mix", "vu", "link", "quality", "power", "xray", "preset" })
        crop (b (id), juce::String (id) == "vu" ? 0 : 4, id);
    crop (b ("ratio0").getUnion (b ("ratio4")), 6, "ratio");
    crop (b ("meter0").getUnion (b ("meter3")), 6, "meterbuttons");
    // X-ray view: the live circuit behind the controls
    ed->setXray (true);
    for (int s2 = 0; s2 + 512 <= n; s2 += 512)
    {
        juce::AudioBuffer<float> v (buf.getArrayOfWritePointers(), 2, s2, 512);
        proc.processBlock (v, midi);
        ed->tickForSnapshot (1);
    }
    save (ed->createComponentSnapshot (ed->getLocalBounds(), true, scale), out.getChildFile ("full_xray.png"));
    ed->setXray (false);
    std::puts ("shots written");
    return 0;
}
