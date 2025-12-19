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

class BarNumbersComponent : public juce::Component {
  public:
    BarNumbersComponent();
    ~BarNumbersComponent();

    void paint(juce::Graphics &g);

    juce::Font getInterRegular() {
        static auto typeface = Typeface::createSystemTypefaceFor(
            BinaryData::Inter_18ptRegular_ttf,
            BinaryData::Inter_18ptRegular_ttfSize);

        return Font(typeface);
    }
};

class ActionAddClip : public juce::UndoableAction {
  public:
    clip addedClip;
    std::vector<int> route;
    void *tc = nullptr;

    ActionAddClip(clip c, std::vector<int> nodeRoute, void *timelineComponent);
    ~ActionAddClip();

    bool perform() override;
    bool undo() override;
    void updateGUI();
};

class ActionCutClip : public juce::UndoableAction {
  public:
    clip addedClip;
    std::vector<int> route;
    void *tc = nullptr;

    ActionCutClip(clip c, std::vector<int> nodeRoute, void *timelineComponent);
    ~ActionCutClip();

    bool perform() override;
    bool undo() override;
    void updateGUI();

  private:
    int clipIndex = -1;
};

class ActionSplitClip : public juce::UndoableAction {
  public:
    clip clipCopy;
    std::vector<int> route;
    void *tc = nullptr;

    int splitSample = -1;

    ActionSplitClip(clip c, std::vector<int> nodeRoute, int sampleToSplit,
                    void *timelineComponent);
    ~ActionSplitClip();

    bool perform() override;
    bool undo() override;
    void updateGUI();

  private:
    int c1Index = -1;
};

class ActionShiftClips : public juce::UndoableAction {
  public:
    ActionShiftClips(int amount, void *timelineComponent);
    ~ActionShiftClips();

    int shiftAmount = -1;
    void *tc;

    bool perform() override;
    bool undo() override;

    void shift(int bars);
    void updateGUI();
};

class TimelineComponent : public juce::Component,
                          public juce::FileDragAndDropTarget {
  public:
    TimelineComponent();
    ~TimelineComponent();

    bool isInterestedInFileDrag(const juce::StringArray &files) override;
    void mouseDown(const juce::MouseEvent &event) override;
    void filesDropped(const juce::StringArray &files, int x, int y) override;
    void handleClipResampling(int modalResult);
    void addNewClipToTimeline(juce::String path, int startSample,
                              int nodeDisplayIndex);

    void paint(juce::Graphics &g) override;
    void resizeClipComponent(ClipComponent *clip);
    void resized() override;
    bool renderingWaveforms();

    TimelineViewport *viewport = nullptr;
    AudioPluginAudioProcessor *processorRef = nullptr;

    std::vector<std::unique_ptr<ClipComponent>> clipComponents;
    void updateClipComponents();
    void updateOnlyStaleClipComponents();
    void resizeTimelineComponent();

    BarNumbersComponent barNumbers;

    bool clipComponentsUpdated = false;
    void deleteClip(clip *c, int trackIndex);
    void splitClip(clip *c, int splitSample, int nodeDisplayIndex);
    void shiftClipByBars(int bars);

    juce::Font getInterRegular() {
        static auto typeface = Typeface::createSystemTypefaceFor(
            BinaryData::Inter_18ptRegular_ttf,
            BinaryData::Inter_18ptRegular_ttfSize);

        return Font(typeface);
    }
};

} // namespace track
