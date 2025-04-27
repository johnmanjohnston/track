#include "../editor.h"
#include "../processor.h"
#include "BinaryData.h"
#include "track.h"
#include <JuceHeader.h>

namespace track {
class CloseButton : public juce::Component {
  public:
    CloseButton();
    ~CloseButton();

    juce::Font font;
    void paint(juce::Graphics &g) override;
    void mouseUp(const juce::MouseEvent &event) override;

    bool isHoveredOver = false;
    void mouseEnter(const juce::MouseEvent &event) override;
    void mouseExit(const juce::MouseEvent &event) override;
    juce::Colour normalColor = juce::Colour(0xFF'585858);
    juce::Colour hoveredColor = juce::Colour(0xFF'808080);
};

class PluginChainComponent : public juce::Component {
  public:
    PluginChainComponent();
    ~PluginChainComponent();

    CloseButton closeBtn;

    int titlebarHeight = -1;

    void mouseDown(const juce::MouseEvent &event) override;
    void mouseDrag(const juce::MouseEvent &event) override;
    juce::Rectangle<int> dragStartBounds;

    void paint(juce::Graphics &g) override;
    void resized() override;

    int trackIndex = -1;
    AudioPluginAudioProcessor *processor = nullptr;
    track *getCorrespondingTrack();

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

    CloseButton closeBtn;

    void timerCallback() override;

    void createEditor();
    void paint(juce::Graphics &g) override;
    void resized() override;

    void mouseUp(const juce::MouseEvent &event) override;
    void mouseDown(const juce::MouseEvent &event) override;
    void mouseDrag(const juce::MouseEvent &event) override;
    juce::Rectangle<int> dragStartBounds;

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
