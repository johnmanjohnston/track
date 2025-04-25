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

    juce::AudioProcessorEditor *ape = nullptr;
};
} // namespace track
