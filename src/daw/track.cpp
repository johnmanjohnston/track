#include "track.h"
#include "../editor.h"
#include "../processor.h"
#include "clipboard.h"
#include "defs.h"
#include "juce_dsp/juce_dsp.h"
#include "juce_events/juce_events.h"
#include "juce_gui_basics/juce_gui_basics.h"
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

    clipNameLabel.setFont(
        getInterSemiBold().withHeight(16.f).withExtraKerningFactor(-.02f));
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
            g.fillAll(juce::Colours::blue);
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

        int thumbnailTopMargin = 18;
        juce::Rectangle<int> thumbnailBounds = getLocalBounds().reduced(2);
        thumbnailBounds.setHeight(thumbnailBounds.getHeight() -
                                  thumbnailTopMargin);
        thumbnailBounds.setY(thumbnailBounds.getY() + thumbnailTopMargin);

        thumbnail.drawChannels(g, thumbnailBounds, 0,
                               thumbnail.getTotalLength(), .7f);
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

        contextMenu.addItem(1, "Reverse");
        contextMenu.addItem(2, "Cut");
        contextMenu.addItem(3, "Toggle activate/deactive clip");
        contextMenu.addItem(4, "Copy clip");

        juce::PopupMenu::Options options;

        contextMenu.showMenuAsync(options, [this](int result) {
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

            else if (result == 3) {
                this->correspondingClip->active =
                    !this->correspondingClip->active;
                repaint();
            }

            else if (result == 4) {
                clipboard::setData(correspondingClip, TYPECODE_CLIP);
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
            (distanceMovedHorizontally * 41000) / UI_ZOOM_MULTIPLIER;

        TimelineComponent *tc = (TimelineComponent *)getParentComponent();
        jassert(tc != nullptr);
        tc->resizeTimelineComponent();
    }

    DBG(event.getDistanceFromDragStartX());
    repaint();
}

track::track *track::TrackComponent::TrackComponent::getCorrespondingTrack() {
    if (processor == nullptr) {
        DBG("! TRACK COMPONENT'S getCorrespondingTrack() RETURNED nullptr");
        DBG("TrackComponent's processor is nullptr");
        return nullptr;
    }
    if (trackIndex == -1) {
        DBG("! TRACK COMPONENT'S getCorrespondingTrack() RETURNED nullptr");
        DBG("TrackComponent's trackIndex is -1");
        return nullptr;
    }

    AudioPluginAudioProcessor *p = (AudioPluginAudioProcessor *)processor;
    return &(p->tracks[trackIndex]);
}

void track::TrackComponent::initializeGainSlider() {
    DBG("intiailizGainSlider() called");
    DBG("track's gain is " << getCorrespondingTrack()->gain);
    gainSlider.setValue(getCorrespondingTrack()->gain);
}

track::TrackComponent::TrackComponent(int trackIndex) : juce::Component() {
    // starting text for track name label is set when TrackComponent is created
    // in createTrackComponents()
    trackNameLabel.setFont(getInterRegular().withHeight(17.f));
    trackNameLabel.setBounds(0, 0, 100, 20);
    trackNameLabel.setEditable(true);
    addAndMakeVisible(trackNameLabel);

    trackNameLabel.onTextChange = [this] {
        this->getCorrespondingTrack()->trackName = trackNameLabel.getText(true);
    };

    this->trackIndex = trackIndex;

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
        editor->openFxChain(this->trackIndex);
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
    trackNameLabel.setBounds(getLocalBounds().getX() + 10,
                             (UI_TRACK_HEIGHT / 4) - 5, 100, 20);

    int btnSize = 24;
    int btnHeight = btnSize;
    int btnWidth = btnSize;

    juce::Rectangle<int> btnBounds = juce::Rectangle<int>(
        UI_TRACK_WIDTH - 115, (UI_TRACK_HEIGHT / 2) - (btnHeight / 2), btnWidth,
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
    newTrackBtn.setButtonText("create new track");

    newTrackBtn.onClick = [this] {
        // this is annoying
        DBG("new track button clicked");

        auto p = (AudioPluginAudioProcessor *)this->processor;
        p->tracks.emplace_back();
        auto &t = p->tracks.back();
        t.processor = this->processor;

        int i = p->tracks.size() - 1;
        this->trackComponents.push_back(std::make_unique<TrackComponent>(i));
        trackComponents.back().get()->processor = processor;
        trackComponents.back().get()->initializeGainSlider();

        // set track label text
        trackComponents.back().get()->trackNameLabel.setText(
            trackComponents.back().get()->getCorrespondingTrack()->trackName,
            juce::NotificationType::dontSendNotification);

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
        trackComponents.back().get()->initializeGainSlider();
        trackComponents.back().get()->trackNameLabel.setText(
            t.trackName, juce::NotificationType::dontSendNotification);
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
    buffer =
        juce::AudioBuffer<float>(reader->numChannels, reader->lengthInSamples);

    reader->read(&buffer, 0, buffer.getNumSamples(), 0, true, true);
}

void track::clip::reverse() { buffer.reverse(0, buffer.getNumSamples()); }

void track::track::addPlugin(juce::String path) {
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
    plugin = apfm.createPluginInstance(*pluginDescriptions[0], sampleRate,
                                       maxSamplesPerBlock, errorMsg);

    jassert(pluginDescriptions.size() > 0);

    DBG("LOGGING OUTPUTS:");
    DBG("   " << plugin->getMainBusNumInputChannels() << " inputs");
    DBG("   " << plugin->getMainBusNumOutputChannels() << " outputs");

    // DBG("setting plugin play config details");
    plugin->setPlayConfigDetails(2, 2, sampleRate, maxSamplesPerBlock);

    /*
    DBG("setting buses");
    AudioPluginAudioProcessor *p = (AudioPluginAudioProcessor *)processor;
    AudioPluginAudioProcessor::BusesLayout busesLayout = p->getBusesLayout();
    plugin->setBusesLayout(busesLayout);

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

void track::track::preparePlugins(int newMaxSamplesPerBlock,
                                  int newSampleRate) {
    for (std::unique_ptr<juce::AudioPluginInstance> &plugin : plugins) {
        plugin->prepareToPlay(newMaxSamplesPerBlock, newSampleRate);
    }
}

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

/*
void track::group::process(juce::AudioBuffer<float> buffer, int currentSample) {
    DBG("trac::group::process() CALLED BUT NOT IMPLEMENTED YET");
}
*/
