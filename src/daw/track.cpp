#include "track.h"
#include "../editor.h"
#include "../processor.h"
#include "clipboard.h"
#include "defs.h"
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_events/juce_events.h"
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
    thumbnail.setSource(&correspondingClip->buffer, 44100.0, 2);

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

        contextMenu.addItem(MENU_COPY_CLIP, "Copy clip");
        contextMenu.addItem(MENU_CUT_CLIP, "Cut");
        contextMenu.addSeparator();
        contextMenu.addItem(MENU_REVERSE_CLIP, "Reverse");
        contextMenu.addItem(MENU_TOGGLE_CLIP_ACTIVATION,
                            "Toggle activate/deactive clip");

        juce::PopupMenu::Options options;

        contextMenu.showMenuAsync(options, [this](int result) {
            if (result == MENU_REVERSE_CLIP) {
                this->correspondingClip->reverse();
                thumbnailCache.clear();
                thumbnail.setSource(&correspondingClip->buffer, 44100.0, 2);
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
                       ((distanceMoved * 44100) / UI_ZOOM_MULTIPLIER);

    // if alt held, don't snap
    if (event.mods.isAltDown()) {
        correspondingClip->startPositionSample = rawSamplePos;
    } else {
        double secondsPerBeat = 60.f / BPM;
        int samplesPerBar = (secondsPerBeat * 44100) * 4;
        int samplesPerSnap = samplesPerBar / SNAP_DIVISION;

        int snappedSamplePos =
            ((rawSamplePos + samplesPerSnap / 2) / samplesPerSnap) *
            samplesPerSnap;

        correspondingClip->startPositionSample = snappedSamplePos;
    }

    repaint();
}

void track::ClipComponent::mouseUp(const juce::MouseEvent &event) {
    if (isBeingDragged) {
        DBG("DRAGGING STOPPED");
        isBeingDragged = false;

        int distanceMovedHorizontally = event.getDistanceFromDragStartX();

        /*
        this->correspondingClip->startPositionSample +=
            (distanceMovedHorizontally * 44100) / UI_ZOOM_MULTIPLIER;
            */

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

void track::TrackComponent::initializeGainSlider() {
    // DBG("intiailizGainSlider() called");
    // DBG("track's gain is " << getCorrespondingTrack()->gain);
    gainSlider.setValue(getCorrespondingTrack()->gain);
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
            "delete " + getCorrespondingTrack()->trackName, [this] {
                Tracklist *tracklist =
                    (Tracklist *)findParentComponentOfClass<Tracklist>();

                // tracklist->deleteTrack(this->siblingIndex);
                tracklist->deleteTrack(this->route);
            });

        contextMenu.showMenuAsync(juce::PopupMenu::Options());
    }
}

void track::TrackComponent::mouseUp(const juce::MouseEvent &event) {
    if (event.mouseWasDraggedSinceMouseDown()) {
        float rawDisplayNodes =
            event.getDistanceFromDragStartY() / (float)UI_TRACK_HEIGHT;
        int displayNodes = (int)rawDisplayNodes;
        DBG("raw display nodes is " << rawDisplayNodes);
        DBG("display nodes is " << displayNodes);

        Tracklist *tracklist = findParentComponentOfClass<Tracklist>();
        tracklist->moveNodeToGroup(this, displayNodes);
        DBG("done");
    }
}

void track::TrackComponent::paint(juce::Graphics &g) {
    g.fillAll(juce::Colour(0xFF5F5F5F));   // bg
    g.setColour(juce::Colour(0xFF535353)); // outline
    g.drawRect(getLocalBounds(), 2);

    g.setColour(juce::Colours::lightgrey);
    g.fillRect(0, 0, 5 * (route.size() - 1), getHeight());

    juce::String trackName = getCorrespondingTrack() == nullptr
                                 ? "null trackk"
                                 : getCorrespondingTrack()->trackName;

    juce::Rectangle<int> textBounds = getLocalBounds();
    textBounds.setX(getLocalBounds().getX() + 14);
    textBounds.setY(getLocalBounds().getY() - 8);

    /*
    // gray out muted track names, and draw yellow square around mute button if
    // needed
    juce::Colour trackNameColour = juce::Colour(0xFFDFDFDF);
    if (getCorrespondingTrack()->m) {
        // draw yellow square
        g.setColour(juce::Colours::yellow);
        g.setOpacity(0.5f);
        auto btnBounds = muteBtn.getBounds();
        btnBounds.expand(1, 1);
        g.drawRect(btnBounds);

        g.setColour(trackNameColour.withAlpha(.5f));
    }

    else {
        g.setColour(trackNameColour);
    }

    g.drawText(trackName, textBounds, juce::Justification::left, true);
    */
}

void track::TrackComponent::resized() {

    // trackNameLabel.setBounds((route.size() - 1) * 10, 0, getWidth(), 20);
    // return;

    int xOffset = (route.size() - 1) * 10;
    trackNameLabel.setBounds(getLocalBounds().getX() + 10 + xOffset,
                             (UI_TRACK_HEIGHT / 4) - 5, 100, 20);
    int btnSize = 24;
    int btnHeight = btnSize;
    int btnWidth = btnSize;

    juce::Rectangle<int> btnBounds = juce::Rectangle<int>(
        xOffset + UI_TRACK_WIDTH - 115, (UI_TRACK_HEIGHT / 2) - (btnHeight / 2),
        btnWidth, btnHeight);

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
    int sliderWidth = 130;
    gainSlider.setBounds(8,
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

    newTrackBtn.onClick = [this] { addNewNode(); };
    newGroupBtn.onClick = [this] { addNewNode(false); };
}
track::Tracklist::~Tracklist() {}

void track::Tracklist::addNewNode(bool isTrack) {
    auto p = (AudioPluginAudioProcessor *)this->processor;
    p->tracks.emplace_back();
    audioNode &t = p->tracks.back();
    t.processor = this->processor;
    t.isTrack = isTrack;
    t.trackName = isTrack ? "Untitled Track" : "Untitled Group";

    jassert(t.processor != nullptr);

    int i = p->tracks.size() - 1;
    std::vector<int> route;
    route.push_back(i);
    this->trackComponents.push_back(std::make_unique<TrackComponent>(i));
    trackComponents.back().get()->processor = processor;
    trackComponents.back().get()->route = route;
    trackComponents.back().get()->initializeGainSlider();

    // set track label text
    trackComponents.back().get()->trackNameLabel.setText(
        trackComponents.back().get()->getCorrespondingTrack()->trackName,
        juce::NotificationType::dontSendNotification);

    addAndMakeVisible(*trackComponents.back());

    DBG("New node component added");
    setTrackComponentBounds();

    repaint();
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
            DBG("deleting a node which has a plugin with an EDITOR WHCIH IS "
                "STILL ACTIVE");
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
    /*
    DBG("deleting track " << trackIndex);

    AudioPluginAudioProcessor *p = (AudioPluginAudioProcessor *)processor;
    p->tracks.erase(p->tracks.begin() + trackIndex);

    TimelineComponent *tc = (TimelineComponent *)timelineComponent;
    tc->updateClipComponents();

    // update trackComponent indices
    trackComponents.erase(trackComponents.begin() + trackIndex);
    for (int i = trackIndex; i < trackComponents.size(); ++i) {
        trackComponents[i].get()->siblingIndex--;
    }

    this->setTrackComponentBounds();
    tc->repaint();
    repaint();
    */

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
}

void track::Tracklist::deepCopyGroupInsideGroup(audioNode *childNode,
                                                audioNode *parentNode) {
    for (auto &child : childNode->childNodes) {
        audioNode *newNode = &parentNode->childNodes.emplace_back();
        newNode->trackName = child.trackName;
        newNode->isTrack = child.isTrack;
        newNode->gain = child.gain;
        newNode->processor = processor;

        if (newNode->isTrack)
            newNode->clips = childNode->clips;
        else {
            deepCopyGroupInsideGroup(&child, newNode);
        }

        // now the annoying thing, copying plugins. (which is why you can't just
        // push_back() trackNode into groupNode->childNodes)
        for (auto &pluginInstance : child.plugins) {
            /*
            if (pluginInstance->getActiveEditor() != nullptr) {
                DBG("pluginInstance's active editor still exists");
                delete pluginInstance->getActiveEditor();
            }
            */

            pluginInstance->releaseResources();

            juce::String identifier =
                pluginInstance->getPluginDescription().fileOrIdentifier;

            identifier = identifier.upToLastOccurrenceOf(".vst3", true, true);

            DBG("adding plugin to new node, using identifier " << identifier);
            newNode->addPlugin(identifier);
            // TODO: handle making sure subplugin data copies over properly
        }
    }
}

// FIXME: moving a group into its child (which is also a group) causes segfault
void track::Tracklist::moveNodeToGroup(track::TrackComponent *caller,
                                       int relativeDisplayNodesToMove) {

    audioNode *trackNode = caller->getCorrespondingTrack();

    int callerIndex = -1;
    // TODO: this is horrendous and you shouldn't need to find the index
    // like this. maybe store the display node index in each track component
    // or something. idk.
    for (size_t i = 0; i < this->trackComponents.size(); ++i) {
        if (this->trackComponents[i].get() == caller) {
            callerIndex = i;
            break;
        }
    }

    int targetIndex = callerIndex + relativeDisplayNodesToMove;
    if (relativeDisplayNodesToMove == 0 || callerIndex < 0 || targetIndex < 0 ||
        targetIndex >= (int)trackComponents.size())
        return;

    auto &nodeComponentToMoveTo = this->trackComponents[(size_t)targetIndex];

    audioNode *parentNode = nodeComponentToMoveTo->getCorrespondingTrack();
    if (parentNode->isTrack) {
        DBG("rejecting moving track inside a track");
        return;
    }

    audioNode *newNode = &parentNode->childNodes.emplace_back();
    newNode->trackName = trackNode->trackName;
    newNode->gain = trackNode->gain;
    newNode->isTrack = trackNode->isTrack;
    newNode->processor = processor;

    std::vector<int> routeCopy = caller->route;
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

        pluginInstance->releaseResources();

        juce::String identifier =
            pluginInstance->getPluginDescription().fileOrIdentifier;

        identifier = identifier.upToLastOccurrenceOf(".vst3", true, true);

        DBG("adding plugin to new node, using identifier " << identifier);
        newNode->addPlugin(identifier);
        // TODO: handle making sure subplugin data copies over properly
    }

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

        contextMenu.addItem("add new track", [this] { this->addNewNode(); });

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
        trackComponents.back().get()->initializeGainSlider();
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

void track::Tracklist::createTrackComponents() {
    AudioPluginAudioProcessor *p =
        (AudioPluginAudioProcessor *)(this->processor);

    int i = 0;
    int foundItems = 0;
    DBG("");
    for (audioNode &t : p->tracks) {
        std::vector<int> route;
        route.push_back(i);

        this->trackComponents.push_back(std::make_unique<TrackComponent>(i));
        trackComponents.back().get()->processor = processor;
        trackComponents.back().get()->route = route;
        trackComponents.back().get()->initializeGainSlider();
        trackComponents.back().get()->trackNameLabel.setText(
            t.trackName, juce::NotificationType::dontSendNotification);
        addAndMakeVisible(*trackComponents.back());

        DBG("found " << t.trackName << " (root node)");
        foundItems = findChildren(&t, route, foundItems, 1);
        DBG("");

        i++;
    }

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

    newTrackBtn.setBounds(0, 0, 80, UI_TRACK_VERTICAL_OFFSET);
    newGroupBtn.setBounds(80 + 20, 0, 80, UI_TRACK_VERTICAL_OFFSET);
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
    DBG("track::track addPlugin() called");
    juce::OwnedArray<PluginDescription> pluginDescriptions;
    juce::KnownPluginList plist;
    juce::AudioPluginFormatManager apfm;
    juce::String errorMsg;

    jassert(processor != nullptr);

    AudioPluginAudioProcessor *p = (AudioPluginAudioProcessor *)processor;
    this->sampleRate = p->getSampleRate();
    this->maxSamplesPerBlock = p->maxSamplesPerBlock;

    jassert(sampleRate > 0);
    jassert(maxSamplesPerBlock > 0);

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
    plugin->prepareToPlay(sampleRate, maxSamplesPerBlock);

    DBG("LOGGING OUTPUTS:");
    DBG("   " << plugin->getMainBusNumInputChannels() << " inputs");
    DBG("   " << plugin->getMainBusNumOutputChannels() << " outputs");

    DBG("plugin added");
}

void track::audioNode::preparePlugins(int newMaxSamplesPerBlock,
                                      int newSampleRate) {
    for (std::unique_ptr<juce::AudioPluginInstance> &plugin : plugins) {
        plugin->prepareToPlay(newMaxSamplesPerBlock, newSampleRate);
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
    for (std::unique_ptr<juce::AudioPluginInstance> &plugin : plugins) {
        if (plugin == nullptr)
            return;

        juce::MidiBuffer mb;
        plugin->processBlock(buffer, mb);
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
