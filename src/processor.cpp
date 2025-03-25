#include "processor.h"
#include "daw/track.h"
#include "editor.h"
#include "juce_audio_basics/juce_audio_basics.h"

AudioPluginAudioProcessor::AudioPluginAudioProcessor()
    : AudioProcessor(
          BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
              ),
      apvts(*this, nullptr, juce::Identifier("track"),
            {std::make_unique<juce::AudioParameterFloat>("master", "Master",
                                                         0.f, 6.f, 1.f)}) {
    // addParameter(masterGain = new juce::AudioParameterFloat("master",
    // "Master",
    // 0.f, 6.f, 1.f));
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor() {}

const juce::String AudioPluginAudioProcessor::getName() const {
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool AudioPluginAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int AudioPluginAudioProcessor::getNumPrograms() {
    return 1; // NB: some hosts don't cope very well if you tell them there are
              // 0 programs, so this should be at least 1, even if you're not
              // really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram() { return 0; }

void AudioPluginAudioProcessor::setCurrentProgram(int index) {
    juce::ignoreUnused(index);
}

const juce::String AudioPluginAudioProcessor::getProgramName(int index) {
    juce::ignoreUnused(index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName(int index,
                                                  const juce::String &newName) {
    juce::ignoreUnused(index, newName);
}

void AudioPluginAudioProcessor::prepareToPlay(double sampleRate,
                                              int samplesPerBlock) {
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    juce::ignoreUnused(sampleRate, samplesPerBlock);

    // this is for hosting sub plugins; revisit this later
    /*
        juce::String pluginPath =
            // juce::File("C:\\Program Files\\Common
    Files\\VST3\\MeldaProduction\\EQ\\MEqualizer.vst3").getFullPathName();
            juce::File("C:\\Program Files\\Common
    Files\\VST3\\ValhallaSupermassive.vst3").getFullPathName();
    OwnedArray<PluginDescription> pluginDescriptions;
    KnownPluginList plist;

    if (!prepared)
        pluginFormatManager.addDefaultFormats(); // only add default formats
    once, otherwise you fail an assertion

    for (int i = 0; i < pluginFormatManager.getNumFormats(); ++i)
        plist.scanAndAddFile(pluginPath, true, pluginDescriptions,
                             *pluginFormatManager.getFormat(i));

    jassert(pluginDescriptions.size() > 0);
    plugin = pluginFormatManager.createPluginInstance(
        *pluginDescriptions[0], sampleRate, samplesPerBlock, msg);

    plugin->setPlayConfigDetails(2, 2, sampleRate, samplesPerBlock);
    plugin.get()->prepareToPlay(sampleRate, samplesPerBlock);
    */

    if (prepared)
        return;

    if (afm.findFormatForFileExtension("wav") == nullptr) {
        afm.registerBasicFormats();
    }

    std::unique_ptr<juce::AudioFormatReader> reader(afm.createReaderFor(path));

    if (reader.get() != nullptr) {
        auto duration = reader->lengthInSamples / reader->sampleRate;
        fileBuffer.setSize((int)reader->numChannels,
                           (int)reader->lengthInSamples);

        reader->read(&fileBuffer, 0, (int)reader->lengthInSamples, 0, true,
                     true);

        DBG("Audio file loaded with "
            << reader->lengthInSamples << " samples, with sample rate of "
            << reader->sampleRate << " lasting " << duration << " seconds");
    }

    else {
        DBG("reader not created");
    }

    /*
    // initialize tracks
    // first track
    track::track t;
    t.trackName = "beans";

    track::clip c;
    c.startPositionSample = startSample;
    c.path = path;
    c.updateBuffer();

    t.clips.push_back(c);
    tracks.push_back(t);

    // second track
    track::track t2;
    t2.trackName = "potato";
    track::clip c2;
    c2.startPositionSample = 44100;
    // c2.path = "/home/johnston/Downloads/jldn.wav";
    c2.path = path;
    c2.updateBuffer();
    t2.clips.push_back(c2);
    tracks.push_back(t2);
    */

    DBG("prepareToPlay() called with sample rate " << sampleRate);
    DBG("total outputs: " << getTotalNumOutputChannels());
    DBG("total inputs: " << getTotalNumInputChannels());

    prepared = true;
}

void AudioPluginAudioProcessor::releaseResources() {
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    // if (plugin != nullptr)
    // plugin->releaseResources();
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const {
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

        // This checks if the input layout matches the output layout
#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}

void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                             juce::MidiBuffer &midiMessages) {
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // DBG(foundPlugins[0]->name);

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    //

    auto playhead = getPlayHead();
    bool playheadExists = playhead != nullptr;

    if (playheadExists) {
        if (playhead->getPosition()->getIsPlaying() == true) {

            int currentSample = *playhead->getPosition()->getTimeInSamples();
            int outputBufferLength = buffer.getNumSamples();

            // process tracks; tracks populate their internal buffer
            for (track::track &t : tracks) {
                t.process(buffer.getNumSamples(), currentSample);
            }

            // sum track buffers
            for (track::track &t : tracks) {
                for (int channel = 0; channel < totalNumOutputChannels;
                     ++channel) {

                    buffer.addFrom(channel, 0, t.buffer, channel, 0,
                                   buffer.getNumSamples());
                }
            }
        }
    }

    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
        auto *channelData = buffer.getWritePointer(channel);
        juce::ignoreUnused(channelData);
    }

    /*
    if (plugin != nullptr) {
        plugin.get()->setPlayHead(getPlayHead());
        plugin.get()->processBlock(buffer, mb);
    }

    if (plugin == nullptr) {
        DBG("plugin is null. msg says " << msg);
    }
    */

    buffer.applyGain(apvts.getParameter("master")->getValue());
}

bool AudioPluginAudioProcessor::hasEditor() const {
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *AudioPluginAudioProcessor::createEditor() {
    // if (plugin != nullptr)
    // return plugin->createEditorIfNeeded();

    return new AudioPluginAudioProcessorEditor(*this);
}

void AudioPluginAudioProcessor::getStateInformation(
    juce::MemoryBlock &destData) {
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());

    xml->deleteAllChildElementsWithTagName("track");

    DBG("getStateInformation() called");

    /*
    xml->addChildElement(new juce::XmlElement("huzzah"));
    xml->getChildByName("huzzah")->setAttribute("val",
                                                "wowowiw this is a string");
                                                */

    /*
    <track name="Jarvis" index="0">
        <clip
            path="/clearly/you/dont/have/an/airfryer.wav"
            start="44100"
            name="ironman">
        </clip>
    </track>
    */
    for (int i = 0; i < tracks.size(); ++i) {
        juce::XmlElement *trackElement = new juce::XmlElement("track");
        trackElement->setAttribute("name", tracks[i].trackName);
        trackElement->setAttribute("index", i);

        for (int j = 0; j < tracks[i].clips.size(); ++j) {
            juce::XmlElement *clipElement = new juce::XmlElement("clip");
            track::clip *c = &tracks[i].clips[j];

            clipElement->setAttribute("path", c->path);
            clipElement->setAttribute("start", c->startPositionSample);
            clipElement->setAttribute("name", c->name);

            trackElement->addChildElement(clipElement);
        }

        xml->addChildElement(trackElement);
    }

    copyXmlToBinary(*xml, destData);

    DBG(xml->createDocument(""));
}

void AudioPluginAudioProcessor::setStateInformation(const void *data,
                                                    int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(
        getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr) {
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));

        /*
        DBG("setStateInformation() called");
        auto a = xmlState.get()->getChildByName("huzzah");
        if (a == nullptr)
            DBG("! a IS NULL :(((");
        else {
            DBG("a is NOT null :)");
            auto &x = a->getAttributeValue(0);
            DBG(x);
        }
        */

        /*
        auto a = xmlState->getChildByName("track");
        jassert(a != nullptr);
        auto &v = a->getAttributeValue(0);
        DBG("v = " << v);
        */

        tracks.clear();

        juce::XmlElement *trackElement = xmlState->getChildByName("track");
        while (trackElement != nullptr) {
            // get track data
            juce::String trackName = trackElement->getStringAttribute("name");
            // DBG("FOUND TRACK " << trackName);
            juce::XmlElement *clipElement =
                trackElement->getChildByName("clip");

            // create track instance
            tracks.emplace_back();
            track::track *t = &tracks.back();
            t->trackName = trackName;

            while (clipElement != nullptr) {
                // get clip data
                juce::String path = clipElement->getStringAttribute("path");
                juce::String clipName = clipElement->getStringAttribute("name");
                int start = clipElement->getIntAttribute("start");

                // create clip instance
                t->clips.emplace_back();
                track::clip *c = &t->clips.back();
                c->path = path;
                c->name = clipName;
                c->startPositionSample = start;
                c->updateBuffer();

                clipElement = clipElement->getNextElementWithTagName("clip");
            }

            trackElement = trackElement->getNextElementWithTagName("track");
        }
    }

    DBG(xmlState->createDocument(""));
}

void AudioPluginAudioProcessor::reset() {
    // if (plugin != nullptr) plugin.reset();
}

// This creates new instances of the plugin.
// This function definition must be in the global namespace.
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
    return new AudioPluginAudioProcessor();
}
