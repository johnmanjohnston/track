#include "../processor.h"
#include <JuceHeader.h>

namespace track {
class TransportStatusComponent : public juce::Component {
  public:
    TransportStatusComponent();
    ~TransportStatusComponent();

    void paint(juce::Graphics &g) override;

    AudioPluginAudioProcessor *processorRef = nullptr;
};
} // namespace track
