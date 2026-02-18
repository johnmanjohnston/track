#include "track.h"
#include "../editor.h"
#include "../processor.h"
#include "automation_relay.h"
#include "clipboard.h"
#include "defs.h"
#include "subwindow.h"
#include "timeline.h"
#include "utility.h"
#include <cstddef>

track::ClipComponent::ClipComponent(clip *c, int clipHash)
    : juce::Component(), thumbnailCache(5),
      thumbnail(256, afm, thumbnailCache) {
    jassert(c != nullptr);

    this->correspondingClip = c;
    thumbnail.setSource(&correspondingClip->buffer, SAMPLE_RATE, clipHash);

    thumbnail.addChangeListener(this);

    clipNameLabel.setFont(
        getInterSemiBold().withHeight(17.f).withExtraKerningFactor(-.02f));
    clipNameLabel.setColour(juce::Label::textColourId,
                            juce::Colour(0xFF'AECBED).brighter(.5f));
    clipNameLabel.setEditable(true);
    clipNameLabel.setText(c->name,
                          juce::NotificationType::dontSendNotification);
    clipNameLabel.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(clipNameLabel);

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

        correspondingClip->name = clipNameLabel.getText(true);

        action->newClip.name = correspondingClip->name;

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

void track::ClipComponent::changeListenerCallback(
    ChangeBroadcaster * /*source*/) {
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
            juce::Colour base = correspondingClip->active
                                    ? juce::Colour(0xFF'33587F)
                                    : juce::Colour(0x00'2D2D2D);

            if (coolColors)
                base = juce::Colour(0xFF'8EBCD0);

            if (this->hasKeyboardFocus(true))
                g.setColour(base.brighter(0.3f).withMultipliedSaturation(1.2f));
            else if (isMouseOver(false))
                g.setColour(base.brighter(0.1f).withAlpha(1.f));
            else
                g.setColour(base);

            g.fillRoundedRectangle(getLocalBounds().toFloat(), cornerSize);
        }

        if (this->correspondingClip->active) {
            g.setColour(juce::Colour(0xFFAECBED));
        } else
            g.setColour(juce::Colour(0xFF'696969)); // nice

        if (coolColors)
            g.setColour(juce::Colour(0xFF'16455B));

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
}

void track::ClipComponent::copyClip() {
    // create new clip and set clipboard's data as this new clip
    // also, by creating a copy of the clip we allow the user to
    // delete the orginal clip from timeline, and still paste it
    track::clip *newClip = new track::clip;

    *newClip = *correspondingClip;

    clipboard::setData(newClip, TYPECODE_CLIP);

    coolColors = true;
    repaint();
    juce::Timer::callAfterDelay(UI_VISUAL_FEEDBACK_FLASH_DURATION_MS, [this] {
        this->coolColors = false;
        repaint();
    });
}

void track::ClipComponent::mouseDown(const juce::MouseEvent &event) {
    if (event.mods.isRightButtonDown()) {
        juce::PopupMenu contextMenu;
        contextMenu.setLookAndFeel(&getLookAndFeel());

#define MENU_CUT_CLIP 2
#define MENU_TOGGLE_CLIP_ACTIVATION 3
#define MENU_COPY_CLIP 4
#define MENU_SHOW_IN_EXPLORER 5
#define MENU_SPLIT_CLIP 6
#define MENU_RENAME_CLIP 7
#define MENU_DUPLICATE_CLIP_IMMEDIATELY_AFTER 8
#define MENU_EDIT_CLIP_PROPERTIES 9

        contextMenu.addItem(MENU_RENAME_CLIP, "Rename clip");
        contextMenu.addItem(MENU_SPLIT_CLIP, "Split clip");
        contextMenu.addItem(MENU_DUPLICATE_CLIP_IMMEDIATELY_AFTER,
                            "Duplicate clip");
        contextMenu.addItem(MENU_COPY_CLIP, "Copy clip");
        contextMenu.addItem(MENU_CUT_CLIP, "Cut");
        contextMenu.addSeparator();
        contextMenu.addItem(MENU_TOGGLE_CLIP_ACTIVATION,
                            "Toggle activate/deactive clip");
        contextMenu.addItem(MENU_SHOW_IN_EXPLORER, "Show in explorer");
        contextMenu.addItem(MENU_EDIT_CLIP_PROPERTIES, "Edit clip properties");

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

            else if (result == MENU_DUPLICATE_CLIP_IMMEDIATELY_AFTER) {
                std::unique_ptr<clip> newClip(new clip());
                *newClip = *correspondingClip;

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
                std::vector<int> route =
                    tracklist->trackComponents[(size_t)this->nodeDisplayIndex]
                        ->route;

                track::TimelineComponent *tc = (track::TimelineComponent *)
                    findParentComponentOfClass<TimelineComponent>();
                jassert(tc != nullptr);

                ActionAddClip *action =
                    new ActionAddClip(*newClip.get(), route, tc);
                tc->processorRef->undoManager.beginNewTransaction(
                    "action add clip (duplicate clip immediately after)");
                tc->processorRef->undoManager.perform(action);
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
                copyClip();
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

// cs exam tomorrow but fuck it the curriculum is full of shit
void track::ClipComponent::mouseDrag(const juce::MouseEvent &event) {
    isBeingDragged = true;

    int distanceMoved = event.getDistanceFromDragStartX();
    int deltaSamples = (distanceMoved * SAMPLE_RATE) / UI_ZOOM_MULTIPLIER;
    int rawSamplePos = startDragStartPositionSample + deltaSamples;

    // if ctrl held, trim
    if (event.mods.isCtrlDown()) {
        // trim left
        if (trimMode == -1) {
            int newTrimLeft =
                std::max(0, startTrimLeftPositionSample + deltaSamples);

            // snap
            if (!event.mods.isAltDown()) {
                // absolute snap position ON THE GRID
                int absoluteLeftBoundary =
                    startDragStartPositionSample + newTrimLeft;
                int snappedAbsolute =
                    utility::snapSample(absoluteLeftBoundary, SNAP_DIVISION);
                newTrimLeft = snappedAbsolute - startDragStartPositionSample;

                newTrimLeft = utility::snapSample(newTrimLeft, SNAP_DIVISION);
            }

            int trimDelta = newTrimLeft - startTrimLeftPositionSample;

            correspondingClip->trimLeft = newTrimLeft;
            correspondingClip->startPositionSample =
                startDragStartPositionSample + trimDelta;
        }

        // trim right
        else if (trimMode == 1) {
            int newTrimRight =
                std::max(0, startTrimRightPositionSample - deltaSamples);

            // snap
            if (!event.mods.isAltDown()) {
                // absolute snap position ON THE GRID
                int absoluteRightBoundary =
                    correspondingClip->startPositionSample +
                    correspondingClip->buffer.getNumSamples() -
                    correspondingClip->trimLeft - newTrimRight;

                int snappedAbsolute =
                    utility::snapSample(absoluteRightBoundary, SNAP_DIVISION);

                newTrimRight = correspondingClip->startPositionSample +
                               correspondingClip->buffer.getNumSamples() -
                               correspondingClip->trimLeft - snappedAbsolute;

                newTrimRight = std::max(0, newTrimRight);

                // newTrimRight = utility::snapSample(newTrimRight,
                // SNAP_DIVISION);
            }

            correspondingClip->trimRight = newTrimRight;
        }

        TimelineComponent *tc = findParentComponentOfClass<TimelineComponent>();
        tc->resizeClipComponent(this);
        return;
    }

    int newStartPos = event.mods.isAltDown()
                          ? rawSamplePos
                          : utility::snapSample(rawSamplePos, SNAP_DIVISION);

    correspondingClip->startPositionSample = newStartPos;

    TimelineComponent *tc = findParentComponentOfClass<TimelineComponent>();
    tc->resizeClipComponent(this);

    repaint();
}

void track::ClipComponent::mouseUp(const juce::MouseEvent &event) {
    if (isBeingDragged) {
        isBeingDragged = false;

        TimelineComponent *tc = (TimelineComponent *)getParentComponent();
        jassert(tc != nullptr);
        tc->resizeTimelineComponent();

        tc->grabKeyboardFocus();

        // dispatch to undomanager
        Tracklist *tracklist = tc->viewport->tracklist;
        audioNode *node = tracklist->trackComponents[(size_t)nodeDisplayIndex]
                              ->getCorrespondingTrack();
        int index = track::utility::getIndexOfClip(node, correspondingClip);

        bool noChange =
            this->correspondingClip->startPositionSample ==
                startDragStartPositionSample &&
            this->correspondingClip->trimLeft == startTrimLeftPositionSample &&
            this->correspondingClip->trimRight == startTrimRightPositionSample;

        if (!noChange) {
            ActionClipModified *action = new ActionClipModified(
                tc->processorRef,
                tracklist->trackComponents[(size_t)nodeDisplayIndex]->route,
                index, *this->correspondingClip);

            action->oldClip.startPositionSample = startDragStartPositionSample;
            action->oldClip.trimLeft = startTrimLeftPositionSample;
            action->oldClip.trimRight = startTrimRightPositionSample;

            tc->processorRef->undoManager.beginNewTransaction(
                "action clip modified");
            tc->processorRef->undoManager.perform(action);
        }
    }

    if (!event.mods.isLeftButtonDown()) {
        trimMode = 0;
    }
    reachedLeft = false;

    repaint();
}

void track::ClipComponent::mouseMove(const juce::MouseEvent &event) {
    if (event.x <= track::TRIM_REGION_WIDTH) {
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
    juce::KeyPress curCombo = juce::KeyPress::createFromDescription("a");
    if (curCombo.isCurrentlyDown()) {
        DBG("curCombo is down");
        DBG(curCombo.getKeyCode());
    }

    if (isKeyDown) {
        // r
        if (juce::KeyPress::isKeyCurrentlyDown(82)) {
            juce::Timer::callAfterDelay(10,
                                        [this] { clipNameLabel.showEditor(); });

            return true;
        }

        // 0
        else if (juce::KeyPress::isKeyCurrentlyDown(48)) {
            this->correspondingClip->active = !this->correspondingClip->active;
            repaint();

            TimelineComponent *tc = (TimelineComponent *)getParentComponent();
            tc->grabKeyboardFocus();

            return true;
        }

        // x, backspace, delete
        // don't allow deletion of clip while its name is being edited
        else if ((juce::KeyPress::isKeyCurrentlyDown(88) ||
                  juce::KeyPress::isKeyCurrentlyDown(8) ||
                  juce::KeyPress::isKeyCurrentlyDown(268435711)) &&
                 !clipNameLabel.isBeingEdited()) {
            TimelineComponent *tc = (TimelineComponent *)getParentComponent();

            if (!tc->renderingWaveforms())
                tc->deleteClip(correspondingClip, nodeDisplayIndex);

            return true;
        }

        // c
        else if (juce::KeyPress::isKeyCurrentlyDown(67)) {
            copyClip();
            return true;
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
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;

    clip *c = getClip();
    *c = newClip;

    markClipComponentStale();
    updateGUI();
    processor->requireSaving();

    return true;
}

bool track::ActionClipModified::undo() {
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;

    clip *c = getClip();
    *c = oldClip;

    markClipComponentStale();
    updateGUI();
    processor->requireSaving();

    return true;
}

void track::ActionClipModified::markClipComponentStale() {
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->dispatchGUIInstruction(UI_INSTRUCTION_MARK_CC_STALE, getClip());
}

void track::ActionClipModified::updateGUI() {
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;

    processor->dispatchGUIInstruction(UI_INSTRUCTION_UPDATE_STALE_TIMELINE);
    processor->dispatchGUIInstruction(UI_INSTRUCTION_INIT_CPWS);
}

track::ClipPropertiesWindow::ClipPropertiesWindow() : track::Subwindow() {
    addAndMakeVisible(nameLabel);
    addAndMakeVisible(gainSlider);

    gainSlider.setRange(0.f, 6.f);
    gainSlider.setNumDecimalPlacesToDisplay(2);
    nameLabel.setEditable(true, true, false);

    nameLabel.onEditorShow = [this] { this->oldName = nameLabel.getText(); };
    nameLabel.onEditorHide = [this] {
        getClip()->name = nameLabel.getText();

        AudioPluginAudioProcessorEditor *editor =
            findParentComponentOfClass<AudioPluginAudioProcessorEditor>();
        TimelineComponent *tc = editor->timelineComponent.get();

        ActionClipModified *action =
            new ActionClipModified(p, route, clipIndex, *getClip());

        action->oldClip.name = this->oldName;

        tc->processorRef->undoManager.beginNewTransaction(
            "action clip modified");
        tc->processorRef->undoManager.perform(action);
    };

    gainSlider.onDragStart = [this] {
        this->gainAtDragStart = gainSlider.getValue();
    };

    gainSlider.onValueChange = [this] {
        // change clips gain
        getClip()->gain = gainSlider.getValue();
    };

    gainSlider.onDragEnd = [this] {
        AudioPluginAudioProcessorEditor *editor =
            findParentComponentOfClass<AudioPluginAudioProcessorEditor>();
        TimelineComponent *tc = editor->timelineComponent.get();

        ActionClipModified *action =
            new ActionClipModified(p, route, clipIndex, *getClip());

        action->oldClip.gain = gainAtDragStart;

        tc->processorRef->undoManager.beginNewTransaction(
            "action clip modified");
        tc->processorRef->undoManager.perform(action);
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

    DBG("CLIP PROPERTIES:");
    DBG("start: " << getClip()->startPositionSample);
    DBG("trimLeft: " << getClip()->trimLeft);
    DBG("trimRight: " << getClip()->trimRight);
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
    jassert(processor != nullptr);
    jassert(siblingIndex != -1);
    jassert(route.size() != 0);

    AudioPluginAudioProcessor *p = (AudioPluginAudioProcessor *)processor;

    audioNode *head = &p->tracks[(size_t)route[0]];
    for (size_t i = 1; i < route.size(); ++i) {
        head = &head->childNodes[(size_t)route[i]];
    }

    return head;
}

void track::TrackComponent::initializSliders() {
    gainSlider.setValue(getCorrespondingTrack()->gain);
    panSlider.setValue(getCorrespondingTrack()->pan);
}

track::TrackComponent::TrackComponent(int trackIndex) : juce::Component() {
    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(true);

    // starting text for track name label is set when TrackComponent is
    // created in createTrackComponents()
    trackNameLabel.setFont(
        getAudioNodeLabelFont().withHeight(17.f).withExtraKerningFactor(
            -0.02f));
    trackNameLabel.setBounds(0, 0, 100, 20);
    trackNameLabel.setEditable(true);
    addAndMakeVisible(trackNameLabel);

    trackNameLabel.onEditorShow = [this] {
        nameValueAtStartChange = getCorrespondingTrack()->trackName;
    };

    trackNameLabel.onEditorHide = [this] {
        this->getCorrespondingTrack()->trackName = trackNameLabel.getText(true);

        TrivialNodeData oldState;
        TrivialNodeData newState;

        utility::getTrivialNodeData(&oldState, this->getCorrespondingTrack());
        utility::getTrivialNodeData(&newState, this->getCorrespondingTrack());
        oldState.trackName = this->nameValueAtStartChange;

        this->nameValueAtStartChange = "";

        AudioPluginAudioProcessor *p = (AudioPluginAudioProcessor *)processor;
        ActionModifyTrivialNodeData *action = new ActionModifyTrivialNodeData(
            route, oldState, newState, processor);
        p->undoManager.beginNewTransaction("action modify trivial node data");
        p->undoManager.perform(action);

        repaint();

        sendFocusToTimeline();
    };

    this->siblingIndex = trackIndex;

    addAndMakeVisible(muteBtn);
    muteBtn.setButtonText("M");

    addAndMakeVisible(soloBtn);
    soloBtn.setButtonText("S");

    gainSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox,
                               true, 0, 0);

    gainSlider.onDragStart = [this] {
        gainValueAtStartDrag = getCorrespondingTrack()->gain;
        repaint();
    };

    gainSlider.onDragEnd = [this] {
        getCorrespondingTrack()->gain = gainSlider.getValue();

        TrivialNodeData oldState;
        TrivialNodeData newState;

        utility::getTrivialNodeData(&oldState, this->getCorrespondingTrack());
        utility::getTrivialNodeData(&newState, this->getCorrespondingTrack());
        oldState.gain = this->gainValueAtStartDrag;

        this->gainValueAtStartDrag = -1.f;

        AudioPluginAudioProcessor *p = (AudioPluginAudioProcessor *)processor;
        ActionModifyTrivialNodeData *action = new ActionModifyTrivialNodeData(
            route, oldState, newState, processor);
        p->undoManager.beginNewTransaction("action modify trivial node data");
        p->undoManager.perform(action);

        sendFocusToTimeline();
    };

    gainSlider.onValueChange = [this] {
        getCorrespondingTrack()->gain = gainSlider.getValue();
    };

    addAndMakeVisible(gainSlider);
    gainSlider.setSkewFactorFromMidPoint(0.3f);
    gainSlider.setRange(0.0, 6.0);

    muteBtn.onClick = [this] {
        getCorrespondingTrack()->m = !(getCorrespondingTrack()->m);
        sendFocusToTimeline();
    };

    soloBtn.onClick = [this] {
        getCorrespondingTrack()->s = !(getCorrespondingTrack()->s);

        AudioPluginAudioProcessor *p = (AudioPluginAudioProcessor *)processor;

        if (getCorrespondingTrack()->s) {
            p->soloMode = true;
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
            }
        }

        sendFocusToTimeline();
    };

    panSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox,
                              true, 0, 0);
    panSlider.setSliderStyle(
        juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);

    addAndMakeVisible(panSlider);
    panSlider.setRange(-1.f, 1.f);

    panSlider.onValueChange = [this] {
        getCorrespondingTrack()->pan = panSlider.getValue();
    };
    panSlider.onDragStart = [this] {
        panValueAtStartDrag = panSlider.getValue();
        repaint();
    };
    panSlider.onDragEnd = [this] {
        TrivialNodeData oldState;
        TrivialNodeData newState;

        utility::getTrivialNodeData(&oldState, this->getCorrespondingTrack());
        utility::getTrivialNodeData(&newState, this->getCorrespondingTrack());
        oldState.pan = this->panValueAtStartDrag;

        this->panValueAtStartDrag = -1.f;

        AudioPluginAudioProcessor *p = (AudioPluginAudioProcessor *)processor;
        ActionModifyTrivialNodeData *action = new ActionModifyTrivialNodeData(
            route, oldState, newState, processor);
        p->undoManager.beginNewTransaction("action modify trivial node data");
        p->undoManager.perform(action);

        sendFocusToTimeline();
    };

    fxBtn.setButtonText("FX");
    addAndMakeVisible(fxBtn);

    fxBtn.onClick = [this] {
        AudioPluginAudioProcessorEditor *editor =
            this->findParentComponentOfClass<AudioPluginAudioProcessorEditor>();
        editor->openFxChain(route);
        sendFocusToTimeline();
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

        contextMenu.addItem("Copy", [this] { copyNodeToClipboard(); });

        contextMenu.addItem(
            "Paste node as child",
            !getCorrespondingTrack()->isTrack &&
                clipboard::typecode == TYPECODE_NODE,
            false, [this] {
                ActionPasteNode *action = new ActionPasteNode(
                    route, (audioNode *)clipboard::data, processor);

                AudioPluginAudioProcessor *p =
                    (AudioPluginAudioProcessor *)processor;
                p->undoManager.beginNewTransaction("action paste node");
                p->undoManager.perform(action);
                repaint();
            });

        contextMenu.addSeparator();

        contextMenu.addItem(
            "Ungroup",
            !getCorrespondingTrack()->isTrack || this->route.size() >= 2, false,
            [this] {
                AudioPluginAudioProcessor *p =
                    (AudioPluginAudioProcessor *)processor;

                ActionUngroup *action = new ActionUngroup(route, processor);
                p->undoManager.beginNewTransaction("action ungroup");
                p->undoManager.perform(action);
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

    repaint();
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

        // clang-format off
        if ((size_t)displayNodes >= tracklist->trackComponents.size() || displayNodes < 0)
            return;

        if (tracklist->trackComponents[(size_t)displayNodes]->getCorrespondingTrack()->isTrack) {
            AudioPluginAudioProcessor *p = (AudioPluginAudioProcessor *)processor;
      
            // check if both tracks belong to the same parent 
            std::vector<int> r1 = tracklist->trackComponents[(size_t)displayNodes]->route;
            r1.pop_back();

            std::vector<int> r2 = this->route;
            r2.pop_back();

            std::vector<int> sourceRoute = this->route;
            std::vector<int> movementRoute = tracklist->trackComponents[(size_t)displayNodes]->route;

            if (r1 == r2) {
                ActionReorderNode *action = new ActionReorderNode(sourceRoute, movementRoute, processor);
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
        }

        // clang-format on
    }
}

void track::TrackComponent::sendFocusToTimeline() {
    Tracklist *tracklist = (Tracklist *)getParentComponent();

    ((TimelineComponent *)tracklist->timelineComponent)
        ->grabKeyboardFocus(); // just rid any track components of focus

    repaint();
}

bool track::TrackComponent::keyStateChanged(bool isKeyDown) {
    if (isKeyDown) {
        // r
        if (juce::KeyPress::isKeyCurrentlyDown(82)) {
            juce::Timer::callAfterDelay(
                10, [this] { trackNameLabel.showEditor(); });

            return true;
        }

        // x, backspace, delete
        // don't allow deletion of node while its name is being edited
        else if ((juce::KeyPress::isKeyCurrentlyDown(88) ||
                  juce::KeyPress::isKeyCurrentlyDown(8) ||
                  juce::KeyPress::isKeyCurrentlyDown(268435711)) &&
                 !trackNameLabel.isBeingEdited()) {

            Tracklist *tracklist =
                (Tracklist *)findParentComponentOfClass<Tracklist>();
            tracklist->deleteTrack(this->route);

            return true;
        }

        // c
        else if (juce::KeyPress::isKeyCurrentlyDown(67)) {
            copyNodeToClipboard();
            return true;
        }

        // v
        else if (juce::KeyPress::isKeyCurrentlyDown(86)) {
            if (!getCorrespondingTrack()->isTrack) {
                ActionPasteNode *action = new ActionPasteNode(
                    route, (audioNode *)clipboard::data, processor);

                AudioPluginAudioProcessor *p =
                    (AudioPluginAudioProcessor *)processor;
                p->undoManager.beginNewTransaction("action paste node");
                p->undoManager.perform(action);
                repaint();
            }

            return true;
        }
    }

    return false;
}

void track::TrackComponent::copyNodeToClipboard() {
    // copy node
    audioNode *node = new audioNode;
    utility::copyNode(node, getCorrespondingTrack(), processor);
    clipboard::setData(node, TYPECODE_NODE);

    // visual feedback
    // if we're group, find all other nodes whose route starts with this
    // route and set coolColors = true
    Tracklist *tl = (Tracklist *)getParentComponent();
    if (!getCorrespondingTrack()->isTrack) {
        for (auto &tc : tl->trackComponents) {
            std::vector<int> tcRoute = tc->route;
            tcRoute.resize(this->route.size());

            if (this->route == tcRoute) {
                tc->coolColors = true;
                tc->repaint();
            }
        }
    } else {
        // we're not a group so this is simple
        coolColors = true;
        repaint();
    }

    juce::Timer::callAfterDelay(UI_VISUAL_FEEDBACK_FLASH_DURATION_MS,
                                [this, tl] {
                                    if (!getCorrespondingTrack()->isTrack) {
                                        for (auto &tc : tl->trackComponents) {
                                            tc->coolColors = false;
                                            tc->repaint();
                                        }
                                    } else {
                                        coolColors = false;
                                        repaint();
                                    }
                                });
}

void track::TrackComponent::paint(juce::Graphics &g) {
    juce::Colour bg = juce::Colour(0xFF'5F5F5F);

    if (hasKeyboardFocus(true))
        bg = bg.brighter(0.14f);

    if (coolColors)
        bg = juce::Colours::white;

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

    // this isn't scalable. if you move buttons in resized() they
    // should be able to automatically reflect over here, but that isn't the
    // case draw mute marker
    //
    // okayyy future john here. this is FINEEEE. (probably)
    // if i run into issues with this not being scalable then suck it future
    // john you big ol' sack of dirt
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
                                          bool isATrack, void *processor)
    : juce::UndoableAction() {
    this->parentRoute = pRoute;
    this->isTrack = isATrack;
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

    x->trackName = isTrack ? "Track" : "Group";
    x->trackName +=
        " " + juce::String(utility::getFlattenedNodes(p).size() + 1);

    updateGUI();
    processor->requireSaving();

    return true;
}

bool track::ActionCreateNode::undo() {
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;

    audioNode *parent = track::utility::getNodeFromRoute(parentRoute, p);

    if (parent == nullptr) {
        processor->tracks.pop_back();
    } else {
        parent->childNodes.pop_back();
    }

    updateGUI();
    processor->requireSaving();

    return true;
}

void track::ActionCreateNode::updateGUI() {
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->dispatchGUIInstruction(UI_INSTRUCTION_UPDATE_NODE_COMPONENTS);
    processor->dispatchGUIInstruction(UI_INSTRUCTION_UPDATE_CORE_SCROLLS);
}

track::ActionDeleteNode::ActionDeleteNode(std::vector<int> nodeRoute,
                                          void *processor)
    : juce::UndoableAction() {
    this->route = nodeRoute;
    this->p = processor;
}
track::ActionDeleteNode::~ActionDeleteNode() {}

bool track::ActionDeleteNode::perform() {
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;

    // close subwindows relevant to this node
    processor->dispatchGUIInstruction(
        UI_INSTRUCTION_CLOSE_ALL_FX_CHAINS_WITH_ROUTE, nullptr, route);

    audioNode *node = utility::getNodeFromRoute(route, p);
    for (size_t i = 0; i < node->plugins.size(); ++i) {
        processor->dispatchGUIInstruction(UI_INSTRUCTION_CLOSE_PEW,
                                          (void *)(uintptr_t)i, route);
        processor->dispatchGUIInstruction(
            UI_INSTRUCTION_CLOSE_ALL_RMC_WITH_ROUTE_AND_INDEX,
            (void *)(uintptr_t)i, route);
    }

    if (route.size() == 1) {
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
    processor->requireSaving();

    return true;
}

bool track::ActionDeleteNode::undo() {
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
    processor->requireSaving();

    return true;
}

void track::ActionDeleteNode::updateGUI() {
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->dispatchGUIInstruction(UI_INSTRUCTION_UPDATE_CORE);
}

track::ActionPasteNode::ActionPasteNode(std::vector<int> pRoute,
                                        track::audioNode *node, void *processor)
    : juce::UndoableAction() {
    this->parentRoute = pRoute;
    this->p = processor;

    this->nodeToPaste = new audioNode;
    utility::copyNode(this->nodeToPaste, node, p);
}
track::ActionPasteNode::~ActionPasteNode() { delete this->nodeToPaste; }
bool track::ActionPasteNode::perform() {
    audioNode *parentNode = utility::getNodeFromRoute(parentRoute, p);

    if (parentNode != nullptr) {
        pastedNodeRoute = parentRoute;
        pastedNodeRoute.push_back(parentNode->childNodes.size());
        audioNode &pastedNode = parentNode->childNodes.emplace_back();
        utility::copyNode(&pastedNode, nodeToPaste, p);
    } else {
        AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
        pastedNodeRoute.clear();
        pastedNodeRoute.push_back(processor->tracks.size());
        audioNode &pastedNode = processor->tracks.emplace_back();
        utility::copyNode(&pastedNode, nodeToPaste, p);
    }

    updateGUI();

    AudioPluginAudioProcessor *pr = (AudioPluginAudioProcessor *)p;
    pr->requireSaving();

    return true;
}
bool track::ActionPasteNode::undo() {
    utility::deleteNode(pastedNodeRoute, p);

    updateGUI();

    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->requireSaving();

    return true;
}
void track::ActionPasteNode::updateGUI() {
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->dispatchGUIInstruction(UI_INSTRUCTION_UPDATE_CORE);
}

track::ActionModifyTrivialNodeData::ActionModifyTrivialNodeData(
    std::vector<int> nodeRoute, TrivialNodeData oldData,
    TrivialNodeData newData, void *processor)
    : juce::UndoableAction() {

    this->route = nodeRoute;
    this->p = processor;
    this->oldState = oldData;
    this->newState = newData;
}
track::ActionModifyTrivialNodeData::~ActionModifyTrivialNodeData() {}

bool track::ActionModifyTrivialNodeData::perform() {
    audioNode *node = utility::getNodeFromRoute(route, p);

    utility::writeTrivialNodeDataToNode(node, newState);

    updateGUI();

    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->requireSaving();

    return true;
}

bool track::ActionModifyTrivialNodeData::undo() {
    audioNode *node = utility::getNodeFromRoute(route, p);

    utility::writeTrivialNodeDataToNode(node, oldState);

    updateGUI();

    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->requireSaving();

    return true;
}

void track::ActionModifyTrivialNodeData::updateGUI() {
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->dispatchGUIInstruction(
        UI_INSTRUCTION_UPDATE_EXISTING_NODE_COMPONENTS);
}

track::ActionMoveNodeToGroup::ActionMoveNodeToGroup(std::vector<int> toMove,
                                                    std::vector<int> group,
                                                    void *processor) {
    this->nodeToMoveRoute = toMove;
    this->groupRoute = group;
    this->p = processor;
}
track::ActionMoveNodeToGroup::~ActionMoveNodeToGroup() {}

// future john here; man i was LOCKED IN WHEN WRITING THIS SHIT
bool track::ActionMoveNodeToGroup::perform() {
    bool valid = true;

    if (nodeToMoveRoute == groupRoute) {
        valid = false;
    }
    if (utility::rWithPopBack(nodeToMoveRoute) == groupRoute) {
        valid = false;
    }
    if ((utility::rWithSize(groupRoute, nodeToMoveRoute.size()) ==
         nodeToMoveRoute) &&
        nodeToMoveRoute.size() < groupRoute.size()) {
        valid = false;
    }

    if (!valid)
        return false;

    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->dispatchGUIInstruction(UI_INSTRUCTION_CLEAR_SUBWINDOWS);

    // stain nodes
    audioNode *group = utility::getNodeFromRoute(groupRoute, p);
    group->stain = STAIN_MOVENODETOGROUP_GROUP;

    audioNode *nodeToMove = utility::getNodeFromRoute(nodeToMoveRoute, p);
    nodeToMove->stain = STAIN_MOVENODETOGROUP_NODE;

    // copy to temp node
    audioNode *temp = new audioNode;
    utility::copyNode(temp, nodeToMove, p);

    // now delete the "original" node that we will move
    utility::deleteNode(nodeToMoveRoute, p);

    // state mutated; re-get group by using its stain and add "temp" to that
    groupRouteAfterMoving = getStainedRoute(STAIN_MOVENODETOGROUP_GROUP);

    DBG("group route after moving is "
        << utility::prettyVector(groupRouteAfterMoving));

    group = utility::getNodeFromRoute(groupRouteAfterMoving, p);
    audioNode &newNode = group->childNodes.emplace_back();
    utility::copyNode(&newNode, temp, p);

    // now we can get route after moving by STAIN_MOVENODETOGROUP_NODE yay!
    routeAfterMoving = getStainedRoute(STAIN_MOVENODETOGROUP_NODE);

    delete temp;

    updateGUI();
    processor->requireSaving();

    return true;
}

bool track::ActionMoveNodeToGroup::undo() {
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->dispatchGUIInstruction(UI_INSTRUCTION_CLEAR_SUBWINDOWS);

    // get the node that we moved and copy it to temp
    audioNode *movedNode = utility::getNodeFromRoute(routeAfterMoving, p);

    audioNode *temp = new audioNode;
    utility::copyNode(temp, movedNode, p);

    // find the group node and stain it
    audioNode *group = utility::getNodeFromRoute(groupRouteAfterMoving, p);
    group->stain = STAIN_MOVENODETOGROUP_GROUP;

    // okay now remove the moved node
    utility::deleteNode(routeAfterMoving, p);

    group = utility::getNodeFromRoute(
        getStainedRoute(STAIN_MOVENODETOGROUP_GROUP), p);

    // add a node back to original position
    audioNode *newNode = nullptr;
    if (nodeToMoveRoute.size() == 1) {
        // no parent
        processor->tracks.emplace(processor->tracks.begin() +
                                  nodeToMoveRoute.back());
    } else {
        // yes parent
        audioNode *parent = utility::getParentFromRoute(nodeToMoveRoute, p);
        parent->childNodes.emplace(parent->childNodes.begin() +
                                   nodeToMoveRoute.back());
    }

    newNode = utility::getNodeFromRoute(nodeToMoveRoute, p);
    utility::copyNode(newNode, temp, p);

    delete temp;

    updateGUI();
    processor->requireSaving();

    return true;
}

void track::ActionMoveNodeToGroup::updateGUI() {
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->dispatchGUIInstruction(UI_INSTRUCTION_UPDATE_CORE);
}

void track::ActionMoveNodeToGroup::updateOnlyTracklist() {
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->dispatchGUIInstruction(UI_INSTRUCTION_RECREATE_RELAY_NODES);
}

std::vector<int> track::ActionMoveNodeToGroup::getStainedRoute(int staincode) {
    std::vector<int> retval;
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;

    for (size_t i = 0; i < processor->tracks.size(); ++i) {
        std::vector<int> tmp;
        tmp.push_back(i);

        audioNode *n = &processor->tracks[i];

        if (n->stain == staincode) {
            return tmp;
        }

        auto x = traverseStain(staincode, n, tmp, 0);

        if (x.size() > 1)
            return x;
    }

    return retval;
}

std::vector<int> track::ActionMoveNodeToGroup::traverseStain(
    int staincode, audioNode *parentNode, std::vector<int> r, int depth = 0) {
    std::vector<int> retval = r;

    for (size_t i = 0; i < parentNode->childNodes.size(); ++i) {
        r.push_back(i);

        audioNode *n = &parentNode->childNodes[i];
        if (n->stain == staincode) {
            return r;
        }

        if (!n->isTrack) {
            auto x = traverseStain(staincode, n, r, depth + 1);

            if (x.size() > r.size())
                return x;
        }

        r.pop_back();
    }

    return retval;
}

track::ActionReorderNode::ActionReorderNode(std::vector<int> route1,
                                            std::vector<int> route2,
                                            void *processor) {
    this->r1 = route1;
    this->r2 = route2;

    this->p = processor;
}
track::ActionReorderNode::~ActionReorderNode() {}

bool track::ActionReorderNode::perform() {
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->dispatchGUIInstruction(UI_INSTRUCTION_CLEAR_SUBWINDOWS);

    utility::reorderNodeAlt(r1, r2, p);
    updateGUI();
    processor->requireSaving();
    return true;
}

bool track::ActionReorderNode::undo() {

    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->dispatchGUIInstruction(UI_INSTRUCTION_CLEAR_SUBWINDOWS);

    utility::reorderNodeAlt(r1, r2, p);
    updateGUI();
    processor->requireSaving();
    return true;
}

void track::ActionReorderNode::updateGUI() {
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->dispatchGUIInstruction(UI_INSTRUCTION_UPDATE_CORE);
}

track::ActionUngroup::ActionUngroup(std::vector<int> nodeRoute,
                                    void *processor) {
    this->route = nodeRoute;
    this->p = processor;

    audioNode *node = utility::getNodeFromRoute(route, p);
    utility::copyNode(&this->nodeCopy, node, p);
}
track::ActionUngroup::~ActionUngroup(){};

bool track::ActionUngroup::perform() {
    audioNode *node = utility::getNodeFromRoute(route, p);
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->dispatchGUIInstruction(UI_INSTRUCTION_CLEAR_SUBWINDOWS);

    if (node->isTrack) {
        // move this node to grapdparent
        audioNode *grandparent = nullptr;

        // find grandparent
        if (route.size() >= 3) {
            std::vector<int> grandparentRoute = route;
            grandparentRoute.pop_back();
            grandparentRoute.pop_back();

            grandparent = utility::getNodeFromRoute(grandparentRoute, p);

            this->trackRouteAfterUngroup = grandparentRoute;
            this->trackRouteAfterUngroup.push_back(
                grandparent->childNodes.size());

        } else {
            grandparent = nullptr;
            trackRouteAfterUngroup.push_back(processor->tracks.size());
        }

        // add new node and copy contents over
        audioNode *newNode = nullptr;
        if (grandparent == nullptr)
            newNode = &processor->tracks.emplace_back();
        else
            newNode = &grandparent->childNodes.emplace_back();

        utility::copyNode(newNode, &this->nodeCopy, p);

        utility::deleteNode(route, p);
    } else {
        utility::deleteNode(route, p);

        // get parent and add all nodes; loop backward, emplace at route.back()
        audioNode *parent = utility::getParentFromRoute(route, p);
        for (int i = nodeCopy.childNodes.size() - 1; i >= 0; --i) {
            // add node and copy data
            if (parent == nullptr) {
                auto &newNode = *processor->tracks.emplace(
                    processor->tracks.begin() + route.back());
                utility::copyNode(&newNode,
                                  &this->nodeCopy.childNodes[(size_t)i], p);
            } else {
                auto &newNode = *parent->childNodes.emplace(
                    parent->childNodes.begin() + route.back());
                utility::copyNode(&newNode,
                                  &this->nodeCopy.childNodes[(size_t)i], p);
            }
        }
    }

    updateGUI();
    processor->requireSaving();

    return true;
}

bool track::ActionUngroup::undo() {
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->dispatchGUIInstruction(UI_INSTRUCTION_CLEAR_SUBWINDOWS);

    audioNode *originalParent = utility::getParentFromRoute(route, p);
    if (nodeCopy.isTrack) {
        audioNode &newNode = *originalParent->childNodes.emplace(
            originalParent->childNodes.begin() + route.back());

        utility::copyNode(&newNode, &this->nodeCopy, p);
        utility::deleteNode(trackRouteAfterUngroup, p);
    } else {
        for (size_t i = 0; i < nodeCopy.childNodes.size(); ++i) {
            if (originalParent == nullptr) {
                processor->tracks.erase(processor->tracks.begin() +
                                        route.back());
            } else {
                originalParent->childNodes.erase(
                    originalParent->childNodes.begin() + route.back());
            }
        }

        if (originalParent == nullptr) {
            auto &newNode = *processor->tracks.emplace(
                processor->tracks.begin() + route.back());
            utility::copyNode(&newNode, &this->nodeCopy, p);
        } else {
            auto &newNode = *originalParent->childNodes.emplace(
                originalParent->childNodes.begin() + route.back());
            utility::copyNode(&newNode, &this->nodeCopy, p);
        }
    }

    updateGUI();
    processor->requireSaving();

    return true;
}

void track::ActionUngroup::updateGUI() {
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->dispatchGUIInstruction(UI_INSTRUCTION_UPDATE_CORE);
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

    ActionCreateNode *action = new ActionCreateNode(parentRoute, isTrack, p);
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
}

// vokd track::Tracklist::deleteTrack(int trackIndex) {
void track::Tracklist::deleteTrack(std::vector<int> route) {
    AudioPluginAudioProcessor *p = (AudioPluginAudioProcessor *)processor;
    ActionDeleteNode *action = new ActionDeleteNode(route, processor);

    p->undoManager.beginNewTransaction("action delete node");
    p->undoManager.perform(action);
}

void track::Tracklist::deepCopyGroupInsideGroup(audioNode *childNode,
                                                audioNode *parentNode) {
    for (auto &child : childNode->childNodes) {
        audioNode *newNode = nullptr;

        if (parentNode == nullptr) {
            AudioPluginAudioProcessor *p =
                (AudioPluginAudioProcessor *)processor;
            newNode = &p->tracks.emplace_back();
        } else
            newNode = &parentNode->childNodes.emplace_back();

        copyNode(newNode, &child);
        continue;
    }
}

void track::Tracklist::moveNodeToGroup(track::TrackComponent *caller,
                                       int targetIndex) {

    if (targetIndex < 0 || (size_t)targetIndex > trackComponents.size() - 1 ||
        caller->displayIndex == targetIndex) {
        return;
    }

    std::vector<int> toMove = caller->route;
    std::vector<int> group = trackComponents[(size_t)targetIndex]->route;

    audioNode *nodeToMove = utility::getNodeFromRoute(toMove, processor);
    audioNode *groupToUse = utility::getNodeFromRoute(group, processor);
    if (utility::isDescendant(groupToUse, nodeToMove, true)) {
        return;
    }

    ActionMoveNodeToGroup *action =
        new ActionMoveNodeToGroup(toMove, group, processor);

    AudioPluginAudioProcessor *p = (AudioPluginAudioProcessor *)processor;
    p->undoManager.beginNewTransaction("action move node to group");
    p->undoManager.perform(action);
}

void track::Tracklist::mouseDown(const juce::MouseEvent &event) {
    if (event.mods.isRightButtonDown()) {
        juce::PopupMenu contextMenu;
        contextMenu.setLookAndFeel(&getLookAndFeel());

        contextMenu.addItem(
            "Paste", clipboard::typecode == TYPECODE_NODE, false, [this] {
                ActionPasteNode *action = new ActionPasteNode(
                    std::vector<int>(), (audioNode *)clipboard::data,
                    processor);

                AudioPluginAudioProcessor *p =
                    (AudioPluginAudioProcessor *)processor;
                p->undoManager.beginNewTransaction("action paste node");
                p->undoManager.perform(action);
                repaint();
            });
        contextMenu.addItem("Add new track", [this] { this->addNewNode(); });
        contextMenu.addItem("Add new group",
                            [this] { this->addNewNode(false); });

        contextMenu.showMenuAsync(juce::PopupMenu::Options());
    }

    ((TimelineComponent *)timelineComponent)
        ->grabKeyboardFocus(); // just rid any track components of focus
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

        foundItems = findChildren(&t, route, foundItems, 1);

        trackComponents.back().get()->displayIndex = foundItems;

        j++;
    }

    setDisplayIndexes();
    setTrackComponentBounds();
    repaint();

    // ee
    // future john here: ee indeed
}

void track::Tracklist::updateExistingTrackComponents() {
    for (auto &tc : trackComponents) {
        audioNode *node = tc->getCorrespondingTrack();

        tc->trackNameLabel.setText(node->trackName, juce::dontSendNotification);
        tc->panSlider.setValue(node->pan, juce::dontSendNotification);
        tc->gainSlider.setValue(node->gain, juce::dontSendNotification);
    }
}

void track::Tracklist::clearStains() {
    for (auto &x : trackComponents) {
        x->getCorrespondingTrack()->stain = -1;
    }
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

    if (this->plugin.get() != nullptr) {
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
}

bool track::subplugin::initializePlugin(juce::String path) {
    juce::OwnedArray<PluginDescription> pluginDescriptions;
    juce::KnownPluginList plist;
    juce::AudioPluginFormatManager apfm;
    juce::String errorMsg;

    if (track::SAMPLE_RATE < 0) {
        track::SAMPLE_RATE = 44100;
    }

    juce::addDefaultFormatsToManager(apfm);

    for (int i = 0; i < apfm.getNumFormats(); ++i)
        plist.scanAndAddFile(path, true, pluginDescriptions,
                             *apfm.getFormat(i));

    jassert(pluginDescriptions.size() > 0);

    plugin =
        apfm.createPluginInstance(*pluginDescriptions[0], track::SAMPLE_RATE,
                                  track::SAMPLES_PER_BLOCK, errorMsg);

    if (plugin.get() == nullptr)
        return false;

    plugin->setPlayConfigDetails(2, 2, track::SAMPLE_RATE,
                                 track::SAMPLES_PER_BLOCK);
    if (track::SAMPLES_PER_BLOCK <= 0) {
        track::SAMPLES_PER_BLOCK = 512;
    }

    plugin->prepareToPlay(track::SAMPLE_RATE, track::SAMPLES_PER_BLOCK);

    return true;
}
track::subplugin::subplugin() : plugin() {}
track::subplugin::~subplugin() {}

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

bool track::audioNode::addPlugin(juce::String path) {
    jassert(processor != nullptr);
    AudioPluginAudioProcessor *p = (AudioPluginAudioProcessor *)processor;

    plugins.push_back(std::make_unique<subplugin>());
    bool success = plugins.back()->initializePlugin(path);

    if (success) {
        plugins.back()->processor = processor;
        p->updateLatencyAfterDelay();
    } else {
        plugins.pop_back();
    }

    return success;
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
