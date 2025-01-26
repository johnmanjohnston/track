#include <JuceHeader.h>

namespace track {
class TimelineComponent : public juce::Component {
  public:
    TimelineComponent();
    ~TimelineComponent();

    void paint(juce::Graphics &g) override;
};
} // namespace track
