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
    void *p = nullptr;

    ActionAddClip(clip c, std::vector<int> nodeRoute, void *processor);
    ~ActionAddClip();

    bool perform() override;
    bool undo() override;
    void updateGUI(); // y
};

class ActionCutClip : public juce::UndoableAction {
  public:
    clip addedClip;
    std::vector<int> route;
    void *p = nullptr;

    ActionCutClip(clip c, std::vector<int> nodeRoute, void *processor);
    ~ActionCutClip();

    bool perform() override;
    bool undo() override;
    void updateGUI(); // y

  private:
    int clipIndex = -1;
};

class ActionSplitClip : public juce::UndoableAction {
  public:
    clip clipCopy;
    std::vector<int> route;
    void *p = nullptr;

    int splitSample = -1;

    ActionSplitClip(clip c, std::vector<int> nodeRoute, int sampleToSplit,
                    void *processor, bool updateUI);
    ~ActionSplitClip();

    bool perform() override;
    bool undo() override;
    void updateGUI(); // y

    bool shouldUpdateGUI = true;

  private:
    int c1Index = -1;
};

class ActionShiftClips : public juce::UndoableAction {
  public:
    ActionShiftClips(int amount, void *processor);
    ~ActionShiftClips();

    int shiftAmount = -1;
    void *p;

    bool perform() override;
    bool undo() override;

    void shift(int bars);
    void updateGUI(); // y
};

struct SplitMultipleClipsData {
    std::vector<int> route;
    int nodeDisplayIndex = -1;
    int clipIndex = -1;
};

class TimelineComponent : public juce::Component,
                          public juce::FileDragAndDropTarget {
  public:
    TimelineComponent();
    ~TimelineComponent();

    bool isInterestedInFileDrag(const juce::StringArray &files) override;
    void mouseDown(const juce::MouseEvent &event) override;
    void filesDropped(const juce::StringArray &files, int x, int y) override;
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
