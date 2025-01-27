#pragma once
#include <JuceHeader.h>

namespace track {
class TimelineComponent : public juce::Component {
  public:
    TimelineComponent();
    ~TimelineComponent();

    void paint(juce::Graphics &g) override;
};

class TimelineViewport : public juce::Viewport {
  public:
    TimelineViewport();
    ~TimelineViewport();
};
} // namespace track
