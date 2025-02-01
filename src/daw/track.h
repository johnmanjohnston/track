#pragma once
#include <JuceHeader.h>

namespace track {
class clip {
  public:
    juce::String path;

    float fadeIn = 0.f;
    float fadeOut = 0.f;

    int startPositionSample = -1;
    int endPositionSample = -1; // irrespective of looping
    int endPositionSampleWithLooping;
    bool isLooping;
};

class ClipComponent : public juce::Component {
  public:
    ClipComponent(clip *c);
    ~ClipComponent();

    clip *correspondingClip = nullptr;

    void paint(juce::Graphics &g) override;
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
};

class TrackViewport : public juce::Viewport {
  public:
    TrackViewport();
    ~TrackViewport();

    void scrollBarMoved(juce::ScrollBar *bar, double newRangeStart) override;

    void *timelineViewport = nullptr;
};
} // namespace track
