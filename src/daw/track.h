#pragma once
#include "BinaryData.h"
#include <JuceHeader.h>

namespace track {
class clip {
  public:
    juce::String path;
    juce::String name = "untitled clip";

    int startPositionSample = -1; // absolute sample position within host
    int endPositionSample =
        -1; // relative to audio file start, irrespective of looping
    int endPositionSampleWithLooping; // absolute sample position within host
    bool isLooping;
    bool active = true;

    juce::AudioBuffer<float> buffer; // irrespective of looping
    void updateBuffer();
    void reverse();
};

class ClipComponent : public juce::Component, public juce::ChangeListener {
  public:
    ClipComponent(clip *c);
    ~ClipComponent();

    juce::AudioThumbnailCache thumbnailCache;
    juce::AudioThumbnail thumbnail;
    juce::AudioFormatManager afm;
    void changeListenerCallback(ChangeBroadcaster *source) override;

    clip *correspondingClip = nullptr;

    void paint(juce::Graphics &g) override;
    int nodeDisplayIndex = -1;

    // moving clips
    void mouseDown(const juce::MouseEvent &event) override;
    void mouseDrag(const juce::MouseEvent &event) override;
    void mouseUp(const juce::MouseEvent &event) override;

    void mouseEnter(const juce::MouseEvent &) override { repaint(); }
    void mouseExit(const juce::MouseEvent &) override { repaint(); }

    bool isBeingDragged = false;
    int startDragX = -1;
    int startDragStartPositionSample = -1;

    // TODO: this function should not be in this class
    juce::Font getInterSemiBold() {
        static auto typeface = Typeface::createSystemTypefaceFor(
            BinaryData::Inter_18ptSemiBold_ttf,
            BinaryData::Inter_18ptSemiBold_ttfSize);

        return Font(typeface);
    }

    juce::Label clipNameLabel;
};

class audioNode {
  public:
    bool s = false;
    bool m = false;
    float gain = 1.f;
    float pan = 0.f;

    juce::String trackName = "Untitled Node";

    // future john, have fun trying to implement hosting audio plugins :skull:
    std::vector<std::unique_ptr<juce::AudioPluginInstance>> plugins;
    void addPlugin(juce::String path);
    void preparePlugins(int newMaxSamplesPerBlock, int newSampleRate);
    int maxSamplesPerBlock = -1;
    int sampleRate = -1;

    void process(int numSamples, int currentSample);
    juce::AudioBuffer<float> buffer;

    void *processor = nullptr;

    bool isTrack = true;
    std::vector<clip> clips;
    std::vector<audioNode> childNodes;
};

class group {
  public:
    bool s = false;
    bool m = false;

    // void process(juce::AudioBuffer<float> buffer, int currentSample);
    // juce::AudioBuffer<float> internalBuffer;
};

class TrackComponent : public juce::Component {
  public:
    // TrackComponent(track *t);
    TrackComponent(int trackIndex);
    ~TrackComponent();

    void paint(juce::Graphics &g) override;
    void resized() override;
    void initializeGainSlider();

    void mouseDown(const juce::MouseEvent &event) override;
    void mouseUp(const juce::MouseEvent &event) override;

    // instances of TrackComponent are responsible for only handling the UI for
    // an indiviaul track (only the left section which shows track name, volume
    // controls, mute/soloing control). The actual CLIPS of audio will be
    // handles in the TimelineComponent class
    // track *correspondingTrack = nullptr;
    audioNode *getCorrespondingTrack();
    void *processor = nullptr;
    int siblingIndex = -1;
    std::vector<int> route;

    juce::TextButton muteBtn;
    juce::TextButton fxBtn; // like in REAPER
    juce::Slider gainSlider;
    juce::Slider panSlider;

    juce::Font getAudioNodeLabelFont() {
        static auto typeface = Typeface::createSystemTypefaceFor(
            BinaryData::Inter_18ptRegular_ttf,
            BinaryData::Inter_18ptRegular_ttfSize);

        return Font(typeface);
    }

    juce::Label trackNameLabel;
};

class Tracklist : public juce::Component {
  public:
    Tracklist();
    ~Tracklist();

    std::vector<std::unique_ptr<TrackComponent>> trackComponents;
    int findChildren(audioNode *parentNode, std::vector<int> route,
                     int foundItems, int depth);
    juce::String getPrettyVector(std::vector<int> v) {
        juce::String retval = "[";
        for (int x : v) {
            retval.append(juce::String(x), 2);
            retval.append(",", 2);
        }
        retval.append("]", 1);
        return retval;
    }
    void createTrackComponents();
    void setTrackComponentBounds();

    void mouseDown(const juce::MouseEvent &event) override;

    void addNewNode(bool isTrack = true);
    void deleteTrack(int trackIndex);
    void moveNodeToGroup(track::TrackComponent *caller,
                         int relativeDisplayNodesToMove);
    void deepCopyGroupInsideGroup(audioNode *childNode, audioNode *parentNode);

    void *processor = nullptr;
    void *timelineComponent = nullptr;

    juce::TextButton newTrackBtn;
    juce::TextButton newGroupBtn;
};

class TrackViewport : public juce::Viewport {
  public:
    TrackViewport();
    ~TrackViewport();

    void scrollBarMoved(juce::ScrollBar *bar, double newRangeStart) override;

    void *timelineViewport = nullptr;
};
} // namespace track
