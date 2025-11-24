#include "track.h"
#include "../editor.h"
#include "../processor.h"
#include "automation_relay.h"
#include "clipboard.h"
#include "defs.h"
#include "subwindow.h"
#include "timeline.h"
#include "utility.h"

track::ClipComponent::ClipComponent(clip *c)
    : juce::Component(), thumbnailCache(5),
      thumbnail(256, afm, thumbnailCache) {
    if (c == nullptr) {
        DBG("JOHN WARNING: ClipComponent initialized with no clip provided");
        return;
    }

    this->correspondingClip = c;
    thumbnail.setSource(&correspondingClip->buffer, SAMPLE_RATE, 2);

    thumbnail.addChangeListener(this);

    clipNameLabel.setFont(
        getInterSemiBold().withHeight(17.f).withExtraKerningFactor(-.02f));
    clipNameLabel.setColour(juce::Label::textColourId,
                            juce::Colour(0xFF'AECBED).brighter(.5f));
    clipNameLabel.setEditable(true);
    clipNameLabel.setText(c->name,
                          juce::NotificationType::dontSendNotification);
    clipNameLabel.setInterceptsMouseClicks(false, false);
    // clipNameLabel.setBounds(getLocalBounds().getX(), getLocalBounds().getY(),
    // 300, 20);
    addAndMakeVisible(clipNameLabel);

    // clipNameLabel.onTextChange = [this] {};

    clipNameLabel.onEditorHide = [this] {
        TimelineComponent *tc = (TimelineComponent *)getParentComponent();
        Tracklist *tracklist = tc->viewport->tracklist;

        audioNode *node = tracklist->trackComponents[(size_t)nodeDisplayIndex]
                              ->getCorrespondingTrack();
        int index = track::utility::getIndexOfClip(node, correspondingClip);

        ActionClipModified *action = new ActionClipModified(
            tc->processorRef,
            tracklist->trackComponents[(size_t)nodeDisplayIndex]->route, index,
            *this->correspondingClip);

        action->tc = tc;
        action->cc = this;

        correspondingClip->name = clipNameLabel.getText(true);

        action->newClip.name = correspondingClip->name;
        // action->oldClip.name = "beans lmao";

        DBG("calling perform()");

        tc->processorRef->undoManager.beginNewTransaction(
            "action clip modified");
        tc->processorRef->undoManager.perform(action);

        repaint();
    };

    setBufferedToImage(true);
    setOpaque(false);

    setWantsKeyboardFocus(true);
}
track::ClipComponent::~ClipComponent() { thumbnail.removeAllChangeListeners(); }

void track::ClipComponent::changeListenerCallback(ChangeBroadcaster *source) {
    repaint();
}

void track::ClipComponent::paint(juce::Graphics &g) {
    float cornerSize = 4.f;

    if (thumbnail.getNumChannels() == 0) {
        g.fillAll(juce::Colours::grey);
        g.setColour(juce::Colours::white);
        g.setFont(12.f);
        g.drawText("SAMPLE OFFLINE", getLocalBounds(),
                   juce::Justification::centred, true);
    }

    else {
        if (isBeingDragged) {
            g.setColour(juce::Colour(0xFF'487FB7));
            g.fillRoundedRectangle(getLocalBounds().toFloat(), cornerSize);
        } else {
            if (this->correspondingClip->active) {
                if (this->hasKeyboardFocus(true))
                    g.setColour(juce::Colour(0xFF'33587F)
                                    .brighter(0.3f)
                                    .withMultipliedSaturation(1.2f));
                else
                    g.setColour(juce::Colour(0xFF'33587F));

                g.fillRoundedRectangle(getLocalBounds().toFloat(), cornerSize);
            }
        }

        if (this->correspondingClip->active) {
            g.setColour(juce::Colour(0xFFAECBED));
        } else
            g.setColour(juce::Colour(0xFF'696969)); // nice

        if (UI_TRACK_HEIGHT > UI_TRACK_HEIGHT_COLLAPSE_BREAKPOINT) {
            int thumbnailTopMargin = 14;
            juce::Rectangle<int> thumbnailBounds = getLocalBounds().reduced(2);
            thumbnailBounds.setHeight(thumbnailBounds.getHeight() -
                                      thumbnailTopMargin);
            thumbnailBounds.setY(thumbnailBounds.getY() + thumbnailTopMargin);

            thumbnail.drawChannels(
                g, thumbnailBounds,
                correspondingClip->trimLeft / track::SAMPLE_RATE,
                (correspondingClip->buffer.getNumSamples() -
                 correspondingClip->trimRight) /
                    track::SAMPLE_RATE,
                correspondingClip->gain); // TODO: is just changing the vertical
                                          // zoom factor actually accurate?

            if (thumbnail.getNumChannels() == 1) {
                // mono
                g.drawHorizontalLine(thumbnailTopMargin +
                                         (thumbnailBounds.getHeight() / 2) + 1,
                                     0, getWidth());
            } else {
                // stereo
                g.drawHorizontalLine(thumbnailTopMargin +
                                         (thumbnailBounds.getHeight() / 4) + 2,
                                     0, getWidth());

                g.drawHorizontalLine(
                    thumbnailTopMargin +
                        ((thumbnailBounds.getHeight() / 4) * 3) + 3,
                    0, getWidth());
            }
        }
    }

    if (isMouseOver(false) && !isBeingDragged) {
        g.setColour(juce::Colours::white.withAlpha(.07f));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), cornerSize);
    }

    g.setColour(juce::Colour(0xFF'000515));
    g.drawRoundedRectangle(getLocalBounds().toFloat(), cornerSize, 1.4f);

    // trim handles
    if (drawTrimHandles != 0) {
        g.setColour(juce::Colours::white.withAlpha(0.4f));
        g.drawRoundedRectangle(getLocalBounds().toFloat(), cornerSize, 2.f);
    }

    g.setColour(juce::Colour(0xFFAECBED));
    int handleWidth = 3;

    float y = 0.0f;
    float w = handleWidth;
    float radius = cornerSize;
    float h = getHeight();

    Path p;
    if (drawTrimHandles == -1) {
        float x = 0.f;

        p.startNewSubPath(x + w, y);
        p.lineTo(x + radius, y);
        p.addArc(x, y, radius, radius, 0.0f, -MathConstants<float>::halfPi);
        p.lineTo(x, y + h - radius);
        p.addArc(x, y + h - radius, radius, radius,
                 -MathConstants<float>::halfPi, -MathConstants<float>::halfPi);
        p.lineTo(x + w, y + h);
        p.closeSubPath();
        g.fillPath(p);

    } else if (drawTrimHandles == 1) {
        float x = getWidth() - handleWidth;

        p.startNewSubPath(x, y);
        p.lineTo(x + w - radius, y);
        p.addArc(x + w - radius, y, radius, radius, 0.0f,
                 MathConstants<float>::halfPi);
        p.lineTo(x + w, y + h - radius);
        p.addArc(x + w - radius, y + h - radius, radius, radius,
                 MathConstants<float>::halfPi, MathConstants<float>::pi);
        p.lineTo(x, y + h);
        p.closeSubPath();
        g.fillPath(p);
    }

    /*
    if (this->correspondingClip->active)
        g.setColour(juce::Colour(0xFF33587F));
    else
        g.setColour(juce::Colour(0xFF'9B9B9B));

    g.drawText(this->correspondingClip->name, 2, 0, getWidth(), 20,
               juce::Justification::left, true);
    */
}

void track::ClipComponent::mouseDown(const juce::MouseEvent &event) {
    if (event.mods.isRightButtonDown()) {
        DBG("rmb down");

        juce::PopupMenu contextMenu;
        contextMenu.setLookAndFeel(&getLookAndFeel());

        // future john: if you're wondering why for clips you define item result
        // IDs like this, but only for clips and not other popup menus, it's
        // because defining item result IDs make it easier to reorder items, and
        // clip components will have many items in its popup menus, but not
        // other components' popup menus

#define MENU_REVERSE_CLIP 1
#define MENU_CUT_CLIP 2
#define MENU_TOGGLE_CLIP_ACTIVATION 3
#define MENU_COPY_CLIP 4
#define MENU_SHOW_IN_EXPLORER 5
#define MENU_SPLIT_CLIP 6
#define MENU_RENAME_CLIP 7
#define MENU_DUPLICATE_CLIP_IMMEDIATELY_AFTER 8
#define MENU_EDIT_CLIP_PROPERTIES 9

        contextMenu.addItem(MENU_RENAME_CLIP, "Rename clip");
        contextMenu.addItem(MENU_DUPLICATE_CLIP_IMMEDIATELY_AFTER,
                            "Duplicate clip immediately after");
        contextMenu.addItem(MENU_COPY_CLIP, "Copy clip");
        contextMenu.addItem(MENU_CUT_CLIP, "Cut");
        contextMenu.addSeparator();
        contextMenu.addItem(MENU_REVERSE_CLIP, "Reverse");
        contextMenu.addItem(MENU_TOGGLE_CLIP_ACTIVATION,
                            "Toggle activate/deactive clip");
        contextMenu.addItem(MENU_SHOW_IN_EXPLORER, "Show in explorer");
        contextMenu.addItem(MENU_EDIT_CLIP_PROPERTIES, "Edit clip properties");

        int splitSample = ((float)event.x / UI_ZOOM_MULTIPLIER) * SAMPLE_RATE;
        contextMenu.addItem(
            MENU_SPLIT_CLIP,
            "split at " + juce::String(splitSample / SAMPLE_RATE) + " seconds");

        juce::PopupMenu::Options options;

        contextMenu.showMenuAsync(options, [this, event](int result) {
            if (result == MENU_RENAME_CLIP) {
                juce::Timer::callAfterDelay(50, [this] {
                    // needs to be done after a slight delay because JUCE focus
                    // changes due to the popup menu or smth idk man im tired i
                    // want to sleep
                    clipNameLabel.showEditor();
                });
            }

            else if (result == MENU_REVERSE_CLIP) {
                this->correspondingClip->reverse();
                thumbnailCache.clear();
                thumbnail.setSource(&correspondingClip->buffer, SAMPLE_RATE, 2);
                repaint();
            }

            else if (result == MENU_DUPLICATE_CLIP_IMMEDIATELY_AFTER) {
                std::unique_ptr<clip> newClip(new clip());
                newClip->path = correspondingClip->path;
                newClip->buffer = correspondingClip->buffer;
                newClip->active = correspondingClip->active;
                newClip->name = correspondingClip->name;
                newClip->trimLeft = correspondingClip->trimLeft;
                newClip->trimRight = correspondingClip->trimRight;
                newClip->gain = correspondingClip->gain;

                int clipLength = correspondingClip->buffer.getNumSamples() -
                                 correspondingClip->trimLeft -
                                 correspondingClip->trimRight;
                newClip->startPositionSample =
                    correspondingClip->startPositionSample + clipLength;

                // retrieve viewport, then get tracklist via viewport
                track::TimelineViewport *viewport = (track::TimelineViewport *)
                    findParentComponentOfClass<TimelineViewport>();
                jassert(viewport != nullptr);

                track::Tracklist *tracklist = viewport->tracklist;
                audioNode *node =
                    tracklist->trackComponents[(size_t)this->nodeDisplayIndex]
                        ->getCorrespondingTrack();

                node->clips.push_back(*newClip);

                // update clip components otherwise shit hits the fan
                track::TimelineComponent *tc = (track::TimelineComponent *)
                    findParentComponentOfClass<TimelineComponent>();
                jassert(tc != nullptr);

                tc->updateClipComponents();
            }

            else if (result == MENU_CUT_CLIP) {
                TimelineComponent *tc =
                    (TimelineComponent *)getParentComponent();

                tc->deleteClip(correspondingClip, nodeDisplayIndex);
            }

            else if (result == MENU_TOGGLE_CLIP_ACTIVATION) {
                this->correspondingClip->active =
                    !this->correspondingClip->active;
                repaint();
            }

            else if (result == MENU_COPY_CLIP) {
                // create new clip and set clipboard's data as this new clip
                // also, by creating a copy of the clip we allow the user to
                // delete the orginal clip from timeline, and still paste it
                track::clip *newClip = new track::clip;

                /*newClip->buffer = correspondingClip->buffer;
                newClip->startPositionSample =
                    correspondingClip->startPositionSample;
                newClip->name = correspondingClip->name;
                newClip->path = correspondingClip->path;
                newClip->active = correspondingClip->active;
                newClip->trimLeft = correspondingClip->trimLeft;
                newClip->trimRight = correspondingClip->trimRight;*/

                *newClip = *correspondingClip;

                clipboard::setData(newClip, TYPECODE_CLIP);
            }

            else if (result == MENU_SHOW_IN_EXPLORER) {
                juce::File f(correspondingClip->path);
                f.revealToUser();
            }

            else if (result == MENU_SPLIT_CLIP) {
                int splitSample =
                    ((float)event.x / UI_ZOOM_MULTIPLIER) * SAMPLE_RATE;

                TimelineComponent *tc =
                    findParentComponentOfClass<TimelineComponent>();
                tc->splitClip(this->correspondingClip, splitSample,
                              nodeDisplayIndex);
            }

            else if (result == MENU_EDIT_CLIP_PROPERTIES) {
                TimelineComponent *tc =
                    (TimelineComponent *)getParentComponent();
                TimelineViewport *tv =
                    (TimelineViewport *)
                        tc->findParentComponentOfClass<TimelineViewport>();

                AudioPluginAudioProcessorEditor *editor =
                    tv->findParentComponentOfClass<
                        AudioPluginAudioProcessorEditor>();

                track::Tracklist *tracklist = tv->tracklist;

                std::vector<int> nodeRoute =
                    tracklist->trackComponents[(size_t)this->nodeDisplayIndex]
                        ->route;
                audioNode *node =
                    tracklist->trackComponents[(size_t)this->nodeDisplayIndex]
                        ->getCorrespondingTrack();

                int clipIndex = -1;
                for (size_t i = 0; i < node->clips.size(); ++i) {
                    if (correspondingClip == &node->clips[i]) {
                        clipIndex = i;
                        break;
                    }
                }

                jassert(clipIndex != -1);

                editor->openClipPropertiesWindows(nodeRoute, clipIndex);
            }
        });
    }

    else {
        startDragX = getLocalBounds().getX();
        startDragStartPositionSample = correspondingClip->startPositionSample;
        startTrimLeftPositionSample = correspondingClip->trimLeft;
        startTrimRightPositionSample = correspondingClip->trimRight;
        mouseClickX = event.x;

        if (mouseClickX <= track::TRIM_REGION_WIDTH)
            trimMode = -1;
        else if (event.x >= (getWidth() - track::TRIM_REGION_WIDTH))
            trimMode = 1;
        else
            trimMode = 0;
    }
}

void track::ClipComponent::mouseDrag(const juce::MouseEvent &event) {
    // DBG("dragging is happening");
    isBeingDragged = true;

    int distanceMoved = startDragX + event.getDistanceFromDragStartX();
    int rawSamplePos = startDragStartPositionSample +
                       ((distanceMoved * SAMPLE_RATE) / UI_ZOOM_MULTIPLIER);

    // if ctrl held, trim
    if (event.mods.isCtrlDown()) {
        // trim left
        if (trimMode == -1) {
            int rawSamplePosTrimLeft =
                startTrimLeftPositionSample +
                ((distanceMoved * SAMPLE_RATE) / UI_ZOOM_MULTIPLIER);

            if (event.mods.isAltDown()) {
                correspondingClip->trimLeft = rawSamplePosTrimLeft;
            } else {
                double secondsPerBeat = 60.f / BPM;
                int samplesPerBar = (secondsPerBeat * SAMPLE_RATE) * 4;
                int samplesPerSnap = samplesPerBar / SNAP_DIVISION;

                int snappedTrimLeft =
                    ((rawSamplePosTrimLeft + samplesPerSnap / 2) /
                     samplesPerSnap) *
                    samplesPerSnap;

                correspondingClip->trimLeft = snappedTrimLeft;
            }

            // DBG("raw left = " << rawSamplePosTrimLeft);

            this->reachedLeft = rawSamplePosTrimLeft <= 0;

            correspondingClip->trimLeft =
                juce::jmax(0, correspondingClip->trimLeft);
        }

        // trim right
        else if (trimMode == 1) {
            DBG("mouseClickX = " << mouseClickX);
            DBG("getWidth() - track::TRIM_REGION_WIDTH = "
                << getWidth() - track::TRIM_REGION_WIDTH);

            int rawSamplePosTrimRight =
                startTrimRightPositionSample +
                ((-distanceMoved * SAMPLE_RATE) / UI_ZOOM_MULTIPLIER);

            if (event.mods.isAltDown()) {
                correspondingClip->trimRight = rawSamplePosTrimRight;
            } else {
                double secondsPerBeat = 60.f / BPM;
                int samplesPerBar = (secondsPerBeat * SAMPLE_RATE) * 4;
                int samplesPerSnap = samplesPerBar / SNAP_DIVISION;

                int snappedTrimRight =
                    ((rawSamplePosTrimRight + samplesPerSnap / 2) /
                     samplesPerSnap) *
                    samplesPerSnap;
                correspondingClip->trimRight = snappedTrimRight;
            }

            DBG("raw right = " << rawSamplePosTrimRight);

            correspondingClip->trimRight =
                juce::jmax(0, correspondingClip->trimRight);

            // trimming left involves changing trim left AND moving start
            // sample which already happens anyway below, but trimming from
            // right involves only changing trim right; so we resize and
            // repaint to finish trimming right
            TimelineComponent *tc =
                findParentComponentOfClass<TimelineComponent>();
            tc->resizeClipComponent(this);

            repaint();
            return;
        }
    }

    bool forbidMovement = trimMode == -1 && reachedLeft;

    int oldEndPos = correspondingClip->startPositionSample +
                    correspondingClip->buffer.getNumSamples() -
                    correspondingClip->trimRight - correspondingClip->trimLeft;

    // moving clips. fixing messed up left trim. this is a mess.
    int newStartPos = -1;

    double secondsPerBeat = 60.f / BPM;
    int samplesPerBar = (secondsPerBeat * SAMPLE_RATE) * 4;
    int samplesPerSnap = samplesPerBar / SNAP_DIVISION;

    // if alt held, don't snap
    int snappedSamplePos = -1;
    if (event.mods.isAltDown()) {
        newStartPos = rawSamplePos;
    } else {
        snappedSamplePos =
            ((rawSamplePos + samplesPerSnap / 2) / samplesPerSnap) *
            samplesPerSnap;

        newStartPos = snappedSamplePos;
    }

    correspondingClip->startPositionSample = newStartPos;

    int newEndPos = newStartPos + correspondingClip->buffer.getNumSamples() -
                    correspondingClip->trimRight - correspondingClip->trimLeft;

    int diff = oldEndPos - newEndPos;

    // if we want to forbid movement, then shove the clip back its orginal
    // place. there's probably a better way to take care of this.
    if (forbidMovement) {
        correspondingClip->startPositionSample += diff;
    }

    int endPosPostCorrection = correspondingClip->startPositionSample +
                               correspondingClip->buffer.getNumSamples() -
                               correspondingClip->trimRight -
                               correspondingClip->trimLeft;

    // there's a strange bug where is you untrim left too quickly, the whole
    // clip shifts to the right a bit. this should shove the clip back in place
    if (finalEndPos - endPosPostCorrection < 0 && trimMode == -1) {
        DBG(finalEndPos - endPosPostCorrection);
        DBG(event.getDistanceFromDragStartX());
        DBG("-");

        // if distance from drag start is 1, the whole clip flies away?????
        if (event.getDistanceFromDragStartX() != 1)
            correspondingClip->startPositionSample +=
                finalEndPos - endPosPostCorrection;
    }
    finalEndPos = endPosPostCorrection;

    TimelineComponent *tc = findParentComponentOfClass<TimelineComponent>();
    tc->resizeClipComponent(this);

    repaint();
}

void track::ClipComponent::mouseUp(const juce::MouseEvent &event) {
    if (isBeingDragged) {
        DBG("DRAGGING STOPPED");
        isBeingDragged = false;

        int distanceMovedHorizontally = event.getDistanceFromDragStartX();

        TimelineComponent *tc = (TimelineComponent *)getParentComponent();
        jassert(tc != nullptr);
        tc->resizeTimelineComponent();

        tc->grabKeyboardFocus();

        // dispatch to undomanager
        Tracklist *tracklist = tc->viewport->tracklist;
        audioNode *node = tracklist->trackComponents[(size_t)nodeDisplayIndex]
                              ->getCorrespondingTrack();
        int index = track::utility::getIndexOfClip(node, correspondingClip);

        ActionClipModified *action = new ActionClipModified(
            tc->processorRef,
            tracklist->trackComponents[(size_t)nodeDisplayIndex]->route, index,
            *this->correspondingClip);

        action->tc = tc;
        action->cc = this;

        action->oldClip.startPositionSample = startDragStartPositionSample;
        action->oldClip.trimLeft = startTrimLeftPositionSample;
        action->oldClip.trimRight = startTrimRightPositionSample;

        tc->processorRef->undoManager.beginNewTransaction(
            "action clip modified");
        tc->processorRef->undoManager.perform(action);
    }

    if (!event.mods.isLeftButtonDown()) {
        trimMode = 0;
    }
    reachedLeft = false;

    DBG(event.getDistanceFromDragStartX());
    repaint();
}

void track::ClipComponent::mouseMove(const juce::MouseEvent &event) {
    // DBG("event.x=" << event.x);
    if (event.x <= track::TRIM_REGION_WIDTH) {
        // DBG("drawTrimHandles = -1");
        drawTrimHandles = -1;
        repaint();
        return;
    }
    if (event.x >= (getWidth() - track::TRIM_REGION_WIDTH)) {
        drawTrimHandles = 1;
        repaint();
        return;
    }

    if (drawTrimHandles != 0) {
        drawTrimHandles = 0;
        repaint();
    }
}

bool track::ClipComponent::keyStateChanged(bool isKeyDown) {
    juce::KeyPress curCombo = juce::KeyPress::createFromDescription("ctrl+y");
    if (curCombo.isCurrentlyDown()) {
        DBG("curCombo is down");
        DBG(curCombo.getKeyCode());
    }

    if (isKeyDown) {
        // r
        if (juce::KeyPress::isKeyCurrentlyDown(82)) {
            juce::Timer::callAfterDelay(10,
                                        [this] { clipNameLabel.showEditor(); });
        }

        // 0
        else if (juce::KeyPress::isKeyCurrentlyDown(48)) {
            this->correspondingClip->active = !this->correspondingClip->active;
            repaint();

            TimelineComponent *tc = (TimelineComponent *)getParentComponent();
            tc->grabKeyboardFocus();
        }

        // x, backspace, delete
        // don't allow deletion of clip while its name is being edited
        else if ((juce::KeyPress::isKeyCurrentlyDown(88) ||
                  juce::KeyPress::isKeyCurrentlyDown(8) ||
                  juce::KeyPress::isKeyCurrentlyDown(268435711)) &&
                 !clipNameLabel.isBeingEdited()) {
            TimelineComponent *tc = (TimelineComponent *)getParentComponent();
            tc->deleteClip(correspondingClip, nodeDisplayIndex);
        }

        // ctrl+z
        else if (juce::KeyPress::isKeyCurrentlyDown(90)) {
            TimelineComponent *tc = (TimelineComponent *)getParentComponent();
            // if (tc->processorRef->undoManager.canUndo()) {
            DBG("calling processor's undoManager.undo()");

            // ActionClipComponentMove x{};
            // tc->processorRef->undoManager.perform(&x);

            if (!tc->processorRef->undoManager.canUndo()) {
                DBG("CANNOT UNDO :(");
            } else {

                tc->processorRef->undoManager.undo();
                // tc->resizeClipComponent(this);
            }

            repaint();
        }

        // ctrl+y
        else if (juce::KeyPress::isKeyCurrentlyDown(89)) {
            DBG("calling redo()");
            TimelineComponent *tc = (TimelineComponent *)getParentComponent();
            tc->processorRef->undoManager.redo();

            // tc->resizeClipComponent(this);
            repaint();
        }
    }

    return false;
}

track::ActionClipModified::ActionClipModified(void *processor,
                                              std::vector<int> nodeRoute,
                                              int indexOfClip, clip c)
    : juce::UndoableAction() {
    this->p = processor;
    this->route = nodeRoute;
    this->clipIndex = indexOfClip;
    this->newClip = c;
    this->oldClip = c;
};
track::ActionClipModified::~ActionClipModified() {}

track::clip *track::ActionClipModified::getClip() {
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    audioNode *head = &processor->tracks[(size_t)route[0]];
    for (size_t i = 1; i < route.size(); ++i) {
        head = &head->childNodes[(size_t)route[i]];
    }
    clip *c = &head->clips[(size_t)clipIndex];
    return c;
}

bool track::ActionClipModified::perform() {
    DBG("PERFORM() CALLED");

    clip *c = getClip();
    *c = newClip;

    updateGUI();

    return true;
}

bool track::ActionClipModified::undo() {
    clip *c = getClip();
    *c = oldClip;

    updateGUI();

    return true;
}

void track::ActionClipModified::updateGUI() {
    jassert(tc != nullptr);
    jassert(cc != nullptr);

    // TODO: some funky stuff happening here when you have a longer undo
    // history. this->cc probably doesn't point to a valid clip component ptr
    TimelineComponent *timelineComponent = (TimelineComponent *)tc;
    /*
    ClipComponent *clipComponent = (ClipComponent *)cc;

    timelineComponent->resizeClipComponent(clipComponent);

    clipComponent->clipNameLabel.setText(
        getClip()->name, juce::NotificationType::dontSendNotification);

    clipComponent->repaint();*/

    timelineComponent->updateClipComponents();
}

track::ClipPropertiesWindow::ClipPropertiesWindow() : track::Subwindow() {
    addAndMakeVisible(nameLabel);
    addAndMakeVisible(gainSlider);

    gainSlider.setRange(0.f, 6.f);
    gainSlider.setNumDecimalPlacesToDisplay(2);
    nameLabel.setEditable(true, true, false);

    nameLabel.onTextChange = [this] {
        getClip()->name = nameLabel.getText();
        repaint();

        AudioPluginAudioProcessorEditor *editor =
            findParentComponentOfClass<AudioPluginAudioProcessorEditor>();
        editor->timelineComponent->updateClipComponents();
    };

    gainSlider.onValueChange = [this] {
        // change clips gain
        getClip()->gain = gainSlider.getValue();
    };

    gainSlider.onDragEnd = [this] {
        // redraw clip
        // TODO: optimize
        AudioPluginAudioProcessorEditor *editor =
            findParentComponentOfClass<AudioPluginAudioProcessorEditor>();
        editor->timelineComponent->updateClipComponents();
    };

    nameLabel.setFont(getInterSemiBold().withHeight(17.f));

    gainSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxLeft,
                               false, 52 - 12, 16);
}

track::ClipPropertiesWindow::~ClipPropertiesWindow() {}
void track::ClipPropertiesWindow::paint(juce::Graphics &g) {
    Subwindow::paint(g);

    g.setColour(juce::Colour(0xFF'A7A7A7));
    g.setFont(getTitleBarFont());
    g.drawText(
        getClip()->name,
        getTitleBarBounds().withLeft(10).withTop(2).withWidth(getWidth() - 36),
        juce::Justification::left);

    // main
    g.setFont(nameLabel.getFont().italicised());
    juce::Rectangle<int> attributeNameLabelBounds =
        nameLabel.getBounds().withWidth(48).withX(nameLabel.getX() - 44);

    g.drawText("NAME ", attributeNameLabelBounds, juce::Justification::left,
               false);

    attributeNameLabelBounds.setY(attributeNameLabelBounds.getY() +
                                  attributeNameLabelBounds.getHeight() + 2);
    g.drawText("GAIN ", attributeNameLabelBounds, juce::Justification::left,
               false);

    attributeNameLabelBounds.setY(attributeNameLabelBounds.getY() +
                                  attributeNameLabelBounds.getHeight() + 2);
    g.drawText("PATH  " + getClip()->path,
               attributeNameLabelBounds.withWidth(getWidth() - 28),
               juce::Justification::left, true);
}

void track::ClipPropertiesWindow::resized() {
    Subwindow::resized();
    nameLabel.setBounds(54, 32, getWidth() - 54 - 8, 24);
    gainSlider.setBounds(34 + 19, 55, getWidth() - 28 - 32, 30);
}

void track::ClipPropertiesWindow::init() {
    nameLabel.setText(getClip()->name,
                      juce::NotificationType::dontSendNotification);

    gainSlider.setValue(getClip()->gain);
}

track::clip *track::ClipPropertiesWindow::getClip() {
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;

    audioNode *head = &processor->tracks[(size_t)route[0]];

    for (size_t i = 1; i < route.size(); ++i) {
        head = &head->childNodes[(size_t)route[i]];
    }

    return &head->clips[(size_t)clipIndex];
}

track::audioNode *
track::TrackComponent::TrackComponent::getCorrespondingTrack() {
    // DBG("TrackComponent::getCorrespondingTrack() called");

    if (processor == nullptr) {
        DBG("! TRACK COMPONENT'S getCorrespondingTrack() RETURNED nullptr");
        DBG("TrackComponent's processor is nullptr");
        return nullptr;
    }
    if (siblingIndex == -1) {
        DBG("! TRACK COMPONENT'S getCorrespondingTrack() RETURNED nullptr");
        DBG("TrackComponent's trackIndex is -1");
        return nullptr;
    }

    if (route.size() == 0) {
        DBG("TrackComponent does not have a route");
        return nullptr;
    }

    AudioPluginAudioProcessor *p = (AudioPluginAudioProcessor *)processor;

    audioNode *head = &p->tracks[(size_t)route[0]];
    for (size_t i = 1; i < route.size(); ++i) {
        head = &head->childNodes[(size_t)route[i]];
    }

    return head;
}

void track::TrackComponent::initializSliders() {
    // DBG("intiailizGainSlider() called");
    // DBG("track's gain is " << getCorrespondingTrack()->gain);
    gainSlider.setValue(getCorrespondingTrack()->gain);
    panSlider.setValue(getCorrespondingTrack()->pan);
}

track::TrackComponent::TrackComponent(int trackIndex) : juce::Component() {
    // starting text for track name label is set when TrackComponent is
    // created in createTrackComponents() DBG("TrackComponent constructor
    // called");

    trackNameLabel.setFont(
        getAudioNodeLabelFont().withHeight(17.f).withExtraKerningFactor(
            -0.02f));
    trackNameLabel.setBounds(0, 0, 100, 20);
    trackNameLabel.setEditable(true);
    addAndMakeVisible(trackNameLabel);

    trackNameLabel.onTextChange = [this] {
        this->getCorrespondingTrack()->trackName = trackNameLabel.getText(true);
    };

    this->siblingIndex = trackIndex;

    addAndMakeVisible(muteBtn);
    muteBtn.setButtonText("M");

    addAndMakeVisible(soloBtn);
    soloBtn.setButtonText("S");

    // gainSlider.hideTextBox(true);
    // gainSlider.setValue(getCorrespondingTrack()->gain);
    gainSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox,
                               true, 0, 0);

    gainSlider.onDragEnd = [this] {
        DBG("setting new gain for track; " << gainSlider.getValue());
        getCorrespondingTrack()->gain = gainSlider.getValue();
    };

    gainSlider.onValueChange = [this] {
        getCorrespondingTrack()->gain = gainSlider.getValue();
    };

    addAndMakeVisible(gainSlider);
    gainSlider.setSkewFactorFromMidPoint(0.3f);
    gainSlider.setRange(0.0, 6.0);

    muteBtn.onClick = [this] {
        getCorrespondingTrack()->m = !(getCorrespondingTrack()->m);
        repaint();
    };

    soloBtn.onClick = [this] {
        getCorrespondingTrack()->s = !(getCorrespondingTrack()->s);
        repaint();

        AudioPluginAudioProcessor *p = (AudioPluginAudioProcessor *)processor;

        if (getCorrespondingTrack()->s) {
            p->soloMode = true;
            DBG("! SOLO MODE = TRUE");
        } else {
            Tracklist *tracklist =
                (Tracklist *)findParentComponentOfClass<Tracklist>();

            bool foundSolo = false;
            for (auto &nodeComponents : tracklist->trackComponents) {
                if (nodeComponents->getCorrespondingTrack()->s) {
                    foundSolo = true;
                    break;
                }
            }

            if (!foundSolo) {
                p->soloMode = false;
                DBG("! SOLO MODE = FALSE");
            }
        }
    };

    panSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox,
                              true, 0, 0);
    panSlider.setSliderStyle(
        juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);

    addAndMakeVisible(panSlider);
    panSlider.setRange(-1.f, 1.f);

    panSlider.onValueChange = [this] {
        getCorrespondingTrack()->pan = panSlider.getValue();
        DBG("new pan value is " << panSlider.getValue());
    };

    fxBtn.setButtonText("FX");
    addAndMakeVisible(fxBtn);

    fxBtn.onClick = [this] {
        DBG("FX button clicked");

        AudioPluginAudioProcessorEditor *editor =
            this->findParentComponentOfClass<AudioPluginAudioProcessorEditor>();
        editor->openFxChain(route);
    };
}
track::TrackComponent::~TrackComponent() {}

void track::TrackComponent::mouseDown(const juce::MouseEvent &event) {
    if (event.mods.isRightButtonDown()) {
        PopupMenu contextMenu;
        contextMenu.setLookAndFeel(&getLookAndFeel());

        contextMenu.addItem("Rename", [this] {
            juce::Timer::callAfterDelay(
                50, [this] { trackNameLabel.showEditor(); });
        });

        contextMenu.addSeparator();

        contextMenu.addItem(
            "Ungroup",
            !getCorrespondingTrack()->isTrack || this->route.size() >= 2, false,
            [this] {
                DBG("ungroup track");

                AudioPluginAudioProcessor *p =
                    (AudioPluginAudioProcessor *)processor;

                audioNode *head = nullptr;
                Tracklist *tracklist = findParentComponentOfClass<Tracklist>();

                if (getCorrespondingTrack()->isTrack) {
                    head = &p->tracks[(size_t)this->route[0]];
                    for (size_t i = 1; i < route.size() - 2; ++i) {
                        head = &head->childNodes[(size_t)route[i]];
                    }

                    audioNode *newNode = nullptr;

                    if (this->route.size() >= 3)
                        newNode = &head->childNodes.emplace_back();
                    else
                        newNode = &p->tracks.emplace_back();

                    DBG("attempting to ungroup track. route.size() = "
                        << this->route.size());
                    tracklist->copyNode(newNode, getCorrespondingTrack());

                } else {
                    head = &p->tracks[(size_t)this->route[0]];
                    for (size_t i = 1; i < route.size() - 1; ++i) {
                        head = &head->childNodes[(size_t)route[i]];
                    }

                    if (this->route.size() >= 2) {
                        tracklist->deepCopyGroupInsideGroup(
                            getCorrespondingTrack(), head);

                    } else {
                        tracklist->deepCopyGroupInsideGroup(
                            getCorrespondingTrack(), nullptr);
                    }
                }

                tracklist->deleteTrack(this->route);

                DBG("ungrouping to parent named " << head->trackName);
            });

        contextMenu.addItem("Reset volume", [this] {
            getCorrespondingTrack()->gain = 1.f;
            gainSlider.setValue(1.f);
            repaint();
        });

        contextMenu.addItem("Reset pan", [this] {
            getCorrespondingTrack()->pan = 0.f;
            panSlider.setValue(0.f);
            repaint();
        });

        contextMenu.addSeparator();

        contextMenu.addItem(
            "Delete " + getCorrespondingTrack()->trackName, [this] {
                Tracklist *tracklist =
                    (Tracklist *)findParentComponentOfClass<Tracklist>();

                // tracklist->deleteTrack(this->siblingIndex);
                tracklist->deleteTrack(this->route);
            });

        if (getCorrespondingTrack()->isTrack == false) {
            contextMenu.addSeparator();

            contextMenu.addItem("Add child track", [this] {
                Tracklist *tracklist = findParentComponentOfClass<Tracklist>();
                tracklist->addNewNode(true, route);
            });

            contextMenu.addItem("Add child group", [this] {
                Tracklist *tracklist = findParentComponentOfClass<Tracklist>();
                tracklist->addNewNode(false, route);
            });
        }

        contextMenu.showMenuAsync(juce::PopupMenu::Options());
    }
}

void track::TrackComponent::mouseDrag(const juce::MouseEvent &event) {
    if (event.mouseWasDraggedSinceMouseDown() &&
        event.mods.isLeftButtonDown()) {
        Tracklist *tracklist = findParentComponentOfClass<Tracklist>();
        float mouseYInTracklist =
            event.getEventRelativeTo(tracklist).position.getY() -
            (UI_TRACK_HEIGHT / 2.f);
        int displayNodes = (int)(mouseYInTracklist / (float)(UI_TRACK_HEIGHT));

        tracklist->updateInsertIndicator(displayNodes);
    }
}

void track::TrackComponent::mouseUp(const juce::MouseEvent &event) {
    if (event.mouseWasDraggedSinceMouseDown()) {
        Tracklist *tracklist = findParentComponentOfClass<Tracklist>();
        tracklist->updateInsertIndicator(-1);

        float mouseYInTracklist =
            event.getEventRelativeTo(tracklist).position.getY() -
            (UI_TRACK_HEIGHT / 2.f);
        int displayNodes = (int)(mouseYInTracklist / (float)(UI_TRACK_HEIGHT));
        DBG("display nodes is " << displayNodes);

        // clang-format off
        if ((size_t)displayNodes >= tracklist->trackComponents.size() || displayNodes < 0)
            return;

        if (tracklist->trackComponents[(size_t)displayNodes]->getCorrespondingTrack()->isTrack) {
            AudioPluginAudioProcessor *p = (AudioPluginAudioProcessor *)processor;
      
            // check if both tracks belong to the same parent 
            std::vector<int> r1 = tracklist->trackComponents[(size_t)displayNodes]->route;
            int r1End = r1.back();
            r1.pop_back();

            std::vector<int> r2 = this->route;
            r2.pop_back();

            std::vector<int> sourceRoute = this->route;
            std::vector<int> movementRoute = tracklist->trackComponents[(size_t)displayNodes]->route;

            if (r1 == r2) {
                DBG("BOTH ARE SIBLINGS");

                //utility::reorderNode(r1, r2, route, r1End, displayNodes, processor);

                ActionReorderNode *action = new ActionReorderNode(sourceRoute, movementRoute, processor, tracklist, tracklist->timelineComponent);
                p->undoManager.beginNewTransaction("action reorder node");
                p->undoManager.perform(action);

                // utility::reorderNodeAlt(sourceRoute, movementRoute, processor);

                tracklist->trackComponents.clear();
                tracklist->createTrackComponents();
                tracklist->setTrackComponentBounds();

                track::TimelineComponent* timelineComponent = (TimelineComponent*)tracklist->timelineComponent;
                timelineComponent->updateClipComponents();
            }

        } else {
            tracklist->moveNodeToGroup(this, displayNodes);
                
            track::TimelineComponent* timelineComponent = (TimelineComponent*)tracklist->timelineComponent;
            timelineComponent->updateClipComponents();
        }

        // clang-format on
        DBG("Finished attempted node movement");
    }
}

void track::TrackComponent::paint(juce::Graphics &g) {
    juce::Colour bg = juce::Colour(0xFF'5F5F5F);
    juce::Colour groupBg = bg.brighter(0.2f);
    juce::Colour outline = juce::Colour(0xFF'535353);
    bool isFirstNodeInGroup = route.size() != 0 && siblingIndex == 0;

    int lineThickness = 1;
    float cornerSize = 5.f;
    g.fillAll(groupBg);

    juce::Rectangle<float> indentedBounds =
        getLocalBounds()
            .withTrimmedLeft(
                (((int)route.size() - 1) * UI_TRACK_DEPTH_INCREMENTS) +
                UI_TRACK_INDEX_WIDTH)
            .withWidth(UI_TRACK_WIDTH)
            .withHeight(UI_MAXIMUM_TRACK_HEIGHT + 10)
            .withY(0)
            .toFloat();
    if (isFirstNodeInGroup)
        indentedBounds.expand(-1, 0);

    if (getCorrespondingTrack()->isTrack) {
        g.setColour(bg);

        if (isFirstNodeInGroup) {
            g.fillRoundedRectangle(indentedBounds, cornerSize);
        } else {
            g.fillRect(indentedBounds);
        }
    } else {
        g.setColour(groupBg);
        g.fillRect(getLocalBounds().withTrimmedLeft(UI_TRACK_INDEX_WIDTH));
    }

    // outline
    g.setColour(outline);
    if (isFirstNodeInGroup)
        g.drawRoundedRectangle(indentedBounds, cornerSize, lineThickness);
    else
        g.drawRect(indentedBounds, lineThickness);

    // divide line between index number and actual track info
    g.setColour(bg);
    g.fillRect(0, 0, UI_TRACK_INDEX_WIDTH, getHeight());
    g.setColour(outline);
    g.drawRect(UI_TRACK_INDEX_WIDTH, 0, 2, getHeight(), 2);

    // depth markers
    g.setColour(outline.withAlpha(0.5f));
    for (int i = 0; i < (int)route.size() - 2; ++i) {
        g.drawRect((UI_TRACK_INDEX_WIDTH) +
                       ((i + 1) * UI_TRACK_DEPTH_INCREMENTS),
                   0, 2, getHeight(), 1);
    }

    g.setColour(juce::Colours::white.withAlpha(0.4f));
    g.setFont(
        getInterSemiBoldFromThisFunctionBecauseOtherwiseThisWillNotBuildOnWindowsForSomeStupidReason()
            .withHeight(17.f));
    g.drawText(juce::String(displayIndex + 1), 0, 0, UI_TRACK_INDEX_WIDTH,
               UI_TRACK_HEIGHT, juce::Justification::centred);

    // TODO: this isn't scalable. if you move buttons in resized() they
    // should be able to automatically reflect over here, but that isn't the
    // case draw mute marker
    if (getCorrespondingTrack()->m || getCorrespondingTrack()->s) {
        int btnSize =
            UI_TRACK_HEIGHT > UI_TRACK_HEIGHT_COLLAPSE_BREAKPOINT ? 24 : 18;

        juce::Rectangle<int> btnBounds = juce::Rectangle<int>(
            UI_TRACK_WIDTH - 130, (UI_TRACK_HEIGHT / 2) - (btnSize / 2),
            btnSize, btnSize);

        if (getCorrespondingTrack()->m)
            g.setColour(juce::Colour(0xFF'FACD51)); // yellow

        if (getCorrespondingTrack()->s) {
            btnBounds.setX(btnBounds.getX() + btnSize + 5);
            g.setColour(juce::Colour(0xFF'41C0FF)); // blue
        }

        btnBounds.expand(1, 1);
        g.drawRect(btnBounds);
    }
}

void track::TrackComponent::resized() {

    // trackNameLabel.setBounds((route.size() - 1) * 10, 0, getWidth(), 20);
    // return;

    int xOffset = (route.size() - 1) * UI_TRACK_DEPTH_INCREMENTS;
    juce::Rectangle<int> trackNameLabelBounds = juce::Rectangle<int>(
        UI_TRACK_INDEX_WIDTH + getLocalBounds().getX() + 5 + xOffset,
        (UI_TRACK_HEIGHT / 4) - 4, 100, 20);

    if (UI_TRACK_HEIGHT <= UI_TRACK_HEIGHT_COLLAPSE_BREAKPOINT + 10) {
        trackNameLabel.setFont(
            getAudioNodeLabelFont().withHeight(16.f).withExtraKerningFactor(
                -0.02f));
    } else {
        trackNameLabel.setFont(
            getAudioNodeLabelFont().withHeight(17.f).withExtraKerningFactor(
                -0.02f));
    }

    trackNameLabel.setBounds(trackNameLabelBounds);

    int btnSize =
        UI_TRACK_HEIGHT > UI_TRACK_HEIGHT_COLLAPSE_BREAKPOINT ? 24 : 18;
    int btnHeight = btnSize;
    int btnWidth = btnSize;

    juce::Rectangle<int> btnBounds = juce::Rectangle<int>(
        UI_TRACK_WIDTH - 130, (UI_TRACK_HEIGHT / 2) - (btnHeight / 2), btnWidth,
        btnHeight);

    muteBtn.setBounds(btnBounds);

    btnBounds.setX(btnBounds.getX() + btnSize + 5);

    soloBtn.setBounds(btnBounds);

    // set pan slider bounds
    float panSliderSizeMultiplier =
        UI_TRACK_HEIGHT > UI_TRACK_HEIGHT_COLLAPSE_BREAKPOINT ? 1.8f : 1.9f;

    juce::Rectangle<int> panSliderBounds = juce::Rectangle<int>(
        btnBounds.getX() + 20, btnBounds.getY() - 8,
        btnSize * panSliderSizeMultiplier, btnSize * panSliderSizeMultiplier);

    panSlider.setBounds(panSliderBounds);

    juce::Rectangle<int> fxBounds = juce::Rectangle<int>(
        panSliderBounds.getX() + 38, btnBounds.getY(), btnSize * 1.2f, btnSize);
    fxBtn.setBounds(fxBounds);

    if (UI_TRACK_HEIGHT >= UI_TRACK_HEIGHT_COLLAPSE_BREAKPOINT + 10) {
        gainSlider.setVisible(true);

        int sliderHeight = 20;
        int sliderWidth = 110;

        int sliderX = xOffset + UI_TRACK_INDEX_WIDTH + 4;

        // if you're nesting ridiclously many tracks, the gain slider overlaps
        // the mute button
        if (sliderX + sliderWidth > muteBtn.getX() - 4) {
            sliderWidth -= xOffset / 2;
        }

        gainSlider.setBounds(sliderX,
                             (UI_TRACK_HEIGHT / 2) - (sliderHeight / 2) +
                                 (int)(UI_TRACK_HEIGHT * .2f) + 2,
                             sliderWidth, sliderHeight);
    } else {
        gainSlider.setVisible(false);
        gainSlider.setBounds(1, 1, 1, 1);
    }
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

track::ActionCreateNode::ActionCreateNode(std::vector<int> pRoute,
                                          bool isATrack, void *tlist,
                                          void *processor)
    : juce::UndoableAction() {
    this->parentRoute = pRoute;
    this->isTrack = isATrack;
    this->tl = tlist;
    this->p = processor;
}
track::ActionCreateNode::~ActionCreateNode() {}
bool track::ActionCreateNode::perform() {
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    audioNode *parent = track::utility::getNodeFromRoute(parentRoute, p);
    audioNode *x = nullptr;
    if (parent == nullptr) {
        x = &processor->tracks.emplace_back();
    } else {
        x = &parent->childNodes.emplace_back();
    }

    x->isTrack = isTrack;
    x->processor = p;

    Tracklist *tracklist = (Tracklist *)tl;
    x->trackName = isTrack ? "Track" : "Group";
    x->trackName += " " + juce::String(tracklist->trackComponents.size() + 1);

    updateGUI();

    return true;
}

bool track::ActionCreateNode::undo() {
    // parent->childNodes.pop_back(); // wow pop_back() exists?
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    audioNode *parent = track::utility::getNodeFromRoute(parentRoute, p);

    if (parent == nullptr) {
        processor->tracks.pop_back();
    } else {
        parent->childNodes.pop_back();
    }

    updateGUI();

    return true;
}

void track::ActionCreateNode::updateGUI() {
    Tracklist *tracklist = (Tracklist *)tl;
    tracklist->trackComponents.clear();
    tracklist->createTrackComponents();
    tracklist->setTrackComponentBounds();
    tracklist->repaint();
}

track::ActionDeleteNode::ActionDeleteNode(std::vector<int> nodeRoute,
                                          void *processor, void *tracklist,
                                          void *timelineComponent)
    : juce::UndoableAction() {
    this->route = nodeRoute;
    this->p = processor;
    this->tl = tracklist;
    this->tc = timelineComponent;
}
track::ActionDeleteNode::~ActionDeleteNode() {}

bool track::ActionDeleteNode::perform() {
    DBG("ActionDeleteNode::perform() called");

    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    if (route.size() == 1) {

        // recursivelyDeleteNodePlugins(&processor->tracks[(size_t)route[0]]);

        if ((size_t)route[0] > processor->tracks.size()) {
            DBG("refusla to perform() action delete node");
        } else {

            utility::copyNode(&this->nodeCopy,
                              &processor->tracks[(size_t)route[0]], p);
            processor->tracks.erase(processor->tracks.begin() + route[0]);
        }

    } else {
        audioNode *head = &processor->tracks[(size_t)route[0]];
        for (size_t i = 1; i < route.size() - 1; ++i) {
            head = &head->childNodes[(size_t)route[i]];
        }

        // recursivelyDeleteNodePlugins(
        //&head->childNodes[route[route.size() - 1]]);

        if ((size_t)route.back() + 1 > head->childNodes.size()) {
            DBG("refusl to perform() action delete node");
        } else {

            utility::copyNode(
                &this->nodeCopy,
                &head->childNodes[(size_t)route[route.size() - 1]], p);

            head->childNodes.erase(head->childNodes.begin() +
                                   route[route.size() - 1]);
        }
    }

    updateGUI();

    return true;
}

bool track::ActionDeleteNode::undo() {
    DBG("ActionDeleteNode::undo() called");

    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    if (route.size() == 1) {
        processor->tracks.insert(processor->tracks.begin() + route[0],
                                 std::move(nodeCopy));
    } else {
        std::vector<int> parentRoute = route;
        parentRoute.resize(route.size() - 1);

        audioNode *parent = utility::getNodeFromRoute(parentRoute, processor);
        parent->childNodes.insert(parent->childNodes.begin() + route.back(),
                                  std::move(nodeCopy));
    }

    updateGUI();

    return true;
}

void track::ActionDeleteNode::updateGUI() {
    TimelineComponent *timelineComponent = (TimelineComponent *)tc;
    Tracklist *tracklist = (Tracklist *)tl;

    timelineComponent->clipComponents.clear();
    tracklist->trackComponents.clear();

    tracklist->createTrackComponents();
    tracklist->setTrackComponentBounds();

    timelineComponent->updateClipComponents();
}

track::ActionMoveNodeToGroup::ActionMoveNodeToGroup(std::vector<int> toMove,
                                                    std::vector<int> group,
                                                    void *processor,
                                                    void *tracklist,
                                                    void *timelineComponent) {
    this->nodeToMoveRoute = toMove;
    this->groupRoute = group;
    this->p = processor;
    this->tl = tracklist;
    this->tc = timelineComponent;
}
track::ActionMoveNodeToGroup::~ActionMoveNodeToGroup() {}

bool track::ActionMoveNodeToGroup::perform() {
    DBG("perform(): groupRoute = " << utility::prettyVector(groupRoute));
    DBG("perform(): nodeToMoveRoute = "
        << utility::prettyVector(nodeToMoveRoute));
    audioNode *groupNode = utility::getNodeFromRoute(groupRoute, p);

    routeAfterMoving = groupRoute;

    size_t commonGeneration = 0;
    for (size_t i = 0; i < jmin(nodeToMoveRoute.size(), groupRoute.size());
         ++i) {
        if (nodeToMoveRoute[i] != groupRoute[i]) {
            commonGeneration = i;
            break;
        }
    }

    DBG("commonGeneration = " << commonGeneration);

    if (nodeToMoveRoute[commonGeneration] < groupRoute[commonGeneration]) {
        routeAfterMoving[(size_t)commonGeneration]--;
    }

    /*
    // aausdf
    if (nodeToMoveRoute.size() == 1 && nodeToMoveRoute.back() < groupRoute[0])
        routeAfterMoving[0]--;*/

    /*if (nodeToMoveRoute.size() == groupRoute.size() &&
        nodeToMoveRoute[nodeToMoveRoute.size() - 1] <
            groupRoute[groupRoute.size() - 1]) {
        routeAfterMoving.back()--;
    }

    routeAfterMoving.push_back(groupNode->childNodes.size());*/

    routeAfterMoving.push_back(groupNode->childNodes.size());

    utility::moveNodeToGroup(nodeToMoveRoute, groupRoute, (Tracklist *)tl, p);

    if (nodeToMoveRoute.size() > 1 &&
        utility::isSibling(groupRoute, nodeToMoveRoute)) {
    }

    updateGUI();

    return true;
}

bool track::ActionMoveNodeToGroup::undo() {
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;

    bool fixEdgeCase = true;
    bool edgeCase = true;

    if (edgeCase) {
        if (fixEdgeCase) {
            DBG("nodeToMoveRoute = " << utility::prettyVector(nodeToMoveRoute));
            DBG("groupRoute = " << utility::prettyVector(groupRoute));
            DBG("routeAfterMoving = "
                << utility::prettyVector(routeAfterMoving));

            // retrieving the node to move uses the same logic, regardless of
            // that node's parent's existence

            DBG("fuck 1.1");
            audioNode *nodeToMove =
                utility::getNodeFromRoute(routeAfterMoving, p);
            audioNode tmp;
            utility::copyNode(&tmp, nodeToMove, p);

            if (nodeToMoveRoute.size() == 1) {
                utility::deleteNode(routeAfterMoving, p);
                auto &newborn = *processor->tracks.emplace(
                    processor->tracks.begin() + nodeToMoveRoute.back());

                utility::copyNode(&newborn, &tmp, p);

            } else {
                DBG("fuck 2.1");
                audioNode *originalParent =
                    utility::getParentFromRoute(nodeToMoveRoute, p);

                DBG("fuck 2.2");
                utility::deleteNode(routeAfterMoving, p);

                DBG("fuck 2.3");
                auto &newborn = *originalParent->childNodes.emplace(
                    originalParent->childNodes.begin() +
                    nodeToMoveRoute.back());

                DBG("fuck 2.4");
                utility::copyNode(&newborn, &tmp, p);
            }

            updateGUI();
            return true;
        }
    }

    audioNode *parent = utility::getNodeFromRoute(groupRoute, p);

    if (nodeToMoveRoute.size() == 1) {
        processor->tracks.emplace(processor->tracks.begin() +
                                  nodeToMoveRoute[0]);

        utility::copyNode(&processor->tracks[(size_t)nodeToMoveRoute[0]],
                          &parent->childNodes.back(), p);

        parent->childNodes.erase(parent->childNodes.begin() +
                                 (long)parent->childNodes.size() - 1);
    } else {
        std::vector<int> originalParentRoute = nodeToMoveRoute;
        originalParentRoute.resize(nodeToMoveRoute.size() - 1);

        // insert slot for node in its old position
        audioNode *originalParent =
            utility::getNodeFromRoute(originalParentRoute, p);

        originalParent->childNodes.emplace(originalParent->childNodes.begin() +
                                           nodeToMoveRoute.back());

        // copy data and remove node from new group
        utility::copyNode(
            &originalParent->childNodes[(size_t)nodeToMoveRoute.back()],
            &parent->childNodes.back(), p);

        parent->childNodes.erase(parent->childNodes.begin() +
                                 (long)parent->childNodes.size() - 1);
    }

    updateGUI();

    return true;
}

void track::ActionMoveNodeToGroup::updateGUI() {
    TimelineComponent *timelineComponent = (TimelineComponent *)tc;
    Tracklist *tracklist = (Tracklist *)tl;

    timelineComponent->clipComponents.clear();
    tracklist->trackComponents.clear();

    tracklist->createTrackComponents();
    tracklist->setTrackComponentBounds();

    timelineComponent->updateClipComponents();
}

track::ActionReorderNode::ActionReorderNode(std::vector<int> route1,
                                            std::vector<int> route2,
                                            void *processor, void *tracklist,
                                            void *timelineComponent) {
    this->r1 = route1;
    this->r2 = route2;

    this->p = processor;
    this->tl = tracklist;
    this->tc = timelineComponent;
}
track::ActionReorderNode::~ActionReorderNode() {}

bool track::ActionReorderNode::perform() {
    utility::reorderNodeAlt(r1, r2, p);
    updateGUI();
    return true;
}

bool track::ActionReorderNode::undo() {
    utility::reorderNodeAlt(r1, r2, p);
    updateGUI();
    return true;
}

void track::ActionReorderNode::updateGUI() {
    TimelineComponent *timelineComponent = (TimelineComponent *)tc;
    Tracklist *tracklist = (Tracklist *)tl;

    timelineComponent->clipComponents.clear();
    tracklist->trackComponents.clear();

    tracklist->createTrackComponents();
    tracklist->setTrackComponentBounds();

    timelineComponent->updateClipComponents();
}

track::Tracklist::Tracklist() : juce::Component() {
    addAndMakeVisible(newTrackBtn);
    newTrackBtn.setButtonText("ADD TRACK");
    newTrackBtn.setTooltip("create new track");

    addAndMakeVisible(newGroupBtn);
    newGroupBtn.setButtonText("ADD GROUP");

    addAndMakeVisible(unmuteAllBtn);
    unmuteAllBtn.setButtonText("UNMUTE ALL");
    unmuteAllBtn.onClick = [this] {
        for (auto &tc : trackComponents) {
            tc->getCorrespondingTrack()->m = false;
        }

        repaint();
    };

    addAndMakeVisible(unsoloAllBtn);
    unsoloAllBtn.setButtonText("UNSOLO ALL");
    unsoloAllBtn.onClick = [this] {
        for (auto &tc : trackComponents) {
            tc->getCorrespondingTrack()->s = false;
        }

        AudioPluginAudioProcessor *p = (AudioPluginAudioProcessor *)processor;
        p->soloMode = false;

        repaint();
    };

    newTrackBtn.onClick = [this] { addNewNode(true, std::vector<int>()); };
    newGroupBtn.onClick = [this] { addNewNode(false, std::vector<int>()); };

    addAndMakeVisible(insertIndicator);
}
track::Tracklist::~Tracklist() {}

void track::Tracklist::copyNode(audioNode *dest, audioNode *src) {
    utility::copyNode(dest, src, processor);
}

void track::Tracklist::addNewNode(bool isTrack, std::vector<int> parentRoute) {
    AudioPluginAudioProcessor *p = (AudioPluginAudioProcessor *)this->processor;
    /*
    audioNode *newNode = nullptr;
    if (parent == nullptr) {
        newNode = &p->tracks.emplace_back();
    } else {
        newNode = &parent->childNodes.emplace_back();
    }

    newNode->processor = p;
    newNode->isTrack = isTrack;
    newNode->trackName = isTrack ? "Track" : "Group";
    newNode->trackName += " " + juce::String(trackComponents.size() + 1);
    */

    ActionCreateNode *action =
        new ActionCreateNode(parentRoute, isTrack, this, p);
    p->undoManager.beginNewTransaction("action create node");
    p->undoManager.perform(action);
}

bool track::Tracklist::isDescendant(audioNode *parent, audioNode *possibleChild,
                                    bool directDescandant) {
    for (audioNode &child : parent->childNodes) {
        if (directDescandant) {
            if (&child == possibleChild)
                return true;
            continue;
        }

        if (&child == possibleChild ||
            isDescendant(&child, possibleChild, false))
            return true;
    }

    return false;
}

void track::Tracklist::recursivelyDeleteNodePlugins(audioNode *node) {
    if (!node->isTrack) {
        for (audioNode &child : node->childNodes) {
            recursivelyDeleteNodePlugins(&child);
        }
    }

    for (auto &plugin : node->plugins) {
        /*
        if (plugin->getActiveEditor() != nullptr) {
            DBG("deleting a node which has a plugin with an EDITOR WHCIH IS
        " "STILL ACTIVE");
            // plugin->editorBeingDeleted(plugin->getActiveEditor());
            delete plugin->getActiveEditor();
        }
        */

        /*
        DBG("recursivelyDeleteNodePlugins(): deleting a plugin: "
            << plugin->getPluginDescription().name);
        plugin->releaseResources();
        */
    }
}

// vokd track::Tracklist::deleteTrack(int trackIndex) {
void track::Tracklist::deleteTrack(std::vector<int> route) {
    AudioPluginAudioProcessor *p = (AudioPluginAudioProcessor *)processor;
    ActionDeleteNode *action =
        new ActionDeleteNode(route, processor, this, timelineComponent);

    DBG("perform()ing node deletion");

    p->undoManager.beginNewTransaction("action delete node");
    p->undoManager.perform(action);

    /*
    TimelineComponent *tc = (TimelineComponent *)this->timelineComponent;
    tc->clipComponents.clear();

    juce::MessageManagerLock mmlock;
    AudioPluginAudioProcessor *p = (AudioPluginAudioProcessor *)processor;

    if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
        DBG("THIS IS THE MESSAGE THREAD");
    }

    // trivially delete
    if (route.size() == 1) {
        DBG("trivially deleted audo node with index: "
            << getPrettyVector(route));

        recursivelyDeleteNodePlugins(&p->tracks[(size_t)route[0]]);
        p->tracks.erase(p->tracks.begin() + route[0]);
    } else {
        audioNode *head = &p->tracks[(size_t)route[0]];
        for (size_t i = 1; i < route.size() - 1; ++i) {
            head = &head->childNodes[(size_t)route[i]];
        }

        recursivelyDeleteNodePlugins(
            &head->childNodes[route[route.size() - 1]]);
        head->childNodes.erase(head->childNodes.begin() +
                               route[route.size() - 1]);
    }

    this->trackComponents.clear();
    this->createTrackComponents();
    this->setTrackComponentBounds();
    tc->updateClipComponents();*/
}

void track::Tracklist::deepCopyGroupInsideGroup(audioNode *childNode,
                                                audioNode *parentNode) {
    for (auto &child : childNode->childNodes) {
        audioNode *newNode = nullptr;

        if (parentNode == nullptr) {
            DBG("parentNode = nullptr");
            AudioPluginAudioProcessor *p =
                (AudioPluginAudioProcessor *)processor;
            newNode = &p->tracks.emplace_back();
        } else
            newNode = &parentNode->childNodes.emplace_back();
        /*
        newNode->trackName = child.trackName;
        newNode->isTrack = child.isTrack;
        newNode->gain = child.gain;
        newNode->processor = processor;

        if (newNode->isTrack)
            newNode->clips = childNode->clips;
        else {
            deepCopyGroupInsideGroup(&child, newNode);
        }
        */

        copyNode(newNode, &child);
        continue;

        // now the annoying thing, copying plugins. (which is why you can't
        // just push_back() trackNode into groupNode->childNodes)
        for (auto &p : child.plugins) {
            /*
            if (pluginInstance->getActiveEditor() != nullptr) {
                DBG("pluginInstance's active editor still exists");
                delete pluginInstance->getActiveEditor();
            }
            */

            auto &pluginInstance = p->plugin;
            // retrieve data then free up original plugin instance's
            // resources
            juce::MemoryBlock pluginData;
            pluginInstance->getStateInformation(pluginData);
            pluginInstance->releaseResources();

            juce::String identifier =
                pluginInstance->getPluginDescription().fileOrIdentifier;

            identifier = identifier.upToLastOccurrenceOf(".vst3", true, true);

            // add plugin to new node and copy data
            DBG("adding plugin to new node, using identifier " << identifier);
            newNode->addPlugin(identifier);
            newNode->plugins[newNode->plugins.size() - 1]
                ->plugin->setStateInformation(pluginData.getData(),
                                              pluginData.getSize());
        }
    }
}

void track::Tracklist::moveNodeToGroup(track::TrackComponent *caller,
                                       int targetIndex) {

    if (targetIndex < 0 || (size_t)targetIndex > trackComponents.size() - 1 ||
        caller->displayIndex == targetIndex) {
        DBG("rejecting node movement");
        return;
    }

    std::vector<int> toMove = caller->route;
    std::vector<int> group = trackComponents[(size_t)targetIndex]->route;

    ActionMoveNodeToGroup *action = new ActionMoveNodeToGroup(
        toMove, group, processor, this, timelineComponent);

    AudioPluginAudioProcessor *p = (AudioPluginAudioProcessor *)processor;
    p->undoManager.beginNewTransaction("action move node to group");
    p->undoManager.perform(action);

    // utility::moveNodeToGroup(caller, targetIndex, this, processor);

    return;

    /*
    if (targetIndex < 0 || (size_t)targetIndex > trackComponents.size() - 1 ||
        caller->displayIndex == targetIndex)
        return;

    audioNode *trackNode = caller->getCorrespondingTrack();

    auto &nodeComponentToMoveTo = this->trackComponents[(size_t)targetIndex];

    audioNode *parentNode = nodeComponentToMoveTo->getCorrespondingTrack();
    if (parentNode->isTrack) {
        DBG("rejecting moving track inside a track");
        return;
    }

    bool x = isDescendant(parentNode, trackNode, true);
    if (x) {
        return;
    }

    bool y = isDescendant(trackNode, parentNode, false);
    if (y) {
        DBG("SECOND DESCENDANCY CHECK FAILED");
        return;
    }

    audioNode *newNode = &parentNode->childNodes.emplace_back();
    std::vector<int> routeCopy = caller->route;

    // calling copyNode() should take care of all the gobbledygook below
    copyNode(newNode, trackNode);
*/
    /*
    newNode->trackName = trackNode->trackName;
    newNode->gain = trackNode->gain;
    newNode->isTrack = trackNode->isTrack;
    newNode->processor = processor;

    if (trackNode->isTrack)
        newNode->clips = trackNode->clips;
    else
        deepCopyGroupInsideGroup(trackNode, newNode);

    // now the annoying thing, copying plugins. (which is why you can't just
    // push_back() trackNode into groupNode->childNodes)
    for (auto &pluginInstance : trackNode->plugins) {
        if (pluginInstance->getActiveEditor() != nullptr) {
            DBG("pluginInstance's active editor still exists");
            delete pluginInstance->getActiveEditor();
        }

        juce::MemoryBlock pluginData;
        pluginInstance->getStateInformation(pluginData);
        pluginInstance->releaseResources();

        juce::String identifier =
            pluginInstance->getPluginDescription().fileOrIdentifier;

        // ABSOLUTE CINEMA.
        identifier = identifier.upToLastOccurrenceOf(".vst3", true, true);

        DBG("adding plugin to new node, using identifier " << identifier);
        newNode->addPlugin(identifier);
        newNode->plugins[newNode->plugins.size() - 1]->setStateInformation(
            pluginData.getData(), pluginData.getSize());
    }

    // node is copied. now delete orginal node
    AudioPluginAudioProcessor *p = (AudioPluginAudioProcessor *)processor;
    jassert(caller->route.size() > 0);

    if (routeCopy.size() == 1) {
        DBG("deleting orphan node with index " << routeCopy[0]);
        for (auto &pluginInstance : p->tracks[(size_t)routeCopy[0]].plugins) {
            AudioProcessorEditor *subpluginEditor =
                pluginInstance->plugin->getActiveEditor();
            if (subpluginEditor != nullptr) {
                pluginInstance->plugin->editorBeingDeleted(subpluginEditor);
            }
        }

        p->tracks.erase(p->tracks.begin() + routeCopy[0]); // orphan
    } else {
        audioNode *head = &p->tracks[(size_t)routeCopy[0]];
        for (size_t i = 1; i < routeCopy.size() - 1; ++i) {
            head = &head->childNodes[(size_t)routeCopy[i]];
        }

        head->childNodes.erase(head->childNodes.begin() +
                               routeCopy[routeCopy.size() - 1]);
    }

    trackComponents.clear();
    createTrackComponents();
    setTrackComponentBounds();
*/
}

void track::Tracklist::mouseDown(const juce::MouseEvent &event) {
    if (event.mods.isRightButtonDown()) {
        juce::PopupMenu contextMenu;
        contextMenu.setLookAndFeel(&getLookAndFeel());

        contextMenu.addItem("Add new track", [this] { this->addNewNode(); });
        contextMenu.addItem("Add new group",
                            [this] { this->addNewNode(false); });

        contextMenu.showMenuAsync(juce::PopupMenu::Options());
    }
}

int track::Tracklist::findChildren(audioNode *parentNode,
                                   std::vector<int> route, int foundItems,
                                   int depth) {
    for (size_t i = 0; i < parentNode->childNodes.size(); ++i) {
        juce::String tabs;
        for (int x = 0; x < depth; ++x) {
            tabs.append("    ", 4);
        }

        audioNode *childNode = &parentNode->childNodes[i];

        route.push_back(i);

        this->trackComponents.push_back(std::make_unique<TrackComponent>(i));
        trackComponents.back().get()->processor = processor;
        trackComponents.back().get()->route = route;
        trackComponents.back().get()->initializSliders();
        trackComponents.back().get()->trackNameLabel.setText(
            childNode->trackName, juce::NotificationType::dontSendNotification);

        addAndMakeVisible(*trackComponents.back());

        /*
        DBG(tabs << "found " << (childNode->isTrack ? "track" : "group") <<
        " "
                 << childNode->trackName << "(" << i << foundItems << depth
                 << ") " << getPrettyVector(route));
                 */

        if (!childNode->isTrack) {
            foundItems = findChildren(childNode, route, foundItems, depth + 1);
        } else {
        }

        ++foundItems;
        route.pop_back();
    }

    return foundItems;
}

void track::Tracklist::setDisplayIndexes() {
    for (size_t i = 0; i < this->trackComponents.size(); ++i) {
        this->trackComponents[i]->displayIndex = i;
    }
}

void track::Tracklist::createTrackComponents() {
    AudioPluginAudioProcessor *p =
        (AudioPluginAudioProcessor *)(this->processor);

    int j = 0;
    int foundItems = 0;
    for (audioNode &t : p->tracks) {
        std::vector<int> route;
        route.push_back(j);

        this->trackComponents.push_back(std::make_unique<TrackComponent>(j));
        trackComponents.back().get()->processor = processor;
        trackComponents.back().get()->route = route;
        trackComponents.back().get()->initializSliders();
        trackComponents.back().get()->trackNameLabel.setText(
            t.trackName, juce::NotificationType::dontSendNotification);
        addAndMakeVisible(*trackComponents.back());

        // DBG("found " << t.trackName << " (root node)");
        foundItems = findChildren(&t, route, foundItems, 1);

        trackComponents.back().get()->displayIndex = foundItems;

        j++;
    }

    setDisplayIndexes();
    setTrackComponentBounds();
    repaint();

    // ee
}

void track::Tracklist::setTrackComponentBounds() {
    int counter = 0;
    for (auto &tc : trackComponents) {
        tc->setBounds(0,
                      UI_TRACK_VERTICAL_OFFSET + (UI_TRACK_HEIGHT * counter) +
                          (UI_TRACK_VERTICAL_MARGIN * counter),
                      UI_TRACK_WIDTH, UI_TRACK_HEIGHT);

        counter++;
        tc->resized();
        tc->repaint();
    }

    int btnWidth = 70;
    int btnMargin = 4;
    int xOffset = 0;
    newTrackBtn.setBounds(xOffset, 0, btnWidth, UI_TRACK_VERTICAL_OFFSET);
    newGroupBtn.setBounds(xOffset + btnWidth + btnMargin, 0, btnWidth,
                          UI_TRACK_VERTICAL_OFFSET);
    unsoloAllBtn.setBounds(xOffset + (2 * (btnWidth + btnMargin)), 0, btnWidth,
                           UI_TRACK_VERTICAL_OFFSET);
    unmuteAllBtn.setBounds(xOffset + (3 * (btnWidth + btnMargin)), 0, btnWidth,
                           UI_TRACK_VERTICAL_OFFSET);
    // DBG("setTrackComponentBounds() called");

    if (getParentComponent()) {
        int newTracklistHeight = juce::jmax((counter + 2) * UI_TRACK_HEIGHT,
                                            getParentComponent()->getHeight());
        this->setSize(getWidth(), newTracklistHeight);
    }

    repaint();

    if (getParentComponent())
        getParentComponent()->repaint();
}

void track::Tracklist::updateInsertIndicator(int index) {
    if (index > -1) {
        this->insertIndicator.setVisible(true);
        this->insertIndicator.toFront(false);
        this->insertIndicator.setBounds(
            0, (UI_TRACK_HEIGHT * index) + UI_TRACK_VERTICAL_OFFSET,
            UI_TRACK_WIDTH, 1);
        repaint();
        return;
    }

    this->insertIndicator.setVisible(false);
}

void track::subplugin::relayParamsToPlugin() {
    for (size_t i = 0; i < relayParams.size(); ++i) {
        relayParam *rp = &this->relayParams[i];

        if (rp->outputParamID == -1 || rp->pluginParamIndex == -1)
            continue;

        // get hosted plugin's param
        juce::RangedAudioParameter *rawPluginParam =
            (juce::RangedAudioParameter *)this->plugin->getHostedParameter(
                rp->pluginParamIndex - 1);

        jassert(rawPluginParam != nullptr);

        // get percentage from processor params
        AudioPluginAudioProcessor *p = (AudioPluginAudioProcessor *)processor;
        jassert(p != nullptr);

        juce::RangedAudioParameter *relayParameterPtr =
            (juce::RangedAudioParameter *)
                p->getParameters()[rp->outputParamID + 1];
        float percentage = relayParameterPtr->getValue();

        rp->percentage = percentage;

        rawPluginParam->setValue(percentage);
    }
}

void track::subplugin::process(juce::AudioBuffer<float> &buffer) {
    juce::MidiBuffer mb;

    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);

    this->plugin->processBlock(buffer, mb);

    float dryMix = 1.f - dryWetMix;
    float wetMix = dryWetMix;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        auto *dry = dryBuffer.getReadPointer(ch);
        auto *wet = buffer.getReadPointer(ch);
        auto *out = buffer.getWritePointer(ch);

        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            out[i] = dry[i] * dryMix + wet[i] * wetMix;
        }
    }
}

void track::subplugin::initializePlugin(juce::String path) {
    DBG("please kill me");
    juce::OwnedArray<PluginDescription> pluginDescriptions;
    juce::KnownPluginList plist;
    juce::AudioPluginFormatManager apfm;
    juce::String errorMsg;

    DBG(track::SAMPLE_RATE);
    DBG(track::SAMPLES_PER_BLOCK);

    if (track::SAMPLE_RATE < 0) {
        track::SAMPLE_RATE = 44100;
    }

    apfm.addDefaultFormats();

    // TODO: handle failure to scan plugin
    for (int i = 0; i < apfm.getNumFormats(); ++i)
        plist.scanAndAddFile(path, true, pluginDescriptions,
                             *apfm.getFormat(i));

    jassert(pluginDescriptions.size() > 0);

    plugin =
        apfm.createPluginInstance(*pluginDescriptions[0], track::SAMPLE_RATE,
                                  track::SAMPLES_PER_BLOCK, errorMsg);

    plugin->setPlayConfigDetails(2, 2, track::SAMPLE_RATE,
                                 track::SAMPLES_PER_BLOCK);
    if (track::SAMPLES_PER_BLOCK <= 0) {
        DBG("setting maxSamplesPerBlock to 512");
        track::SAMPLES_PER_BLOCK = 512;
    }

    plugin->prepareToPlay(track::SAMPLE_RATE, track::SAMPLES_PER_BLOCK);
}
track::subplugin::subplugin() : plugin() {}
track::subplugin::~subplugin() {}

// TODO: update this function to take care of more advanced audio clip
// operations (like trimming, and offsetting)
void track::clip::updateBuffer() {
    juce::File file(path);

    if (!file.exists()) {
        buffer.setSize(0, 0);
        DBG("updateBuffer() called--file " << path << " does not exist");
        return;
    }

    juce::AudioFormatManager afm;
    afm.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(afm.createReaderFor(file));
    buffer = juce::AudioBuffer<float>((int)reader->numChannels,
                                      reader->lengthInSamples);

    reader->read(&buffer, 0, buffer.getNumSamples(), 0, true, true);
}

void track::clip::reverse() { buffer.reverse(0, buffer.getNumSamples()); }

void track::audioNode::addPlugin(juce::String path) {
    jassert(processor != nullptr);
    AudioPluginAudioProcessor *p = (AudioPluginAudioProcessor *)processor;

    DBG("calling initializePlugin()");

    plugins.push_back(std::make_unique<subplugin>());
    plugins.back()->initializePlugin(path);
    plugins.back()->processor = processor;

    p->updateLatencyAfterDelay();
}

void track::audioNode::removePlugin(int index) {
    this->plugins.erase(this->plugins.begin() + index);

    AudioPluginAudioProcessor *p = (AudioPluginAudioProcessor *)processor;
    p->updateLatencyAfterDelay();
}

int track::audioNode::getLatencySamples() {
    int retval = 0;

    for (auto &pluginInstance : plugins) {
        retval += pluginInstance->plugin->getLatencySamples();
    }

    return retval;
}

int track::audioNode::getTotalLatencySamples() {
    int retval = 0;

    retval += this->getLatencySamples();

    for (audioNode &node : this->childNodes) {
        retval += node.getTotalLatencySamples();
    }

    if (retval > MAX_LATENT_SAMPLES)
        MAX_LATENT_SAMPLES = retval;

    this->latency = retval;
    return retval;
}

void track::audioNode::preparePlugins() {
    for (auto &p : plugins) {
        p->plugin->prepareToPlay(track::SAMPLE_RATE, track::SAMPLES_PER_BLOCK);
    }
}

void track::audioNode::process(int numSamples, int currentSample) {
    AudioPluginAudioProcessor *p = (AudioPluginAudioProcessor *)processor;

    if (this->m || (p->soloMode && !this->s)) {
        buffer.clear();
        return;
    }

    if (buffer.getNumSamples() != numSamples)
        buffer.setSize(2, numSamples, false, false, true);

    buffer.clear();

    int outputBufferLength = numSamples;
    int totalNumInputChannels = 2;

    int latencyOffset = MAX_LATENT_SAMPLES - this->latency;
    currentSample -= latencyOffset;

    if (isTrack) {
        // add sample data to buffer
        for (clip &c : clips) {
            if (!c.active)
                continue;

            int clipStart = c.startPositionSample;
            int clipEnd = c.startPositionSample + c.buffer.getNumSamples();
            int clipUsableNumSamples = c.buffer.getNumSamples() - c.trimLeft;

            if (clipUsableNumSamples <= 0)
                continue;

            // bounds check
            if (clipEnd > currentSample &&
                clipStart < currentSample + outputBufferLength) {

                /*
                DBG("clip in bounds: "
                    << c.name << "; clipstart: " << c.startPositionSample
                    << "; trimleft=" << c.trimLeft << ";
                clipstart+trimleft="
                    << c.startPositionSample + c.trimLeft
                    << "; cursample=" << currentSample
                    << "; clipUsableNumSamples=" << clipUsableNumSamples);
                    */

                // where in buffer should clip start?
                int outputOffset =
                    (clipStart < currentSample) ? 0 : clipStart - currentSample;
                // starting point in clip's buffer?
                int clipBufferStart =
                    c.trimLeft + ((clipStart < currentSample)
                                      ? currentSample - clipStart
                                      : 0);
                ++clipBufferStart; // avoid doing this the "proper" way;
                                   // besides 1 sample doesn't matter
                // how many samples can we safely copy?
                int samplesToCopy =
                    juce::jmin(outputBufferLength - outputOffset,
                               c.buffer.getNumSamples() - c.trimRight -
                                   clipBufferStart - 1);

                if (samplesToCopy <= 0 || clipBufferStart < 0)
                    continue;

                if (c.buffer.getNumChannels() > 1) {
                    for (int channel = 0; channel < buffer.getNumChannels();
                         ++channel) {
                        buffer.addFrom(channel, outputOffset, c.buffer,
                                       channel % totalNumInputChannels,
                                       clipBufferStart, samplesToCopy);
                    }
                }

                else {
                    for (int channel = 0; channel < buffer.getNumChannels();
                         ++channel) {
                        buffer.addFrom(channel, outputOffset, c.buffer, 0,
                                       clipBufferStart, samplesToCopy);
                    }
                }

                buffer.applyGain(c.gain);
            }
        }
    } else {
        // let child tracks process
        for (audioNode &child : this->childNodes) {
            child.process(numSamples, currentSample);
        }

        // sum up buffers
        for (track::audioNode &t : childNodes) {
            int totalNumOutputChannels = 2;
            for (int channel = 0; channel < totalNumOutputChannels; ++channel) {

                buffer.addFrom(channel, 0, t.buffer, channel, 0,
                               buffer.getNumSamples());
            }
        }
    }

    // let subplugins process audio
    for (size_t i = 0; i < this->plugins.size(); ++i) {
        if (this->plugins[i] == nullptr)
            return;

        if (this->plugins[i]->bypassed == true)
            continue;

        juce::MidiBuffer mb;
        this->plugins[i]->relayParamsToPlugin();
        this->plugins[i]->process(this->buffer);
    }

    // pan audio
    float normalisedPan = (0.5f) * (pan + 1.f);

    float l = juce::jmin(0.5f, 1.f - normalisedPan);
    float r = juce::jmin(0.5f, normalisedPan);
    float boost = 2.f;

    buffer.applyGain(0, 0, buffer.getNumSamples(), l * boost);
    buffer.applyGain(1, 0, buffer.getNumSamples(), r * boost);

    // main audio processing is done; add gain as final step
    buffer.applyGain(gain);
}
