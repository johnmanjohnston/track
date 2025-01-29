#include "track.h"
track::ClipComponent::ClipComponent() : juce::Component() {}
track::ClipComponent::~ClipComponent() {}

track::TrackComponent::TrackComponent(track *t) : juce::Component() {
    if (t == nullptr) {
        DBG("JOHN WARNING: TrackComponent initialized with no track provided");
        return;
    }

    this->correspondingTrack = t;
}
track::TrackComponent::~TrackComponent() {}

void track::ClipComponent::paint(juce::Graphics &g) {
    g.fillAll(juce::Colours::red);
    g.fillAll();
    // DBG("ClipComponent::paint() called");
}

void track::TrackComponent::paint(juce::Graphics &g) {
    g.fillAll(juce::Colours::green);

    juce::String trackName = correspondingTrack == nullptr
                                 ? "null track"
                                 : correspondingTrack->trackName;

    g.drawText(trackName, getLocalBounds(), juce::Justification::left, true);
}

// ASJAJSAJSJAS
track::TrackViewport::TrackViewport() : juce::Viewport() {}
track::TrackViewport::~TrackViewport(){};

track::Tracklist::Tracklist() : juce::Component() {}
track::Tracklist::~Tracklist() {}
