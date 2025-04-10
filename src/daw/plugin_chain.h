#include "../processor.h"
#include "track.h"
#include <JuceHeader.h>

namespace track {
class PluginChainComponent : public juce::Component {
  public:
    PluginChainComponent();
    ~PluginChainComponent();

    void mouseDown(const juce::MouseEvent &event) override;
    void mouseDrag(const juce::MouseEvent &event) override;
    juce::Rectangle<int> dragStartBounds;

    void paint(juce::Graphics &g) override;

    int trackIndex = -1;
    AudioPluginAudioProcessor *processor = nullptr;
    track *getCorrespondingTrack();
};
} // namespace track
