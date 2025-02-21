#include "track.h"
#include "timeline.h"

track::ClipComponent::ClipComponent(clip *c)
    : juce::Component(), thumbnailCache(5),
      thumbnail(512, afm, thumbnailCache) {
    if (c == nullptr) {
        DBG("JOHN WARNING: ClipComponent initialized with no clip provided");
        return;
    }

    this->correspondingClip = c;
    thumbnail.setSource(&correspondingClip->buffer, 41000.0, 2);
}
track::ClipComponent::~ClipComponent() {}

void track::ClipComponent::paint(juce::Graphics &g) {
    if (thumbnail.getNumChannels() == 0) {
        g.fillAll(juce::Colours::blue);
        g.setColour(juce::Colours::white);
        g.setFont(12.f);
        g.drawText("SAMPLE OFFLINE", getLocalBounds(),
                   juce::Justification::centred, true);
    }

    else {
        if (isBeingDragged) {
            g.fillAll(juce::Colours::blue);
        } else {
            g.setColour(juce::Colour(0xFF33587F));
            g.fillRoundedRectangle(getLocalBounds().toFloat(), 2.f);
        }

        g.setColour(juce::Colour(0xFFAECBED));
   
        int thumbnailTopMargin = 12;
        juce::Rectangle<int> thumbnailBounds = getLocalBounds();
        thumbnailBounds.setHeight(thumbnailBounds.getHeight() - thumbnailTopMargin);
        thumbnailBounds.setY(thumbnailBounds.getY() + thumbnailTopMargin);

        thumbnail.drawChannels(g, thumbnailBounds, 0,
                               thumbnail.getTotalLength(), .7f);
    }

    g.drawText(this->correspondingClip->name, 0, 0, getWidth(), 20, juce::Justification::left, true);
}

void track::ClipComponent::mouseDown(const juce::MouseEvent &event) {
    if (event.mods.isRightButtonDown()) {
        DBG("rmb down"); 

        juce::PopupMenu contextMenu;
        contextMenu.addItem(1, "Reverse");

        contextMenu.showMenuAsync(
            juce::PopupMenu::Options(), [this](int result) {
                if (result == 1) {
                    this->correspondingClip->reverse();
                    thumbnailCache.clear();
                    thumbnail.setSource(&correspondingClip->buffer, 44100.0, 2);
                    repaint();
                }      
            }
        );
    }
}

void track::ClipComponent::mouseDrag(const juce::MouseEvent &event) {
    // DBG("dragging is happening");
    isBeingDragged = true;
}

void track::ClipComponent::mouseUp(const juce::MouseEvent &event) {
    if (isBeingDragged) {
        DBG("DRAGGING STOPPED");
        isBeingDragged = false;

        int distanceMovedHorizontally = event.getDistanceFromDragStartX();

        this->correspondingClip->startPositionSample +=
            distanceMovedHorizontally * 1100;
    }

    DBG(event.getDistanceFromDragStartX());
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
            tv->setViewPosition(tv->getViewPositionX(), newRangeStart);
        }
    }
}

track::Tracklist::Tracklist() : juce::Component() {}
track::Tracklist::~Tracklist() {}
void track::Tracklist::createTrackComponents() {
    AudioPluginAudioProcessor *p =
        (AudioPluginAudioProcessor *)(this->processor);

    int counter = 0;
    for (track &t : p->tracks) {
        this->trackComponents.push_back(std::make_unique<TrackComponent>(&t));
        addAndMakeVisible(*trackComponents.back());

        auto &tc = trackComponents.back();
        tc->setBounds(10, 10 + (100 * counter), 100, 100);

        counter++;
        // DBG("added track component for track with name " << t.trackName);
    }

    repaint();
}

// TODO: update this function to take care of more advanced audio clip
// operations (like trimming, and offsetting)
void track::clip::updateBuffer() {
    juce::File file(path);

    if (!file.exists()) {
        DBG("updateBuffer() called--file " << path << " does not exist");
        return;
    }

    juce::AudioFormatManager afm;
    afm.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(afm.createReaderFor(file));
    buffer =
        juce::AudioBuffer<float>(reader->numChannels, reader->lengthInSamples);

    reader->read(&buffer, 0, buffer.getNumSamples(), 0, true, true);
}

void track::clip::reverse() {
    buffer.reverse(0, buffer.getNumSamples());
}
