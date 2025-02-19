#include "timeline.h"
#include <memory>

// TODO: organize this file

track::TimelineComponent::TimelineComponent() : juce::Component() {
    setSize(9000, 2000);

    /*
    track *t = new track();
    trackComponents.emplace_back(new TrackComponent(t));

    addAndMakeVisible(*trackComponents[0]);
    trackComponents[0]->setBounds(0, 50, 300, 50);
    */
};
track::TimelineComponent::~TimelineComponent(){};

track::TimelineViewport::TimelineViewport() : juce::Viewport() {}
track::TimelineViewport::~TimelineViewport() {}

void track::TimelineViewport::scrollBarMoved(juce::ScrollBar *bar,
                                             double newRangeStart) {
    if (bar->isVertical()) {
        // update tracklist scroll
        if (trackViewport != nullptr) {
            trackViewport->setViewPosition(0, newRangeStart);
        }

        setViewPosition(getViewPositionX(), newRangeStart);
    } else {
        setViewPosition(newRangeStart, getViewPositionY());
    }
}

void track::TimelineComponent::paint(juce::Graphics &g) {
    g.fillAll(juce::Colours::black);

    g.setColour(juce::Colours::white);
    for (int i = 0; i < (getBounds().getWidth() / 100); ++i) {
        int scrollValue =
            viewport->getVerticalScrollBar().getCurrentRangeStart();

        juce::Rectangle<int> newBounds = getLocalBounds();
        newBounds.setX(newBounds.getX() + (i * (zoomLevel * 100)));
        newBounds.setY(newBounds.getY() - (newBounds.getHeight() / 2.f) + 10 +
                       scrollValue);

        g.drawText(juce::String(i), newBounds, juce::Justification::left,
                   false);
    }

    g.drawText(
        juce::String(processorRef->tracks[0].clips[0].buffer.getNumChannels()),
        10, 10, 50, 20, juce::Justification::left, false);

    if (clipComponentsUpdated == false)
        updateClipComponents();

    // draw clips
    int trackHeight = 128;
    for (auto &&clip : this->clipComponents) {
        clip->setBounds(
            clip->correspondingClip->startPositionSample / 41000.0 * 32.0,
            clip->trackIndex * trackHeight,
            clip->correspondingClip->buffer.getNumSamples() / 41000.0 * 32.0,
            trackHeight);
    }

    // draw playhead
    // DBG("paint() for timeline component called");
    g.setColour(juce::Colours::white);
    if (processorRef->getPlayHead() != nullptr &&
        processorRef->getPlayHead()->getPosition()->getBpm().hasValue()) {
        int curSample =
            *processorRef->getPlayHead()->getPosition()->getTimeInSamples();

        g.drawRect(curSample / 41000.0 * 32.0, 0, 2, getHeight(), 2);
    }
}

void track::TimelineComponent::updateClipComponents() {
    clipComponentsUpdated = true;

    clipComponents.clear();

    for (std::vector<track>::size_type i = 0; i < processorRef->tracks.size();
         ++i) {
        for (auto &c : processorRef->tracks[i].clips) {

            /*
            ClipComponent *cc = new ClipComponent(&c);
            cc->trackIndex = i;
            this->clipComponents.push_back(std::unique_ptr<ClipComponent>(cc));

            addAndMakeVisible(cc);
            */

            this->clipComponents.push_back(std::make_unique<ClipComponent>(&c));
            addAndMakeVisible(*clipComponents.back());

            auto &cc = clipComponents.back();
            cc->trackIndex = i;
        }
    }
}

bool track::TimelineComponent::isInterestedInFileDrag(
    const juce::StringArray &files) {
    return true;
}

void track::TimelineComponent::filesDropped(const juce::StringArray &files,
                                            int x, int y) {
    // DBG("files dropped; x: " << x << "; y: " << y);

    int trackIndex = y / 130;
    DBG("track index is " << trackIndex);

    std::unique_ptr<clip> c(new clip());
    c->path = files[0];
    c->startPositionSample = x * 32 * 40;
    c->updateBuffer();

    processorRef->tracks[trackIndex].clips.push_back(*c);

    updateClipComponents();
}
