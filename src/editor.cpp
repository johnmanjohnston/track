#include "editor.h"
#include "daw/automation_relay.h"
#include "daw/clipboard.h"
#include "daw/defs.h"
#include "daw/plugin_chain.h"
#include "daw/timeline.h"
#include "daw/track.h"
#include "lookandfeel.h"
#include "processor.h"
#include <cmath>
#include <cstddef>
#include <signal.h>

AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(
    AudioPluginAudioProcessor &p)
    : AudioProcessorEditor(&p), latencyPoller(&p, this), processorRef(p),
      masterSliderAttachment(*p.masterGain, masterSlider) {

    juce::ignoreUnused(processorRef);

    // sizing
    setResizable(true, true);
    setResizeLimits(540, 360, 1280, 720);
    setSize(1280, 720);

    // TimelineViewport holds TimelineComponent
    timelineComponent->viewport = &timelineViewport;
    timelineComponent->processorRef = &processorRef;
    timelineViewport.setViewedComponent(timelineComponent.get(), true);
    timelineViewport.setBounds(track::UI_TRACK_WIDTH, 55,
                               getWidth() - track::UI_TRACK_WIDTH, 665);
    addAndMakeVisible(timelineViewport);

    // tracks
    tracklist.setBounds(0, 60, track::UI_TRACK_WIDTH, 2000);
    tracklist.processor = (void *)&processorRef;
    tracklist.timelineComponent = (void *)timelineComponent.get();
    tracklist.createTrackComponents();

    trackViewport.setViewedComponent(&tracklist, false);
    trackViewport.setBounds(0, 55, track::UI_TRACK_WIDTH, 655);
    trackViewport.setScrollBarsShown(true, false);
    addAndMakeVisible(trackViewport);

    trackViewport.timelineViewport = &timelineViewport;
    timelineViewport.trackViewport = &trackViewport;
    timelineViewport.tracklist = &tracklist;

    setLookAndFeel(&lnf);

    addAndMakeVisible(masterSlider);

    transportStatus.processorRef = &processorRef;
    addAndMakeVisible(transportStatus);

    playhead.processor = &processorRef;
    playhead.tv = &timelineViewport;
    addAndMakeVisible(playhead);

    startTimerHz(20);

    int tcHeight = processorRef.tracks.size() * (size_t)track::UI_TRACK_HEIGHT;
    timelineComponent->setSize(
        4000, juce::jmax(tcHeight, timelineViewport.getHeight()) - 4);

    // addAndMakeVisible(pluginChainComponent);
    // pluginChainComponent.setBounds(1, 1, 1, 1);
    //

    masterSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox,
                                 true, 0, 0);

    configBtn.setButtonText("CONFIG");
    configBtn.onClick = [this] {
        juce::PopupMenu contextMenu;
        contextMenu.setLookAndFeel(&getLookAndFeel());

#define MENU_PLUGIN_SCAN 1
#define MENU_PLUGIN_LAZY_SCAN 4
#define MENU_CLEAR_SCANNED_PLUGINS 5
#define MENU_ABOUT 2
#define MENU_UPDATE_LATENCY 3
#define MENU_BUILD_INFO 6
#define MENU_OPEN_RELAY_PARAMS_INSPECTOR 7
#define MENU_RAISE_SEGFAULT 8

        contextMenu.addItem(MENU_PLUGIN_SCAN, "Scan plugins");
        contextMenu.addItem(MENU_PLUGIN_LAZY_SCAN, "Lazy scan for plugins");
        contextMenu.addItem(MENU_CLEAR_SCANNED_PLUGINS,
                            "Clear scanned plugins");
        contextMenu.addItem(MENU_OPEN_RELAY_PARAMS_INSPECTOR,
                            "Open relay params inspector");
        contextMenu.addSeparator();
        contextMenu.addItem(MENU_UPDATE_LATENCY, "Update latency");
        contextMenu.addSeparator();
        contextMenu.addItem(MENU_ABOUT, "About");
        contextMenu.addItem(MENU_BUILD_INFO, "Build info");

#if JUCE_DEBUG
        contextMenu.addItem(MENU_RAISE_SEGFAULT, "Segfault");
#endif
        juce::PopupMenu::Options popupmenuOptions;
        contextMenu.showMenuAsync(popupmenuOptions, [this](int result) {
            if (result == MENU_PLUGIN_SCAN) {
                scan();
            }

            else if (result == MENU_PLUGIN_LAZY_SCAN) {
                lazyScan();
            }

            else if (result == MENU_CLEAR_SCANNED_PLUGINS) {
                this->processorRef.knownPluginList.clear();
            }

            else if (result == MENU_ABOUT) {
                juce::URL url("https://github.com/johnmanjohnston/track");
                url.launchInDefaultBrowser();
            }

            else if (result == MENU_UPDATE_LATENCY) {
                processorRef.updateLatency();
                latencyPoller.updateLastKnownLatency();
                repaint();
            }

            else if (result == MENU_OPEN_RELAY_PARAMS_INSPECTOR) {
                openRelayParamInspector();
            }

            else if (result == MENU_BUILD_INFO) {
                juce::String buildType = "Release";
#if JUCE_DEBUG
                buildType = "Debug";
#endif

                juce::String juceVersionString =
                    juce::String(JUCE_MAJOR_VERSION) + "." +
                    juce::String(JUCE_MINOR_VERSION) + "." +
                    juce::String(JUCE_BUILDNUMBER);

                juce::String buildInfoString =
                    "track " + buildType + " " + juce::String(VERSION_STRING) +
                    "\n" + "JUCE v" + juceVersionString;

                juce::NativeMessageBox::showMessageBoxAsync(
                    juce::MessageBoxIconType::InfoIcon, "track build info",
                    buildInfoString);
            }

            else if (result == MENU_RAISE_SEGFAULT) {
                raise(SIGSEGV);
            }
        });
    };

    addAndMakeVisible(configBtn);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() {
    /*
    DBG("track plugin editor destructor called");
    DBG("plugin editor window vector size is " << pluginEditorWindows.size());
    DBG("plugin chain coomponents vector size is "
        << pluginChainComponents.size());
        */

    stopTimer();

    setLookAndFeel(nullptr);
    track::clipboard::releaseResources();
}

void AudioPluginAudioProcessorEditor::paint(juce::Graphics &g) {
    g.fillAll(juce::Colour(0xFF3D3D3D));

    g.setColour(juce::Colours::white);
    g.setFont(15.0f);

    // === TOP BAR ===
    g.setFont(lnf.getRobotoMonoThin());

    // draw top bar bg
    g.setColour(juce::Colour(0xFFCBCBCB).brighter(0.2f));
    g.fillRect(0, 0, getWidth(), track::UI_TOPBAR_HEIGHT);

    // draw "track" logo
    g.setColour(juce::Colour(0xFF727272));
    auto epicFont = lnf.getInterRegular();

    epicFont = epicFont.withExtraKerningFactor(-.05f);

    g.setFont(epicFont.withHeight(26.f).italicised().boldened());

    g.drawFittedText("track", juce::Rectangle<int>(34, 1, 100, 50),
                     juce::Justification::left, 1);

#if JUCE_LINUX
    g.setFont(epicFont.withHeight(14.f));
#else
    g.setFont(epicFont.withHeight(15.f).withExtraKerningFactor(-0.02));
#endif

    // [x86_64-linux VST3 44.1kHz 512spls 1ms]

    juce::String audioInfoText = "[";

    juce::String versionInfo = "v";
    versionInfo += VERSION_STRING;
    versionInfo += " ";
    audioInfoText += versionInfo;

#ifdef __x86_64__
    audioInfoText += "x86_64-";
#endif

#if _WIN32
    audioInfoText += "x86_64-";
#endif

#if JUCE_LINUX
    audioInfoText += "linux ";
#elif JUCE_WINDOWS
    audioInfoText += "win ";
#elif JUCE_MAC
    audioInfoText += "mac ";
#elif JUCE_BSD
    audioInfoText += "bsd ";
#endif

    audioInfoText +=
        AudioProcessor::getWrapperTypeDescription(processorRef.wrapperType);
    audioInfoText += " ";

    audioInfoText += juce::String(track::SAMPLE_RATE / 1000);
    audioInfoText += "kHz ";
    audioInfoText += juce::String(track::SAMPLES_PER_BLOCK);

    audioInfoText += "spls ";
    if (latencyPoller.knownLatencySamples == -1)
        audioInfoText += "-";
    else {
        int latencyMs = std::round(
            (latencyPoller.knownLatencySamples / track::SAMPLE_RATE) * 1000.f);
        audioInfoText += latencyMs;
        audioInfoText += "ms";
    }

    audioInfoText += "]";
    g.drawText(audioInfoText, getWidth() - 290 - 3, 0, 290, 20,
               juce::Justification::right, false);
}

void AudioPluginAudioProcessorEditor::resized() {
    masterSlider.setBounds(((getWidth() / 3) * 2) - 30, -8, 200, 70);

    int timeInfoBgWidth = 280;
    int timeInfoBgHeight = 40;

    juce::Rectangle<int> timeInfoRectangle = juce::Rectangle<int>(
        (getWidth() / 2) - (timeInfoBgWidth / 2),
        (track::UI_TOPBAR_HEIGHT / 2) - (timeInfoBgHeight / 2), timeInfoBgWidth,
        timeInfoBgHeight);

    transportStatus.setBounds(timeInfoRectangle);

    configBtn.setColour(juce::TextButton::ColourIds::textColourOnId,
                        juce::Colours::orange);
    configBtn.setBounds(getWidth() - 66, 25, 62, 20);
}

void AudioPluginAudioProcessorEditor::openRelayMenu(std::vector<int> route,
                                                    int pluginIndex) {
    relayManagerCompnoents.emplace_back(new track::RelayManagerComponent());
    std::unique_ptr<track::RelayManagerComponent> &rmc =
        relayManagerCompnoents.back();

    rmc->route = route;
    rmc->pluginIndex = pluginIndex;
    rmc->processor = &processorRef;

    rmc->rmNodesWrapper.createRelayNodes();

    addAndMakeVisible(*rmc);
    rmc->setBounds(10, 10, 280, 640);
    repaint();
}

void AudioPluginAudioProcessorEditor::closeRelayMenu(std::vector<int> route,
                                                     int pluginIndex) {
    for (size_t i = 0; i < relayManagerCompnoents.size(); ++i) {
        if (relayManagerCompnoents[i]->route == route &&
            relayManagerCompnoents[i]->pluginIndex == pluginIndex) {
            this->relayManagerCompnoents.erase(
                this->relayManagerCompnoents.begin() + (long)i);
            break;
        }
    }
}

bool AudioPluginAudioProcessorEditor::isRelayMenuOpened(std::vector<int> route,
                                                        int pluginIndex) {
    bool retval = false;

    for (size_t i = 0; i < relayManagerCompnoents.size(); ++i) {
        if (relayManagerCompnoents[i]->route == route) {
            if (relayManagerCompnoents[i]->pluginIndex == pluginIndex) {
                retval = true;
                break;
            }
        }
    }

    return retval;
}

void AudioPluginAudioProcessorEditor::closeAllRelayMenusWithRouteAndPluginIndex(
    std::vector<int> route, int pluginIndex) {
    for (int i = relayManagerCompnoents.size() - 1; i >= 0; --i) {
        if (relayManagerCompnoents[i]->route == route &&
            relayManagerCompnoents[i]->pluginIndex == pluginIndex) {
            relayManagerCompnoents.erase(relayManagerCompnoents.begin() + i);
        }
    }
}

void AudioPluginAudioProcessorEditor::openRelayParamInspector() {
    relayParamInspector = std::make_unique<track::RelayParamInspector>();
    relayParamInspector->rpiComponent.processor = &processorRef;

    relayParamInspector->initSliders();

    addAndMakeVisible(*relayParamInspector);
    relayParamInspector->setBounds(10, 10, 300, 640);
}

void AudioPluginAudioProcessorEditor::openFxChain(std::vector<int> route) {
    pluginChainComponents.emplace_back(new track::PluginChainComponent());
    std::unique_ptr<track::PluginChainComponent> &pcc =
        pluginChainComponents.back();

    pcc->nodesWrapper.knownPluginList = &this->processorRef.knownPluginList;
    pcc->route = route;
    pcc->processor = &processorRef;
    pcc->nodesWrapper.createPluginNodeComponents();

    addAndMakeVisible(*pcc);
    pcc->setBounds(10, 10, 900, 124);
    repaint();
}

void AudioPluginAudioProcessorEditor::closeAllFxChainsWithRoute(
    std::vector<int> route) {
    for (int i = pluginChainComponents.size() - 1; i >= 0; --i) {
        if (pluginChainComponents[i]->route == route) {
            pluginChainComponents.erase(pluginChainComponents.begin() + i);
        }
    }
}

void AudioPluginAudioProcessorEditor::openPluginEditorWindow(
    std::vector<int> route, int pluginIndex) {
    pluginEditorWindows.emplace_back(new track::PluginEditorWindow());
    std::unique_ptr<track::PluginEditorWindow> &pew =
        pluginEditorWindows.back();

    pew->route = route;
    pew->pluginIndex = pluginIndex;
    pew->processor = &processorRef;
    pew->createEditor();
    addAndMakeVisible(*pew);

    pew->setBounds(400, 0, 100, 100);
    repaint();
}

void AudioPluginAudioProcessorEditor::openClipPropertiesWindows(
    std::vector<int> route, int clipIndex) {
    clipPropertiesWindows.emplace_back(new track::ClipPropertiesWindow());

    std::unique_ptr<track::ClipPropertiesWindow> &cpw =
        clipPropertiesWindows.back();

    cpw->setBounds(10, 10, 300, 140);
    addAndMakeVisible(*cpw);

    cpw->route = route;
    cpw->clipIndex = clipIndex;
    cpw->p = &processor;

    cpw->init();
}

void AudioPluginAudioProcessorEditor::closePluginEditorWindow(
    std::vector<int> route, int pluginIndex) {
    for (size_t i = 0; i < this->pluginEditorWindows.size(); ++i) {
        if (pluginEditorWindows[i]->route == route &&
            pluginEditorWindows[i]->pluginIndex == pluginIndex) {
            pluginEditorWindows.erase(pluginEditorWindows.begin() + (int)i);
            break;
        }
    }
}

bool AudioPluginAudioProcessorEditor::isPluginEditorWindowOpen(
    std::vector<int> route, int pluginIndex) {
    bool retval = false;

    for (size_t i = 0; i < this->pluginEditorWindows.size(); ++i) {
        if (pluginEditorWindows[i]->route == route) {
            if (pluginEditorWindows[i]->pluginIndex == pluginIndex) {
                retval = true;
                break;
            }
        }
    }

    return retval;
}

void AudioPluginAudioProcessorEditor::scan() {
    if (apfm.getNumFormats() < 1)
        apfm.addDefaultFormats();

    if (pluginListComponent.get() == nullptr) {
        pluginListComponent = std::make_unique<juce::PluginListComponent>(
            apfm, this->processorRef.knownPluginList, juce::File(),
            propertiesFile.get(), true);
    }

    juce::AudioPluginFormat *format = apfm.getFormat(0);
    pluginListComponent->scanFor(*format);
}

void AudioPluginAudioProcessorEditor::lazyScan() {
    DBG("lazyScan() called");

    processorRef.knownPluginList.clear();

    fileChooser = std::make_unique<juce::FileChooser>(
        "Select folders to scan...",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.vst3");

    int flags = juce::FileBrowserComponent::openMode |
                juce::FileBrowserComponent::canSelectDirectories |
                juce::FileBrowserComponent::canSelectMultipleItems;

    fileChooser->launchAsync(flags, [this](const juce::FileChooser &chooser) {
        DBG("file chooser deployed KAJSDFJKASDF");

        for (auto searchDir : chooser.getResults()) {
            juce::String currentDir = searchDir.getFullPathName();

            juce::FileSearchPath fsp(currentDir);

            auto pluginFileEntries = fsp.findChildFiles(
                juce::File::TypesOfFileToFind::findFilesAndDirectories, true,
                "*.vst3");

            for (auto f : pluginFileEntries) {
                DBG("f: " << f.getFileName() << " at " << f.getFullPathName());

                // find file entries for .vst3 files and create dummy plugin
                // description data
                juce::PluginDescription pd;
                pd.name = f.getFileName().dropLastCharacters(5); // ".vst3"
                pd.fileOrIdentifier = f.getFullPathName();
                pd.numInputChannels = 2;
                pd.numOutputChannels = 2;

                processorRef.knownPluginList.addType(pd);
            }
        }
    });
}
