#include "track.h"
#include "timeline.h"
track::ClipComponent::ClipComponent(clip *c) : juce::Component() {
    if (c == nullptr) {
        DBG("JOHN WARNING: ClipComponent initialized with no clip provided");
        return;
    }

    this->correspondingClip = c;
}
track::ClipComponent::~ClipComponent() {}

void track::ClipComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::blue);
}

track::TrackComponent::TrackComponent(track *t) : juce::Component() {
    if (t == nullptr) {
        DBG("JOHN WARNING: TrackComponent initialized with no track provided");
        return;
    }

    this->correspondingTrack = t;
}
track::TrackComponent::~TrackComponent() {}

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

void track::TrackViewport::scrollBarMoved(juce::ScrollBar *bar,
                                     double newRangeStart) {
    if (bar->isVertical()) {
        if (timelineViewport != nullptr) {
            TimelineViewport *tv = (TimelineViewport *)(timelineViewport);
            tv->setViewPosition(tv->getViewPositionX(),
                newRangeStart);
        } 
    }
}

track::Tracklist::Tracklist() : juce::Component() {}
track::Tracklist::~Tracklist() {}
