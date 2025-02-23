#include "timeline.h"
#include "defs.h"
#include "juce_gui_basics/juce_gui_basics.h"
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

void track::TimelineViewport::mouseWheelMove(
    const juce::MouseEvent &ev,
    const juce::MouseWheelDetails &mouseWheelDetails) {

    if (juce::ModifierKeys::currentModifiers.isAltDown()) {
        if (mouseWheelDetails.deltaY < 0) {
            UI_TRACK_HEIGHT -= 5;
        } else if (mouseWheelDetails.deltaY > 0) {
            UI_TRACK_HEIGHT += 5;
        }

        tracklist->setTrackComponentBounds();
        trackViewport->repaint();
    }

    if (ev.eventComponent == this)
        if (!useMouseWheelMoveIfNeeded(ev, mouseWheelDetails))
            Component::mouseWheelMove(ev, mouseWheelDetails);
}

void track::TimelineComponent::paint(juce::Graphics &g) {
    g.fillAll(juce::Colour(0xFF2E2E2E));

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
    for (auto &&clip : this->clipComponents) {
        clip->setBounds(
            clip->correspondingClip->startPositionSample / 41000.0 * 32.0,
            UI_TRACK_VERTICAL_OFFSET + (clip->trackIndex * UI_TRACK_HEIGHT) +
                (UI_TRACK_VERTICAL_MARGIN * clip->trackIndex),
            clip->correspondingClip->buffer.getNumSamples() / 41000.0 * 32.0,
            UI_TRACK_HEIGHT);
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

void track::TimelineComponent::deleteClip(clip *c, int trackIndex) {
    int clipIndex = 0;

    for (auto &clip : processorRef->tracks[trackIndex].clips) {
        if (clip.startPositionSample == c->startPositionSample) {
            break;
        }
        ++clipIndex;
    }

    DBG("clipIndex is " << clipIndex << "; trackIndex is " << trackIndex);

    if (trackIndex != 0) {
        processorRef->tracks[trackIndex].clips.erase(
            processorRef->tracks[trackIndex].clips.begin() + clipIndex);
    } else {
        // for some reason when removing the leftmost clip of the first track,
        // the program crashes; so instead of removing clips on the first track,
        // just get rid of its data and move it to the left so that we cannot
        // see it
        processorRef->tracks[trackIndex].clips[clipIndex].buffer.setSize(0, 1);
        processorRef->tracks[trackIndex].clips[clipIndex].startPositionSample =
            -10;
    }

    updateClipComponents();
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

    // remove directories leading up to the actual file name we want, and strip
    // file extension
    if (files[0].contains("/")) {
        c->name = files[0].fromLastOccurrenceOf(
            "/", false, true); // for REAL operating systems.
    } else {
        c->name = files[0].fromLastOccurrenceOf("\\", false, true);
    }
    c->name = c->name.upToLastOccurrenceOf(".", false, true);

    c->startPositionSample = x * 32 * 40;
    c->updateBuffer();

    processorRef->tracks[trackIndex].clips.push_back(*c);

    updateClipComponents();
}
