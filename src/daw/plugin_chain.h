#include "../editor.h"
#include "../processor.h"
#include "BinaryData.h"
#include "track.h"
#include <JuceHeader.h>

namespace track {
class PluginChainComponent : public juce::Component {
  public:
    PluginChainComponent();
    ~PluginChainComponent();

    int titlebarHeight = -1;

    void mouseDown(const juce::MouseEvent &event) override;
    void mouseDrag(const juce::MouseEvent &event) override;
    juce::Rectangle<int> dragStartBounds;

    void paint(juce::Graphics &g) override;
    void resized() override;

    int trackIndex = -1;
    AudioPluginAudioProcessor *processor = nullptr;
    track *getCorrespondingTrack();

    juce::TextButton closeBtn;

    juce::TextButton addPluginBtn;

    // TODO: this shouldn't belong in this class
    static const juce::Font getInterBoldItalic() {
        static auto typeface = Typeface::createSystemTypefaceFor(
            BinaryData::Inter_18ptBoldItalic_ttf,
            BinaryData::Inter_18ptBoldItalic_ttfSize);

        return typeface;
    }

    // juce::AudioProcessorEditor *ape = nullptr;
};

class PluginEditorWindow : public juce::Component, juce::Timer {
  public:
    PluginEditorWindow();
    ~PluginEditorWindow();

    void timerCallback() override;

    void createEditor();
    void paint(juce::Graphics &g) override;
    void resized() override;

    int trackIndex = -1;
    int pluginIndex = -1;

    // info to show on titlebar
    juce::String pluginName;
    juce::String pluginManufacturer;
    juce::String trackName;

    // juce::AudioProcessorEditor *ape = nullptr;
    std::unique_ptr<juce::AudioProcessorEditor> ape;

    AudioPluginAudioProcessor *processor = nullptr;
    track *getCorrespondingTrack();
    std::unique_ptr<juce::AudioPluginInstance> *getPlugin();

    // TODO: this shouldn't belong in this class
    static const juce::Font getInterBoldItalic() {
        static auto typeface = Typeface::createSystemTypefaceFor(
            BinaryData::Inter_18ptBoldItalic_ttf,
            BinaryData::Inter_18ptBoldItalic_ttfSize);

        return typeface;
    }
};
} // namespace track
