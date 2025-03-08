#include "timeline.h"
#include <JuceHeader.h>

namespace track {
class PlayheadComponent : public juce::Component {
  public:
    PlayheadComponent();
    ~PlayheadComponent();

    TimelineViewport *tv = nullptr;
    AudioPluginAudioProcessor *processor = nullptr;

    void updateBounds();
    void paint(juce::Graphics &g) override;
};
} // namespace track
