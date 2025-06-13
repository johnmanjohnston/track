#include "processor.h"
#include "daw/defs.h"
#include "editor.h"
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_core/juce_core.h"
#include <complex>

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
    track::SAMPLE_RATE = sampleRate;
    track::SAMPLES_PER_BLOCK = samplesPerBlock;

    this->maxSamplesPerBlock = samplesPerBlock;

    updateLatency();

    for (track::audioNode &t : tracks) {
        t.processor = this;
        t.preparePlugins();
    }

    if (prepared)
        return;

    if (afm.findFormatForFileExtension("wav") == nullptr) {
        afm.registerBasicFormats();
    }

    bool createDummyTracks = false;
    if (createDummyTracks) {
        auto &g1 = tracks.emplace_back();
        g1.isTrack = false;
        g1.trackName = "g1";
        g1.processor = this;

        for (int i = 0; i < 5; ++i) {
            auto &subtrack = g1.childNodes.emplace_back();
            subtrack.trackName = "child track " + juce::String(i);
            subtrack.processor = this;
        }

        auto &sg = g1.childNodes.emplace_back();
        sg.isTrack = false;
        sg.trackName = "subgroup1";
        sg.processor = this;

        for (int i = 0; i < 3; ++i) {
            auto &subtrack = sg.childNodes.emplace_back();
            subtrack.trackName = "subgroup child " + juce::String(i);
            subtrack.processor = this;
        }

        auto &g2 = tracks.emplace_back();
        g2.trackName = "group 2";
        g2.isTrack = false;
        g2.processor = this;
        auto &x = g2.childNodes.emplace_back();
        x.trackName = "2nd parent's child whcih is a group";
        x.isTrack = false;
        x.processor = this;

        auto &gilbert = x.childNodes.emplace_back();
        gilbert.trackName = "gilbert :)";
        gilbert.isTrack = false;
        gilbert.processor = this;
        for (int i = 0; i < 3; ++i) {
            auto &sub = gilbert.childNodes.emplace_back();
            sub.trackName = juce::String(i);
            sub.isTrack = false;
            sub.processor = this;

            for (int j = 0; j < 4; ++j) {
                auto &y = sub.childNodes.emplace_back();
                y.trackName = juce::String(j);
                y.processor = this;
            }
        }
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

    /*
    track::track t;
    t.trackName = "beans";

    track::clip c;
    c.startPositionSample = startSample;
    c.path = path;
    c.updateBuffer();

    t.clips.push_back(c);
    tracks.push_back(t);
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
            for (track::audioNode &t : tracks) {
                t.process(buffer.getNumSamples(), currentSample);
            }

            // sum track buffers
            for (track::audioNode &t : tracks) {
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

juce::XmlElement *
AudioPluginAudioProcessor::serializeNode(track::audioNode *node) {
    DBG("serializing node " << node->trackName);
    juce::XmlElement *nodeElement = new juce::XmlElement("node");
    nodeElement->setAttribute("istrack", node->isTrack);
    nodeElement->setAttribute("name", node->trackName);

    jassert(node->plugins.size() == node->bypassedPlugins.size());
    for (size_t i = 0; i < node->plugins.size(); ++i) {
        auto &pluginInstance = node->plugins[i];
        juce::XmlElement *pluginElement = new juce::XmlElement("plugin");

        juce::String identifier =
            pluginInstance->getPluginDescription()
                .fileOrIdentifier.upToLastOccurrenceOf(".vst3", true, true);
        pluginElement->setAttribute("identifier", identifier);

        juce::MemoryBlock pluginData;
        pluginInstance->getStateInformation(pluginData);
        pluginElement->setAttribute("data", pluginData.toBase64Encoding());
        pluginElement->setAttribute("bypass", node->bypassedPlugins[i]);

        nodeElement->addChildElement(pluginElement);
    }

    if (node->isTrack) {
        for (size_t i = 0; i < node->clips.size(); ++i) {
            juce::XmlElement *clipElement = new juce::XmlElement("clip");
            track::clip *c = &node->clips[i];

            clipElement->setAttribute("active", c->active);

            clipElement->setAttribute("path", c->path);
            clipElement->setAttribute("start", c->startPositionSample);
            clipElement->setAttribute("name", c->name);

            nodeElement->addChildElement(clipElement);
        }
    } else {
        for (track::audioNode &child : node->childNodes) {
            nodeElement->addChildElement(serializeNode(&child));
        }
    }

    return nodeElement;
}

void AudioPluginAudioProcessor::deserializeNode(juce::XmlElement *nodeElement,
                                                track::audioNode *node) {
    DBG("deserializing - " << nodeElement->getStringAttribute("name"));
    node->isTrack = nodeElement->getBoolAttribute("istrack", true);
    node->trackName = nodeElement->getStringAttribute("name");
    node->processor = this;
    node->bypassedPlugins.clear();

    juce::XmlElement *pluginElement = nodeElement->getChildByName("plugin");
    for (size_t i = 0; pluginElement != nullptr; ++i) {
        juce::String identifier =
            pluginElement->getStringAttribute("identifier");
        node->addPlugin(identifier);

        // get plugin
        auto &pluginInstance = *node->plugins.back();

        // get and assign data from base64
        juce::String encodedPluginData =
            pluginElement->getStringAttribute("data");
        juce::MemoryBlock pluginData;

        pluginData.fromBase64Encoding(encodedPluginData);
        pluginInstance.setStateInformation(pluginData.getData(),
                                           pluginData.getSize());
        bool bypassed = pluginElement->getBoolAttribute("bypass", false);

        // addPlugin() already adds element to bypassedPlugins vector
        node->bypassedPlugins[i] = bypassed;

        pluginElement = pluginElement->getNextElementWithTagName("plugin");
    }

    if (node->isTrack) {
        juce::XmlElement *clipElement = nodeElement->getChildByName("clip");

        while (clipElement != nullptr) {
            // get clip data
            juce::String path = clipElement->getStringAttribute("path");
            juce::String clipName = clipElement->getStringAttribute("name");
            int start = clipElement->getIntAttribute("start");
            bool active = clipElement->getIntAttribute("active");

            // create clip instance
            node->clips.emplace_back();
            track::clip *c = &node->clips.back();
            c->active = active;
            c->path = path;
            c->name = clipName;
            c->startPositionSample = start;
            c->updateBuffer();

            clipElement = clipElement->getNextElementWithTagName("clip");
        }
    } else {
        juce::XmlElement *childElement = nodeElement->getChildByName("node");
        while (childElement != nullptr) {
            track::audioNode *child = &node->childNodes.emplace_back();
            deserializeNode(childElement, child);

            childElement = childElement->getNextElementWithTagName("node");
        }
    }
}

void AudioPluginAudioProcessor::getStateInformation(
    juce::MemoryBlock &destData) {
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());

    xml->deleteAllChildElementsWithTagName("node");
    xml->deleteAllChildElementsWithTagName("projectsettings");

    DBG("getStateInformation() called");

    juce::XmlElement *projectSettings = new juce::XmlElement("projectsettings");
    projectSettings->setAttribute("samplerate", getSampleRate());

    xml->addChildElement(projectSettings);

    for (size_t i = 0; i < tracks.size(); ++i) {
        xml->addChildElement(serializeNode(&tracks[i]));
    }

    copyXmlToBinary(*xml, destData);

    // DBG(xml->createDocument(""));
}

void AudioPluginAudioProcessor::setStateInformation(const void *data,
                                                    int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(
        getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr) {
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));

        tracks.clear();

        juce::XmlElement *projectSettings =
            xmlState->getChildByName("projectsettings");
        track::SAMPLE_RATE = projectSettings->getDoubleAttribute("samplerate");

        juce::XmlElement *nodeElement = xmlState->getChildByName("node");

        while (nodeElement != nullptr) {
            track::audioNode *node = &tracks.emplace_back();
            DBG("root deserialization call for " << node->trackName);
            deserializeNode(nodeElement, node);
            nodeElement = nodeElement->getNextElementWithTagName("node");
        }
    }

    // DBG(xmlState->createDocument(""));
}

void AudioPluginAudioProcessor::updateLatency() {
    DBG("updateLatency() called");
    int totalLatency = 0;
    for (track::audioNode &node : this->tracks) {
        DBG("calculating latency for node "
            << node.trackName << " which returned sample count of "
            << node.getTotalLatencySamples());
        totalLatency += node.getTotalLatencySamples();
    }

    DBG("updateLatency() called. new latency is " << totalLatency);
    setLatencySamples(totalLatency);
}

void AudioPluginAudioProcessor::reset() {
    // if (plugin != nullptr) plugin.reset();
}

// This creates new instances of the plugin.
// This function definition must be in the global namespace.
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
    return new AudioPluginAudioProcessor();
}
