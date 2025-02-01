#include "track.h"
#include "timeline.h"
track::ClipComponent::ClipComponent(clip *c) : juce::Component(), thumbnailCache(5), thumbnail(512, afm, thumbnailCache) {
    if (c == nullptr) {
        DBG("JOHN WARNING: ClipComponent initialized with no clip provided");
        return;
    }

    this->correspondingClip = c;
}
track::ClipComponent::~ClipComponent() {}

void track::ClipComponent::paint(juce::Graphics& g) {
    if (thumbnail.getNumChannels() == 0) {
        g.fillAll(juce::Colours::blue);
        g.setColour(juce::Colours::white);
        g.setFont(12.f);
        g.drawText("SAMPLE OFFLINE", getLocalBounds(),
                   juce::Justification::centred, true);
    } 

    else {
        thumbnail.drawChannels(g, getLocalBounds(), 0,
                               thumbnail.getTotalLength(), 1.f);  
    }
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


// TODO: update this function to take care of more advanced audio clip operations (like trimming, and offsetting)
void track::clip::updateBuffer() { 
    juce::File file(path);
    juce::AudioFormatManager afm;
    afm.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(afm.createReaderFor(file));
    buffer =
        juce::AudioBuffer<float>(reader->numChannels, reader->lengthInSamples);

    reader->read(&buffer, 0, buffer.getNumSamples(), 0, true, true);
}
