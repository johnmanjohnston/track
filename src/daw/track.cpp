#include "track.h"
#include "defs.h"
#include "timeline.h"

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
}
track::ClipComponent::~ClipComponent() { thumbnail.removeAllChangeListeners(); }

void track::ClipComponent::changeListenerCallback(ChangeBroadcaster *source) {
    repaint();
}

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

        int thumbnailTopMargin = 18;
        juce::Rectangle<int> thumbnailBounds = getLocalBounds();
        thumbnailBounds.setHeight(thumbnailBounds.getHeight() -
                                  thumbnailTopMargin);
        thumbnailBounds.setY(thumbnailBounds.getY() + thumbnailTopMargin);

        thumbnail.drawChannels(g, thumbnailBounds, 0,
                               thumbnail.getTotalLength(), .7f);

        g.fillRect(0, 0, getWidth(), thumbnailTopMargin);
    }

    g.setColour(juce::Colour(0xFF33587F));
    g.drawText(this->correspondingClip->name, 2, 0, getWidth(), 20,
               juce::Justification::left, true);
}

void track::ClipComponent::mouseDown(const juce::MouseEvent &event) {
    if (event.mods.isRightButtonDown()) {
        DBG("rmb down");

        juce::PopupMenu contextMenu;
        contextMenu.addItem(1, "Reverse");
        contextMenu.addItem(2, "Cut");

        contextMenu.showMenuAsync(
            juce::PopupMenu::Options(), [this](int result) {
                if (result == 1) {
                    this->correspondingClip->reverse();
                    thumbnailCache.clear();
                    thumbnail.setSource(&correspondingClip->buffer, 44100.0, 2);
                    repaint();
                }

                else if (result == 2) {
                    TimelineComponent *tc =
                        (TimelineComponent *)getParentComponent();

                    tc->deleteClip(correspondingClip, trackIndex);
                }
            });
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
    repaint();
}

track::track *track::TrackComponent::TrackComponent::getCorrespondingTrack() {
    if (processor == nullptr) {
        DBG("TrackComponent's processor is nullptr");
    }
    if (trackIndex == -1)
        DBG("TrackComponent's trackIndex is -1");

    AudioPluginAudioProcessor *p = (AudioPluginAudioProcessor *)processor;
    return &(p->tracks[trackIndex]);
}

track::TrackComponent::TrackComponent(int trackIndex) : juce::Component() {

    addAndMakeVisible(muteBtn);
    muteBtn.setButtonText("M");

    this->trackIndex = trackIndex;

    muteBtn.onClick = [this] {
        getCorrespondingTrack()->m = !(getCorrespondingTrack()->m);
    };
}
track::TrackComponent::~TrackComponent() {}

void track::TrackComponent::paint(juce::Graphics &g) {
    g.fillAll(juce::Colour(0xFF5F5F5F));   // bg
    g.setColour(juce::Colour(0xFF535353)); // outline
    g.drawRect(getLocalBounds(), 2);

    juce::String trackName = getCorrespondingTrack() == nullptr
                                 ? "null trackk"
                                 : getCorrespondingTrack()->trackName;

    g.setColour(juce::Colour(0xFFDFDFDF));

    juce::Rectangle<int> textBounds = getLocalBounds();
    textBounds.setX(getLocalBounds().getX() + 10);

    g.drawText(trackName, textBounds, juce::Justification::left, true);
}

void track::TrackComponent::resized() {
    int btnHeight = 30;
    juce::Rectangle<int> btnBounds = juce::Rectangle<int>(
        UI_TRACK_WIDTH - 100, (UI_TRACK_HEIGHT / 2) - (btnHeight / 2), 30,
        btnHeight);

    muteBtn.setBounds(btnBounds);
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
    newTrackBtn.setButtonText("create new track");

    newTrackBtn.onClick = [this] {
        // this is annoying
        DBG("new track button clicked");

        auto p = (AudioPluginAudioProcessor *)this->processor;
        p->tracks.emplace_back();
        auto &t = p->tracks.back();

        int i = p->tracks.size() - 1;
        this->trackComponents.push_back(std::make_unique<TrackComponent>(i));
        trackComponents.back().get()->processor = processor;
        addAndMakeVisible(*trackComponents.back());

        DBG("track component added. will set track components bounds now");
        setTrackComponentBounds();
        repaint();
    };
}
track::Tracklist::~Tracklist() {}
void track::Tracklist::createTrackComponents() {
    AudioPluginAudioProcessor *p =
        (AudioPluginAudioProcessor *)(this->processor);

    int i = 0;
    for (track &t : p->tracks) {
        this->trackComponents.push_back(std::make_unique<TrackComponent>(i));
        trackComponents.back().get()->processor = processor;
        addAndMakeVisible(*trackComponents.back());

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

    newTrackBtn.setBounds(0,
                          UI_TRACK_VERTICAL_OFFSET +
                              (UI_TRACK_HEIGHT * counter) +
                              (UI_TRACK_VERTICAL_MARGIN * counter),
                          UI_TRACK_WIDTH, UI_TRACK_HEIGHT);

    DBG("setTrackComponentBounds() called");

    repaint();

    if (getParentComponent())
        getParentComponent()->repaint();
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

void track::clip::reverse() { buffer.reverse(0, buffer.getNumSamples()); }

void track::track::process(int numSamples, int currentSample) {
    if (m) {
        buffer.clear();
        return;
    }

    if (buffer.getNumSamples() != numSamples)
        buffer.setSize(2, numSamples, false, false, true);

    buffer.clear();

    int outputBufferLength = numSamples;
    int totalNumInputChannels = 2;

    for (clip &c : clips) {
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
}

/*
void track::group::process(juce::AudioBuffer<float> buffer, int currentSample) {
    DBG("trac::group::process() CALLED BUT NOT IMPLEMENTED YET");
}
*/
