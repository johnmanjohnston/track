#pragma once
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

    juce::AudioBuffer<float> buffer; // irrespective of looping
    void updateBuffer();
    void reverse();
};

class ClipComponent : public juce::Component {
  public:
    ClipComponent(clip *c);
    ~ClipComponent();

    juce::AudioThumbnailCache thumbnailCache;
    juce::AudioThumbnail thumbnail;
    juce::AudioFormatManager afm;

    clip *correspondingClip = nullptr;

    void paint(juce::Graphics &g) override;
    int trackIndex = -1;

    // moving clips
    void mouseDown(const juce::MouseEvent &event) override;
    void mouseDrag(const juce::MouseEvent &event) override;
    void mouseUp(const juce::MouseEvent &event) override;
    bool isBeingDragged = false;
};

class track {
  public:
    bool s = false;
    bool m = false;

    juce::String trackName = "Untitled Track";

    // future john, have fun trying to implement hosting audio plugins :skull:

    std::vector<clip> clips;
};

class TrackComponent : public juce::Component {
  public:
    TrackComponent(track *t);
    ~TrackComponent();

    void paint(juce::Graphics &g);

    // instances of TrackComponent are responsible for only handling the UI for
    // an indiviaul track (only the left section which shows track name, volume
    // controls, mute/soloing control). The actual CLIPS of audio will be
    // handles in the TimelineComponent class
    track *correspondingTrack = nullptr;
};

class Tracklist : public juce::Component {
  public:
    Tracklist();
    ~Tracklist();

    std::vector<std::unique_ptr<TrackComponent>> trackComponents;
    void createTrackComponents();
    void *processor = nullptr;
};

class TrackViewport : public juce::Viewport {
  public:
    TrackViewport();
    ~TrackViewport();

    void scrollBarMoved(juce::ScrollBar *bar, double newRangeStart) override;

    void *timelineViewport = nullptr;
};
} // namespace track
