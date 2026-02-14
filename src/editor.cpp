#include "editor.h"
#include "daw/automation_relay.h"
#include "daw/clipboard.h"
#include "daw/defs.h"
#include "daw/plugin_chain.h"
#include "daw/timeline.h"
#include "daw/track.h"
#include "daw/utility.h"
#include "lookandfeel.h"
#include "processor.h"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <signal.h>

AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(
    AudioPluginAudioProcessor &p)
    : AudioProcessorEditor(&p), juce::ChangeListener(), latencyPoller(&p, this),
      processorRef(p), masterSliderAttachment(*p.masterGain, masterSlider) {

    juce::ignoreUnused(processorRef);

    // listen for messages from processor (for un/redoable actions UI updates)
    processorRef.addChangeListener(this);

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
#define MENU_UNDO 9
#define MENU_REDO 10
#define MENU_COPY_STATE 11
#define MENU_WRITE_STATE 12

        contextMenu.addItem(MENU_PLUGIN_SCAN, "Scan plugins");
        contextMenu.addItem(MENU_PLUGIN_LAZY_SCAN, "Lazy scan for plugins");
        contextMenu.addItem(MENU_CLEAR_SCANNED_PLUGINS,
                            "Clear scanned plugins");
        contextMenu.addItem(MENU_OPEN_RELAY_PARAMS_INSPECTOR,
                            "Open relay params inspector");
        contextMenu.addSeparator();
        contextMenu.addItem(MENU_UPDATE_LATENCY, "Update latency");
        contextMenu.addSeparator();
        contextMenu.addItem(MENU_UNDO, "Undo",
                            processorRef.undoManager.canUndo());
        contextMenu.addItem(MENU_REDO, "Redo",
                            processorRef.undoManager.canRedo());
        contextMenu.addSeparator();
        contextMenu.addItem(MENU_ABOUT, "About");
        contextMenu.addItem(MENU_BUILD_INFO, "Build info");
        contextMenu.addItem(MENU_COPY_STATE, "Copy state to clipboard");
        contextMenu.addItem(MENU_WRITE_STATE, "Write state from clipboard");

#if JUCE_DEBUG
        contextMenu.addSeparator();
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
            } else if (result == MENU_UNDO) {
                processorRef.undoManager.undo();
            } else if (result == MENU_REDO) {
                processorRef.undoManager.redo();
            } else if (result == MENU_COPY_STATE) {
                juce::MemoryBlock state;
                processorRef.getStateInformation(state);
                auto x = processorRef.getXmlFromBinary(state.getData(),
                                                       state.getSize());
                juce::SystemClipboard::copyTextToClipboard(x->toString());
            } else if (result == MENU_WRITE_STATE) {

                juce::NativeMessageBox::showAsync(
                    juce::MessageBoxOptions()
                        .withIconType(juce::MessageBoxIconType::WarningIcon)
                        .withTitle("Replace plugin state?")
                        .withMessage("Are you sure you want to replace plugin "
                                     "state? You cannot undo this within track")
                        .withButton("Affirmative.")
                        .withButton("No"),
                    [this](int result) {
                        if (result == 0) {
                            timelineComponent->clipComponents.clear();
                            tracklist.trackComponents.clear();
                            processorRef.undoManager.clearUndoHistory();

                            auto xmlText =
                                juce::SystemClipboard::getTextFromClipboard();
                            std::unique_ptr<juce::XmlElement> xml(
                                juce::XmlDocument::parse(xmlText));

                            juce::MemoryBlock state;
                            processorRef.copyXmlToBinary(*xml, state);
                            processorRef.setStateInformation(state.getData(),
                                                             state.getSize());

                            timelineComponent->clipComponentsUpdated = false;
                            timelineComponent->updateClipComponents();

                            tracklist.trackComponents.clear();
                            tracklist.createTrackComponents();
                            tracklist.setTrackComponentBounds();

                            timelineComponent->updateClipComponents();
                            timelineComponent->repaint();
                        }
                    });
            }
        });
    };

    addAndMakeVisible(configBtn);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() {
    stopTimer();
    processorRef.removeChangeListener(this);

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

void AudioPluginAudioProcessorEditor::changeListenerCallback(
    ChangeBroadcaster * /*source*/) {
    DBG("changeListenerCallback() called from editor");

    track::uiinstruction x = processorRef.GUIInstruction;

    if (x.command == UI_INSTRUCTION_UPDATE_CORE) {
        timelineComponent->clipComponents.clear();
        tracklist.trackComponents.clear();

        tracklist.createTrackComponents();
        tracklist.setTrackComponentBounds();
        tracklist.clearStains();

        timelineComponent->updateClipComponents();
    }

    else if (x.command == UI_INSTRUCTION_MARK_CC_STALE) {
        track::clip *c = (track::clip *)x.metadata;
        for (auto &cc : timelineComponent->clipComponents) {
            if (cc->correspondingClip == c) {
                DBG("stale match found!");
                cc->stale = true;
            }
        }
    }

    else if (x.command == UI_INSTRUCTION_UPDATE_STALE_TIMELINE) {
        timelineComponent->updateOnlyStaleClipComponents();
        timelineComponent->repaint();
    }

    else if (x.command == UI_INSTRUCTION_RECREATE_ALL_PNCS) {
        for (size_t i = 0; i < pluginChainComponents.size(); ++i) {
            if (pluginChainComponents[i]->route == x.r) {
                pluginChainComponents[i]
                    ->nodesWrapper.pluginNodeComponents.clear();
                pluginChainComponents[i]
                    ->nodesWrapper.createPluginNodeComponents();
            }
        }
    }

    else if (x.command == UI_INSTRUCTION_RECREATE_RELAY_NODES) {
        for (size_t i = 0; i < relayManagerCompnoents.size(); ++i) {
            if (relayManagerCompnoents[i]->route == x.r)
                relayManagerCompnoents[i]->rmNodesWrapper.createRelayNodes();
        }
    }

    else if (x.command == UI_INSTRUCTION_UPDATE_CLIP_COMPONENTS) {
        timelineComponent->updateClipComponents();
    }

    else if (x.command == UI_INSTRUCTION_UPDATE_NODE_COMPONENTS) {
        tracklist.trackComponents.clear();
        tracklist.createTrackComponents();
        tracklist.setTrackComponentBounds();
        tracklist.repaint();
    }

    else if (x.command == UI_INSTRUCTION_UPDATE_EXISTING_NODE_COMPONENTS) {
        tracklist.updateExistingTrackComponents();
    }

    else if (x.command == UI_INSTRUCTION_CLOSE_PEW) {
        closePluginEditorWindow(x.r, (int)(uintptr_t)x.metadata);
    }

    else if (x.command == UI_INSTRUCTION_CLOSE_ALL_FX_CHAINS_WITH_ROUTE) {
        closeAllFxChainsWithRoute(x.r);
    }

    else if (x.command == UI_INSTRUCTION_CLOSE_ALL_RMC_WITH_ROUTE_AND_INDEX) {
        closeAllRelayMenusWithRouteAndPluginIndex(x.r,
                                                  (int)(uintptr_t)x.metadata);
    }

    // close/opens
    else if (x.command == UI_INSTRUCTION_CLOSE_OPENED_EDITORS) {
        track::utility::closeOpenedEditors(x.r, &this->tmpOpenedEditors,
                                           &processorRef, this);
    }

    else if (x.command == UI_INSTRUCTION_OPEN_CLOSED_EDITORS) {
        track::utility::openEditors(x.r, this->tmpOpenedEditors, &processorRef,
                                    this);
        this->tmpOpenedEditors.clear();
    }

    else if (x.command == UI_INSTRUCTION_CLOSE_OPENED_RELAY_PARAM_WINDOWS) {
        track::utility::closeOpenedRelayParamWindows(x.r, &this->tmpOpenedRMCs,
                                                     &processorRef, this);
    }

    else if (x.command == UI_INSTRUCTION_OPEN_CLOSED_RELAY_PARAM_WINDOWS) {
        track::utility::openRelayParamWindows(x.r, this->tmpOpenedRMCs,
                                              &processorRef, this);
        this->tmpOpenedRMCs.clear();
    }

    else if (x.command == UI_INSTRUCTION_CLEAR_SUBWINDOWS) {
        pluginChainComponents.clear();
        pluginEditorWindows.clear();
        relayManagerCompnoents.clear();
    }
}

void AudioPluginAudioProcessorEditor::openRelayMenu(std::vector<int> route,
                                                    int pluginIndex) {
    timelineComponent
        ->grabKeyboardFocus(); // rid plugin node component of focus

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
        if (relayManagerCompnoents[(size_t)i]->route == route &&
            relayManagerCompnoents[(size_t)i]->pluginIndex == pluginIndex) {
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
    pcc->setBounds(10, 10, 900, 128);
    repaint();
}

void AudioPluginAudioProcessorEditor::closeAllFxChainsWithRoute(
    std::vector<int> route) {
    for (int i = pluginChainComponents.size() - 1; i >= 0; --i) {
        if (pluginChainComponents[(size_t)i]->route == route) {
            pluginChainComponents.erase(pluginChainComponents.begin() + i);
        }
    }
}

void AudioPluginAudioProcessorEditor::openPluginEditorWindow(
    std::vector<int> route, int pluginIndex) {
    timelineComponent
        ->grabKeyboardFocus(); // just take away focus from whatever had it

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
        juce::addDefaultFormatsToManager(apfm);

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

bool AudioPluginAudioProcessorEditor::keyStateChanged(bool isKeyDown) {
    juce::Component *focused = juce::Component::getCurrentlyFocusedComponent();

    // if we're typing then let that text editor take all keypresses
    // including ctrl+z/ctrl+y
    if (dynamic_cast<juce::TextEditor *>(focused)) {
        return false;
    }

    if (isKeyDown) {
        // ctrl+z
        if (juce::KeyPress::isKeyCurrentlyDown(90)) {
            DBG("");

            // this is temporary; in the future, implement a toast
            // system that tells the user smth "please wait, clips are still
            // rendering..." or smth idk bro i'm vibing to south arcade now,
            // bleed out bangs
            //
            // future john here: NOPE
            if (!timelineComponent->renderingWaveforms())
                processorRef.undoManager.undo();

            return true;
        }

        // ctrl+y
        else if (juce::KeyPress::isKeyCurrentlyDown(89)) {
            if (!timelineComponent->renderingWaveforms())
                processorRef.undoManager.redo();

            return true;
        }
    }

    return false;
}
