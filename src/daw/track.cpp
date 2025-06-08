#include "track.h"
#include "../editor.h"
#include "../processor.h"
#include "clipboard.h"
#include "defs.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "timeline.h"
#include <cstddef>

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
    clipNameLabel.setBounds(getLocalBounds().getX(), getLocalBounds().getY(),
                            300, 20);
    addAndMakeVisible(clipNameLabel);

    clipNameLabel.onTextChange = [this] {
        correspondingClip->name = clipNameLabel.getText(true);
    };

    setBufferedToImage(true);
    setOpaque(false);
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
                g.setColour(juce::Colour(0xFF33587F));
                g.fillRoundedRectangle(getLocalBounds().toFloat(), cornerSize);
            }
        }

        if (this->correspondingClip->active)
            g.setColour(juce::Colour(0xFFAECBED));
        else
            g.setColour(juce::Colour(0xFF'696969));

        int thumbnailTopMargin = 14;
        juce::Rectangle<int> thumbnailBounds = getLocalBounds().reduced(2);
        thumbnailBounds.setHeight(thumbnailBounds.getHeight() -
                                  thumbnailTopMargin);
        thumbnailBounds.setY(thumbnailBounds.getY() + thumbnailTopMargin);

        thumbnail.drawChannels(g, thumbnailBounds, 0,
                               thumbnail.getTotalLength(), .7f);
    }

    if (isMouseOver(true) && !isBeingDragged) {
        g.setColour(juce::Colours::white.withAlpha(.07f));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), cornerSize);
    }

    g.setColour(juce::Colour(0xFF'000515));
    g.drawRoundedRectangle(getLocalBounds().toFloat(), cornerSize, 1.4f);

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

#define MENU_REVERSE_CLIP 1
#define MENU_CUT_CLIP 2
#define MENU_TOGGLE_CLIP_ACTIVATION 3
#define MENU_COPY_CLIP 4
#define MENU_SHOW_IN_EXPLORER 5

        contextMenu.addItem(MENU_COPY_CLIP, "Copy clip");
        contextMenu.addItem(MENU_CUT_CLIP, "Cut");
        contextMenu.addSeparator();
        contextMenu.addItem(MENU_REVERSE_CLIP, "Reverse");
        contextMenu.addItem(MENU_TOGGLE_CLIP_ACTIVATION,
                            "Toggle activate/deactive clip");
        contextMenu.addItem(MENU_SHOW_IN_EXPLORER, "Show in explorer");

        juce::PopupMenu::Options options;

        contextMenu.showMenuAsync(options, [this](int result) {
            if (result == MENU_REVERSE_CLIP) {
                this->correspondingClip->reverse();
                thumbnailCache.clear();
                thumbnail.setSource(&correspondingClip->buffer, SAMPLE_RATE, 2);
                repaint();
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
                newClip->buffer = correspondingClip->buffer;
                newClip->startPositionSample =
                    correspondingClip->startPositionSample;
                newClip->name = correspondingClip->name;
                newClip->path = correspondingClip->path;
                newClip->active = correspondingClip->active;

                clipboard::setData(newClip, TYPECODE_CLIP);
            }

            else if (result == MENU_SHOW_IN_EXPLORER) {
                juce::File f(correspondingClip->path);
                f.revealToUser();
            }
        });
    }

    else {
        startDragX = getLocalBounds().getX();
        startDragStartPositionSample = correspondingClip->startPositionSample;
    }
}

void track::ClipComponent::mouseDrag(const juce::MouseEvent &event) {
    // DBG("dragging is happening");
    isBeingDragged = true;

    int distanceMoved = startDragX + event.getDistanceFromDragStartX();
    int rawSamplePos = startDragStartPositionSample +
                       ((distanceMoved * SAMPLE_RATE) / UI_ZOOM_MULTIPLIER);

    // if alt held, don't snap
    if (event.mods.isAltDown()) {
        correspondingClip->startPositionSample = rawSamplePos;
    } else {
        double secondsPerBeat = 60.f / BPM;
        int samplesPerBar = (secondsPerBeat * SAMPLE_RATE) * 4;
        int samplesPerSnap = samplesPerBar / SNAP_DIVISION;

        int snappedSamplePos =
            ((rawSamplePos + samplesPerSnap / 2) / samplesPerSnap) *
            samplesPerSnap;

        correspondingClip->startPositionSample = snappedSamplePos;
    }

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
    }

    DBG(event.getDistanceFromDragStartX());
    repaint();
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
    // starting text for track name label is set when TrackComponent is created
    // in createTrackComponents()
    // DBG("TrackComponent constructor called");

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

    // gainSlider.hideTextBox(true);
    // gainSlider.setValue(getCorrespondingTrack()->gain);
    gainSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox,
                               true, 0, 0);

    gainSlider.onDragEnd = [this] {
        DBG("setting new gain for track; " << gainSlider.getValue());
        getCorrespondingTrack()->gain = gainSlider.getValue();
    };

    addAndMakeVisible(gainSlider);
    gainSlider.setRange(0.0, 1.0);

    muteBtn.onClick = [this] {
        getCorrespondingTrack()->m = !(getCorrespondingTrack()->m);
        repaint();
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
                tracklist->addNewNode(true, getCorrespondingTrack());
            });

            contextMenu.addItem("Add child group", [this] {
                Tracklist *tracklist = findParentComponentOfClass<Tracklist>();
                tracklist->addNewNode(false, getCorrespondingTrack());
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

            if (r1 == r2) {
                DBG("BOTH ARE SIBLINGS");
                if (this->route.size() == 1) {
                    // trivially move node
                    // TODO: copy plugins 
                    // TODO: handle moving groups

                    //DBG("displaynodes is " << displayNodes);
                    //DBG("route[0] is " << route[0]);

                    int insertionNode = displayNodes;
                    int sourceNode = route[0];

                    bool movingUp = insertionNode < sourceNode;
                    if (movingUp) {
                        sourceNode++;
                    }

                    insertionNode = juce::jmin((size_t)insertionNode, p->tracks.size() - 1);

                    audioNode& newNode = *p->tracks.emplace(p->tracks.begin() + insertionNode);
                    audioNode& originalNode = p->tracks[(size_t)sourceNode]; // don't use getCorrespondingTrack() because route is invalid due to emplacing a node
                    
                    tracklist->copyNode(&newNode, &originalNode);

                    /*
                    if (!originalNode.isTrack) {
                        for (auto& child : originalNode.childNodes) {

                            auto& newChild = newNode.childNodes.emplace_back();
                            tracklist->copyNode(&newChild, &child);

                            if (!child.isTrack) {
                                // copy its child nodes
                                tracklist->deepCopyGroupInsideGroup(&child, &newChild); 
                            }
                        }
                    }
                    */

                    p->tracks.erase(p->tracks.begin() + sourceNode);
                }
                else {
                    audioNode* head = &p->tracks[(size_t)route[0]];
                    for (size_t i = 1; i < route.size()-1; ++i) {
                        head = &head->childNodes[(size_t)route[i]];
                    }

                    int sourceNode = route.back();
                    int insertionNode = r1End;

                    bool movingUp = insertionNode < sourceNode;
                    if (movingUp) {
                        sourceNode++;
                    }

                    insertionNode = juce::jmin((size_t)insertionNode, head->childNodes.size()-1);
                    
                    audioNode& newNode = *head->childNodes.emplace(head->childNodes.begin() + insertionNode);
                    audioNode& originalNode = head->childNodes[(size_t)sourceNode];

                    tracklist->copyNode(&newNode, &originalNode);

                    head->childNodes.erase(head->childNodes.begin() + sourceNode);
                }

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
        track::ui::CustomLookAndFeel::getInterSemiBold().withHeight(17.f));
    g.drawText(juce::String(displayIndex + 1), 0, 0, UI_TRACK_INDEX_WIDTH,
               UI_TRACK_HEIGHT, juce::Justification::centred);

    // draw mute marker
    if (getCorrespondingTrack()->m) {
        int btnSize = 24;
        juce::Rectangle<int> btnBounds = juce::Rectangle<int>(
            UI_TRACK_WIDTH - 100, (UI_TRACK_HEIGHT / 2) - (btnSize / 2),
            btnSize, btnSize);

        btnBounds.expand(1, 1);
        g.setColour(juce::Colour(0xFF'FACD51));
        g.drawRect(btnBounds);
    }
}

void track::TrackComponent::resized() {

    // trackNameLabel.setBounds((route.size() - 1) * 10, 0, getWidth(), 20);
    // return;

    int xOffset = (route.size() - 1) * UI_TRACK_DEPTH_INCREMENTS;
    trackNameLabel.setBounds(UI_TRACK_INDEX_WIDTH + getLocalBounds().getX() +
                                 5 + xOffset,
                             (UI_TRACK_HEIGHT / 4) - 5, 100, 20);
    int btnSize = 24;
    int btnHeight = btnSize;
    int btnWidth = btnSize;

    juce::Rectangle<int> btnBounds = juce::Rectangle<int>(
        UI_TRACK_WIDTH - 100, (UI_TRACK_HEIGHT / 2) - (btnHeight / 2), btnWidth,
        btnHeight);

    muteBtn.setBounds(btnBounds);

    // set pan slider bounds
    float panSliderSizeMultiplier = 1.8f;
    juce::Rectangle<int> panSliderBounds = juce::Rectangle<int>(
        btnBounds.getX() + 20, btnBounds.getY() - 8,
        btnSize * panSliderSizeMultiplier, btnSize * panSliderSizeMultiplier);

    panSlider.setBounds(panSliderBounds);

    juce::Rectangle<int> fxBounds = juce::Rectangle<int>(
        panSliderBounds.getX() + 38, btnBounds.getY(), btnSize * 1.2f, btnSize);
    fxBtn.setBounds(fxBounds);

    int sliderHeight = 20;
    int sliderWidth = 120;
    gainSlider.setBounds(xOffset + UI_TRACK_INDEX_WIDTH + 4,
                         (UI_TRACK_HEIGHT / 2) - (sliderHeight / 2) +
                             (int)(UI_TRACK_HEIGHT * .2f),
                         sliderWidth, sliderHeight);
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
    };

    addAndMakeVisible(unsoloAllBtn);
    unsoloAllBtn.setButtonText("UNSOLO ALL");
    unmuteAllBtn.onClick = [this] {
        for (auto &tc : trackComponents) {
            tc->getCorrespondingTrack()->s = false;
        }
    };

    newTrackBtn.onClick = [this] { addNewNode(); };
    newGroupBtn.onClick = [this] { addNewNode(false); };

    addAndMakeVisible(insertIndicator);
}
track::Tracklist::~Tracklist() {}

void track::Tracklist::copyNode(audioNode *dest, audioNode *src) {
    dest->isTrack = src->isTrack;
    dest->trackName = src->trackName;
    dest->bypassedPlugins = src->bypassedPlugins;
    dest->gain = src->gain;
    dest->processor = processor;
    dest->s = src->s;
    dest->m = src->m;
    dest->pan = src->pan;
    dest->maxSamplesPerBlock = src->maxSamplesPerBlock;
    dest->sampleRate = src->sampleRate;

    if (src->isTrack) {
        dest->clips = src->clips;
    } else {
        for (auto &child : src->childNodes) {
            auto &newChild = dest->childNodes.emplace_back();
            copyNode(&newChild, &child);
        }
    }

    for (auto &pluginInstance : src->plugins) {
        juce::MemoryBlock pluginData;
        pluginInstance->getStateInformation(pluginData);
        pluginInstance->releaseResources();

        juce::String identifier =
            pluginInstance->getPluginDescription().fileOrIdentifier;

        identifier = identifier.upToLastOccurrenceOf(".vst3", true, true);

        // add plugin to new node and copy data
        DBG("adding plugin to new node, using identifier " << identifier);
        dest->addPlugin(identifier);
        dest->plugins[dest->plugins.size() - 1]->setStateInformation(
            pluginData.getData(), pluginData.getSize());
    }
}

void track::Tracklist::addNewNode(bool isTrack, audioNode *parent) {
    AudioPluginAudioProcessor *p = (AudioPluginAudioProcessor *)this->processor;
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

    trackComponents.clear();
    createTrackComponents();
    setTrackComponentBounds();
    repaint();
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

        DBG("recursivelyDeleteNodePlugins(): deleting a plugin: "
            << plugin->getPluginDescription().name);
        plugin->releaseResources();
    }
}

// vokd track::Tracklist::deleteTrack(int trackIndex) {
void track::Tracklist::deleteTrack(std::vector<int> route) {
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
    tc->updateClipComponents();
}

void track::Tracklist::deepCopyGroupInsideGroup(audioNode *childNode,
                                                audioNode *parentNode) {
    for (auto &child : childNode->childNodes) {
        audioNode *newNode = &parentNode->childNodes.emplace_back();
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
        for (auto &pluginInstance : child.plugins) {
            /*
            if (pluginInstance->getActiveEditor() != nullptr) {
                DBG("pluginInstance's active editor still exists");
                delete pluginInstance->getActiveEditor();
            }
            */

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
            newNode->plugins[newNode->plugins.size() - 1]->setStateInformation(
                pluginData.getData(), pluginData.getSize());
        }
    }
}

void track::Tracklist::moveNodeToGroup(track::TrackComponent *caller,
                                       int targetIndex) {
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
    */

    // node is copied. now delete orginal node
    AudioPluginAudioProcessor *p = (AudioPluginAudioProcessor *)processor;
    jassert(caller->route.size() > 0);

    if (routeCopy.size() == 1) {
        DBG("deleting orphan node with index " << routeCopy[0]);
        for (auto &pluginInstance : p->tracks[(size_t)routeCopy[0]].plugins) {
            AudioProcessorEditor *subpluginEditor =
                pluginInstance->getActiveEditor();
            if (subpluginEditor != nullptr) {
                pluginInstance->editorBeingDeleted(subpluginEditor);
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

        DBG(tabs << "found " << (childNode->isTrack ? "track" : "group") << " "
                 << childNode->trackName << "(" << i << foundItems << depth
                 << ") " << getPrettyVector(route));

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
    DBG("");
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

        DBG("found " << t.trackName << " (root node)");
        foundItems = findChildren(&t, route, foundItems, 1);

        trackComponents.back().get()->displayIndex = foundItems;

        DBG("");

        j++;
    }

    setDisplayIndexes();
    setTrackComponentBounds();
    repaint();
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

    int btnWidth = 71;
    int btnMargin = 3;
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
            0, (UI_TRACK_HEIGHT * index) + (UI_TRACK_HEIGHT / 2) - 4,
            UI_TRACK_WIDTH, 1);
        repaint();
        return;
    }

    this->insertIndicator.setVisible(false);
}

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
    this->bypassedPlugins.push_back(false);

    DBG("track::track addPlugin() called with path " << path);
    juce::OwnedArray<PluginDescription> pluginDescriptions;
    juce::KnownPluginList plist;
    juce::AudioPluginFormatManager apfm;
    juce::String errorMsg;

    jassert(processor != nullptr);

    AudioPluginAudioProcessor *p = (AudioPluginAudioProcessor *)processor;
    this->sampleRate = p->getSampleRate();
    this->maxSamplesPerBlock = p->maxSamplesPerBlock;

    jassert(sampleRate > 0);
    // jassert(maxSamplesPerBlock > 0);

    apfm.addDefaultFormats();

    DBG("scanning plugin");
    for (int i = 0; i < apfm.getNumFormats(); ++i)
        plist.scanAndAddFile(path, true, pluginDescriptions,
                             *apfm.getFormat(i));

    plugins.emplace_back();
    std::unique_ptr<juce::AudioPluginInstance> &plugin = this->plugins.back();

    DBG("creating plugin instance");
    jassert(pluginDescriptions.size() > 0);

    plugin = apfm.createPluginInstance(*pluginDescriptions[0], sampleRate,
                                       maxSamplesPerBlock, errorMsg);

    DBG("LOGGING OUTPUTS:");
    DBG("   " << plugin->getMainBusNumInputChannels() << " inputs");
    DBG("   " << plugin->getMainBusNumOutputChannels() << " outputs");

    // DBG("setting plugin play config details");
    plugin->setPlayConfigDetails(2, 2, sampleRate, maxSamplesPerBlock);

    /*
    DBG("setting buses");
    AudioPluginAudioProcessor *p = (AudioPluginAudioProcessor *)processor;
    AudioPluginAudioProcessor::BusesLayout busesLayout =
    p->getBusesLayout(); plugin->setBusesLayout(busesLayout);

    DBG("setting rate and buffer size details");
    plugin->setRateAndBufferSizeDetails(sampleRate, maxSamplesPerBlock);
    */

    DBG("preparing plugin to play with sample rate,maxSamplesPerBlock"
        << sampleRate << " " << maxSamplesPerBlock);

    // TODO: is this really a good idea?
    if (maxSamplesPerBlock <= 0) {
        DBG("setting maxSamplesPerBlock to 512");
        maxSamplesPerBlock = 512;
    }
    plugin->prepareToPlay(sampleRate, maxSamplesPerBlock);

    DBG("LOGGING OUTPUTS:");
    DBG("   " << plugin->getMainBusNumInputChannels() << " inputs");
    DBG("   " << plugin->getMainBusNumOutputChannels() << " outputs");

    DBG("plugin added");
}

void track::audioNode::removePlugin(int index) {
    this->plugins.erase(this->plugins.begin() + index);
    this->bypassedPlugins.erase(this->bypassedPlugins.begin() + index);
}

void track::audioNode::preparePlugins(int newMaxSamplesPerBlock,
                                      int newSampleRate) {
    for (std::unique_ptr<juce::AudioPluginInstance> &plugin : plugins) {
        plugin->prepareToPlay(sampleRate, newMaxSamplesPerBlock);
    }
}

void track::audioNode::process(int numSamples, int currentSample) {
    if (m) {
        buffer.clear();
        return;
    }

    if (buffer.getNumSamples() != numSamples)
        buffer.setSize(2, numSamples, false, false, true);

    buffer.clear();

    int outputBufferLength = numSamples;
    int totalNumInputChannels = 2;

    if (isTrack) {
        // add sample data to buffer
        for (clip &c : clips) {
            if (!c.active)
                continue;

            int clipStart = c.startPositionSample;
            int clipEnd = c.startPositionSample + c.buffer.getNumSamples();

            // bounds check
            if (clipEnd > currentSample &&
                clipStart < currentSample + outputBufferLength) {
                // where in buffer should clip start?
                int outputOffset =
                    (clipStart < currentSample) ? 0 : clipStart - currentSample;
                // starting point in clip's buffer?
                int clipBufferStart =
                    (clipStart < currentSample) ? currentSample - clipStart : 0;
                // how many samples can we safely copy?
                int samplesToCopy =
                    juce::jmin(outputBufferLength - outputOffset,
                               c.buffer.getNumSamples() - clipBufferStart);

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

        if (this->bypassedPlugins[i] == true)
            continue;

        juce::MidiBuffer mb;
        this->plugins[i]->processBlock(this->buffer, mb);
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
