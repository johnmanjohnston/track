#pragma once
#include <JuceHeader.h>

namespace track {
class TimelineViewport : public juce::Viewport {
  public:
    TimelineViewport();
    ~TimelineViewport();
};

class TimelineComponent : public juce::Component {
  public:
    TimelineComponent();
    ~TimelineComponent();

    void paint(juce::Graphics &g) override;

    TimelineViewport *viewport = nullptr;

  private:
    int zoomLevel = 1;
};

} // namespace track
