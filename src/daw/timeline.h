#pragma once
#include "../processor.h"
#include "track.h"
#include <JuceHeader.h>

namespace track {
class TimelineViewport : public juce::Viewport {
  public:
    TimelineViewport();
    ~TimelineViewport();

    void
    mouseWheelMove(const juce::MouseEvent &ev,
                   const juce::MouseWheelDetails &mouseWheelDetails) override;
    void scrollBarMoved(juce::ScrollBar *bar, double newRangeStart) override;
    TrackViewport *trackViewport = nullptr;
    Tracklist *tracklist = nullptr;
};

class TimelineComponent : public juce::Component,
                          public juce::FileDragAndDropTarget {
  public:
    TimelineComponent();
    ~TimelineComponent();

    bool isInterestedInFileDrag(const juce::StringArray &files) override;
    void filesDropped(const juce::StringArray &files, int x, int y) override;
    void mouseDown(const juce::MouseEvent &event) override;

    void paint(juce::Graphics &g) override;
    void resizeClipComponent(ClipComponent *clip);
    void resized() override;

    TimelineViewport *viewport = nullptr;
    AudioPluginAudioProcessor *processorRef = nullptr;

    // std::vector<ClipComponent*> clipComponents;
    std::vector<std::unique_ptr<ClipComponent>> clipComponents;
    void updateClipComponents();
    void resizeTimelineComponent();

    bool clipComponentsUpdated = false;
    void deleteClip(clip *c, int trackIndex);
    void splitClip(clip *c, int splitSample, int nodeDisplayIndex);

    /*
    //forum.juce.com/t/creating-arrays-vectors-of-components-copy-move-semantics/47054/5
    std::vector<std::unique_ptr<TrackComponent>> trackComponents;
    */

    // TODO: this function should not be in this class
    juce::Font getInterRegular() {
        static auto typeface = Typeface::createSystemTypefaceFor(
            BinaryData::Inter_18ptRegular_ttf,
            BinaryData::Inter_18ptRegular_ttfSize);

        return Font(typeface);
    }

  private:
    int zoomLevel = 1;
};

} // namespace track
