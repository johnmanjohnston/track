#pragma once
#include <JuceHeader.h>

namespace track {
class clip {
  public:
    juce::String path;
    float fadeIn = 0.f;
    float fadeOut = 0.f;
};

class ClipComponent : public juce::Component {
  public:
    ClipComponent();
    ~ClipComponent();

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
};
} // namespace track
