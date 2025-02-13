#pragma once
#include "track.h"
#include "../processor.h"
#include <JuceHeader.h>

namespace track {
class TimelineViewport : public juce::Viewport {
  public:
    TimelineViewport();
    ~TimelineViewport();

    void scrollBarMoved(juce::ScrollBar *bar, double newRangeStart) override;
    TrackViewport *trackViewport = nullptr;
};

class TimelineComponent : public juce::Component {
  public:
    TimelineComponent();
    ~TimelineComponent();

    void paint(juce::Graphics &g) override;

    TimelineViewport *viewport = nullptr;
    AudioPluginAudioProcessor *processorRef = nullptr;

   // std::vector<ClipComponent*> clipComponents;
    std::vector<std::unique_ptr<ClipComponent>> clipComponents;
    void updateClipComponents();

    bool clipComponentsUpdated = false;

    // std::vector<ClipComponent> clipComponents;

    /*
    //forum.juce.com/t/creating-arrays-vectors-of-components-copy-move-semantics/47054/5
    std::vector<std::unique_ptr<TrackComponent>> trackComponents;
    */

  private:
    int zoomLevel = 1;
};

} // namespace track
