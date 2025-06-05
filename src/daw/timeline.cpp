#include "timeline.h"
#include "clipboard.h"
#include "defs.h"
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_graphics/native/juce_EventTracing.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "track.h"
#include <memory>

// TODO: organize this file

track::TimelineComponent::TimelineComponent() : juce::Component(){};
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

    if (juce::ModifierKeys::currentModifiers.isCtrlDown() ||
        juce::ModifierKeys::currentModifiers.isCommandDown()) {

        TimelineComponent *tc = (TimelineComponent *)getViewedComponent();
        int orginalWidth = tc->getWidth();
        int x = getViewPositionX();
        float ratio = (float)orginalWidth / (float)x;

        if (mouseWheelDetails.deltaY < 0 &&
            UI_ZOOM_MULTIPLIER > UI_MINIMUM_ZOOM_MULTIPLIER) {
            UI_ZOOM_MULTIPLIER -= 2;
        } else if (mouseWheelDetails.deltaY > 0 &&
                   UI_ZOOM_MULTIPLIER < UI_MAXIMUM_ZOOM_MULTIPLIER) {
            UI_ZOOM_MULTIPLIER += 2;
        }

        tc->resizeTimelineComponent();

        int newWidth = tc->getWidth();
        int newX = (int)((float)newWidth / (float)ratio);
        setViewPosition(newX, getViewPositionY());

        repaint();
    }

    if (juce::ModifierKeys::currentModifiers.isAltDown()) {
        if (mouseWheelDetails.deltaY < 0 &&
            UI_TRACK_HEIGHT > UI_MINIMUM_TRACK_HEIGHT) {
            UI_TRACK_HEIGHT -= 5;
        } else if (mouseWheelDetails.deltaY > 0 &&
                   UI_TRACK_HEIGHT < UI_MAXIMUM_TRACK_HEIGHT) {
            UI_TRACK_HEIGHT += 5;
        }

        tracklist->setTrackComponentBounds();
        trackViewport->repaint();

        TimelineComponent *timelineComponent =
            (TimelineComponent *)getViewedComponent();
        int tcHeight =
            (tracklist->trackComponents.size() + 2) * (size_t)UI_TRACK_HEIGHT;
        timelineComponent->setSize(timelineComponent->getWidth(),
                                   juce::jmax(tcHeight, this->getHeight()));
        timelineComponent->repaint();

        if (timelineComponent->getHeight() <= this->getHeight()) {
            setScrollBarsShown(false, true);
        } else {
            setScrollBarsShown(true, true);
        }

        repaint();
    }

    if (ev.eventComponent == this)
        if (!useMouseWheelMoveIfNeeded(ev, mouseWheelDetails))
            Component::mouseWheelMove(ev, mouseWheelDetails);
}

void track::TimelineComponent::mouseDown(const juce::MouseEvent &event) {
    if (event.mods.isRightButtonDown()) {
        DBG("TimelineComponent::mouseDown() for right mouse button down");
        juce::PopupMenu contextMenu;
        contextMenu.setLookAndFeel(&getLookAndFeel());

        // to select grid size for snapping to grid
        juce::PopupMenu gridMenu;
        gridMenu.addItem("1/1", true, SNAP_DIVISION == 1, [this] {
            SNAP_DIVISION = 1;
            repaint();
        });
        gridMenu.addItem("1/2", true, SNAP_DIVISION == 2, [this] {
            SNAP_DIVISION = 2;
            repaint();
        });
        gridMenu.addItem("1/4", true, SNAP_DIVISION == 4, [this] {
            SNAP_DIVISION = 4;
            repaint();
        });
        gridMenu.addItem("1/8", true, SNAP_DIVISION == 8, [this] {
            SNAP_DIVISION = 8;
            repaint();
        });
        gridMenu.addItem("1/16", true, SNAP_DIVISION == 16, [this] {
            SNAP_DIVISION = 16;
            repaint();
        });
        gridMenu.addItem("1/32", true, SNAP_DIVISION == 32, [this] {
            SNAP_DIVISION = 32;
            repaint();
        });
        gridMenu.addItem("1/64", true, SNAP_DIVISION == 64, [this] {
            SNAP_DIVISION = 64;
            repaint();
        });

#define MENU_PASTE_CLIP 1

        contextMenu.addItem(MENU_PASTE_CLIP, "Paste clip",
                            clipboard::typecode == TYPECODE_CLIP);
        contextMenu.addSubMenu("Grid", gridMenu);

        contextMenu.showMenuAsync(
            juce::PopupMenu::Options(), [this, event](int result) {
                if (result == MENU_PASTE_CLIP) {
                    DBG("paste clip");
                    if (clipboard::typecode != TYPECODE_CLIP)
                        return;

                    // create clip for processor
                    clip *orginalClip = (clip *)clipboard::retrieveData();
                    std::unique_ptr<clip> newClip(new clip());
                    newClip->path = orginalClip->path;
                    newClip->buffer = orginalClip->buffer;
                    newClip->active = orginalClip->active;
                    newClip->name = orginalClip->name;
                    newClip->startPositionSample =
                        (event.getMouseDownX() * 44100) / UI_ZOOM_MULTIPLIER;

                    int trackIndex = juce::jmin(
                        trackIndex = event.getMouseDownY() / UI_TRACK_HEIGHT,
                        (int)processorRef->tracks.size() - 1);
                    processorRef->tracks[(size_t)trackIndex].clips.push_back(
                        *newClip);

                    updateClipComponents();
                }
            });
    }
}

void track::TimelineComponent::paint(juce::Graphics &g) {
    // DBG("timelineComponent paint called");
    g.fillAll(juce::Colour(0xFF2E2E2E));

    // bar markers
    float secondsPerBeat = 60.f / BPM;
    float pxPerSecond = UI_ZOOM_MULTIPLIER;
    float pxPerBeat = secondsPerBeat * pxPerSecond;

    int beats = this->getWidth() / pxPerBeat;
    int scrollValue = viewport->getVerticalScrollBar().getCurrentRangeStart();

    // TODO: make this work for other time signatures
    // assuming 4/4 time signature
    float pxPerBar = pxPerBeat * 4;
    int bars = beats * 4;

    // space out markers
    float minSpace = 30.f;
    int incrementAmount = std::ceil(minSpace / pxPerBar);
    incrementAmount = std::max(1, incrementAmount);

    g.setFont(getInterRegular().withHeight(16.f));
    juce::Rectangle<int> bounds = getLocalBounds();
    bounds.setY(bounds.getY() - (bounds.getHeight() / 2.f) + 10 + scrollValue);

    for (int i = 0; i < bars; i += incrementAmount) {
        float x = i * pxPerBar;

        // draw numbers
        bounds.setX((int)x + 4);
        g.setColour(juce::Colour(0xFF'929292).withAlpha(.8f));
        g.drawText(juce::String(i + 1), bounds, juce::Justification::left,
                   false);

        // draw vertical lines for bar numbers
        g.setColour(juce::Colour(0xFF'444444).withAlpha(.5f));
        g.fillRect((int)x, 0, 1, 1280);

        // draw lines for grid snapping
        g.setColour(juce::Colour(0xFF'444444).withAlpha(.3f));
        for (int j = 0; j < SNAP_DIVISION; ++j) {
            int divX = x + ((pxPerBar / SNAP_DIVISION) * j);
            g.fillRect(divX, 0, 1, 1280);
        }
    }

    // horizontal divide line thingy
    g.setColour(juce::Colour(0xFF'1E1E1E).withAlpha(0.4f));
    g.drawRect(0, 20, getWidth(), 1);

    if (clipComponentsUpdated == false)
        updateClipComponents();

    // draw clips
    for (auto &&clip : this->clipComponents) {
        clip->setBounds(clip->correspondingClip->startPositionSample / 44100.0 *
                            UI_ZOOM_MULTIPLIER,
                        UI_TRACK_VERTICAL_OFFSET +
                            (clip->nodeDisplayIndex * UI_TRACK_HEIGHT) +
                            (UI_TRACK_VERTICAL_MARGIN * clip->nodeDisplayIndex),
                        clip->correspondingClip->buffer.getNumSamples() /
                            44100.0 * UI_ZOOM_MULTIPLIER,
                        UI_TRACK_HEIGHT);

        // handle offline clips
        if (clip->correspondingClip->buffer.getNumSamples() == 0) {
            juce::Rectangle<int> offlineClipBounds = clip->getBounds();
            offlineClipBounds.setWidth(110);
            clip->setBounds(offlineClipBounds);
        }
    }
}

void track::TimelineComponent::updateClipComponents() {
    clipComponentsUpdated = true;

    clipComponents.clear();

    DBG("updateClipComponents() called");
    DBG("viewport->tracklist->trackComponents.size() = "
        << viewport->tracklist->trackComponents.size());

    for (size_t i = 0; i < viewport->tracklist->trackComponents.size(); ++i) {
        DBG("i = " << i);
        audioNode *node =
            viewport->tracklist->trackComponents[i]->getCorrespondingTrack();

        if (!node->isTrack)
            continue;

        for (auto &c : node->clips) {
            this->clipComponents.push_back(std::make_unique<ClipComponent>(&c));
            addAndMakeVisible(*clipComponents.back());

            auto &cc = clipComponents.back();
            cc->nodeDisplayIndex = i;
        }
    }

    resizeTimelineComponent();
}

void track::TimelineComponent::resizeTimelineComponent() {
    int largestEnd = -1;

    for (audioNode &t : processorRef->tracks) {
        for (clip &c : t.clips) {
            if ((c.buffer.getNumSamples() + c.startPositionSample) >
                largestEnd) {
                largestEnd = c.buffer.getNumSamples() + c.startPositionSample;
            }
        }
    }

    // DBG("largestEnd is " << largestEnd);
    jassert(UI_ZOOM_MULTIPLIER > 0);
    largestEnd /= 44100 / UI_ZOOM_MULTIPLIER;
    largestEnd += 2000;
    // DBG("now, largestEnd is " << largestEnd);

    // this->setSize(juce::jmax(getWidth(), largestEnd), getHeight());
    this->setSize(largestEnd, getHeight());
}

void track::TimelineComponent::deleteClip(clip *c, int trackIndex) {
    int clipIndex = -1;

    audioNode *node = viewport->tracklist->trackComponents[(size_t)trackIndex]
                          ->getCorrespondingTrack();

    for (size_t i = 0; i < node->clips.size(); ++i) {
        if (c == &node->clips[i]) {
            clipIndex = i;
            break;
        }
    }

    node->clips.erase(node->clips.begin() + clipIndex);

    updateClipComponents();
}

bool track::TimelineComponent::isInterestedInFileDrag(
    const juce::StringArray &files) {
    return true;
}

void track::TimelineComponent::filesDropped(const juce::StringArray &files,
                                            int x, int y) {
    // TODO: handle changing samplle rates of dragged file to match with sample
    // rate of host

    if (viewport->tracklist->trackComponents.size() == 0)
        return;

    int nodeDisplayIndex =
        juce::jmin((y / UI_TRACK_HEIGHT) - 1,
                   (int)viewport->tracklist->trackComponents.size() - 1);
    DBG("track index is " << nodeDisplayIndex);

    std::unique_ptr<clip> c(new clip());
    c->path = files[0];

    // check if file is valid audio
    juce::File file(c->path);
    juce::AudioFormatManager afm;
    afm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(afm.createReaderFor(file));

    if (reader == nullptr) {
        DBG("INVALID AUDIO FILE DRAGGED");
        return;
    }

    // remove directories leading up to the actual file name we want, and
    // strip file extension
    if (files[0].contains("/")) {
        c->name = files[0].fromLastOccurrenceOf(
            "/", false, true); // for REAL operating systems.
    } else {
        c->name = files[0].fromLastOccurrenceOf("\\", false, true);
    }
    c->name = c->name.upToLastOccurrenceOf(".", false, true);

    c->startPositionSample = x * 32 * 40;
    c->updateBuffer();

    DBG("nodeDisplayIndex is " << nodeDisplayIndex);
    audioNode *node =
        viewport->tracklist->trackComponents[(size_t)nodeDisplayIndex]
            ->getCorrespondingTrack();

    if (!node->isTrack) {
        DBG("Rejecting file drop onto group");
    } else {
        node->clips.push_back(*c);
        DBG("inserted clip into node");
    }

    updateClipComponents();
}
