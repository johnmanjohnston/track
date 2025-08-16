#include "editor.h"
#include "daw/clipboard.h"
#include "daw/defs.h"
#include "daw/plugin_chain.h"
#include "daw/timeline.h"
#include "daw/track.h"
#include "juce_core/juce_core.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "lookandfeel.h"
#include "processor.h"

AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(
    AudioPluginAudioProcessor &p)
    : AudioProcessorEditor(&p), processorRef(p),
      masterSliderAttachment(p.apvts, "master", masterSlider) {

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

    scanBtn.setButtonText("scan");
    scanBtn.onClick = [this] { scan(); };

    configBtn.setButtonText("CONFIG");
    configBtn.onClick = [this] {
        juce::PopupMenu contextMenu;
        contextMenu.setLookAndFeel(&getLookAndFeel());

#define MENU_PLUGIN_SCAN 1
#define MENU_ABOUT 2

        contextMenu.addItem(MENU_PLUGIN_SCAN, "Scan plugins");
        contextMenu.addSeparator();
        contextMenu.addItem(MENU_ABOUT, "About");

        juce::PopupMenu::Options popupmenuOptions;
        contextMenu.showMenuAsync(popupmenuOptions, [this](int result) {
            if (result == MENU_PLUGIN_SCAN) {
                scan();
            }

            if (result == MENU_ABOUT) {
                juce::URL url("https://github.com/johnmanjohnston/track");
                url.launchInDefaultBrowser();
            }
        });
    };

    addAndMakeVisible(configBtn);
    addAndMakeVisible(scanBtn);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() {
    /*
    DBG("track plugin editor destructor called");
    DBG("plugin editor window vector size is " << pluginEditorWindows.size());
    DBG("plugin chain coomponents vector size is "
        << pluginChainComponents.size());
        */

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
    auto epicFont = lnf.getInterRegular().withExtraKerningFactor(-.05f);

    g.setFont(epicFont.withHeight(26.f).italicised().boldened());

    g.drawFittedText("track", juce::Rectangle<int>(34, 1, 100, 50),
                     juce::Justification::left, 1);

    g.setFont(epicFont.withHeight(14.f));

    // [x86_64-linux VST3 44.1kHz]

    juce::String audioInfoText = "[";

#ifdef __x86_64__
    audioInfoText += "x86_64-";
#endif

#if JUCE_LINUX
    audioInfoText += "linux ";
#elif JUCE_WINDOWS
    audioInfoText += "win";
#elif JUCE_MAC
    audioInfoText += "mac";
#elif JUCE_BSD
    audioInfoText += "bsd";
#endif

    audioInfoText +=
        AudioProcessor::getWrapperTypeDescription(processorRef.wrapperType);
    audioInfoText += " ";

    audioInfoText += juce::String(track::SAMPLE_RATE / 1000);
    audioInfoText += "kHz ";
    audioInfoText += juce::String(track::SAMPLES_PER_BLOCK);
    audioInfoText += "spls";
    audioInfoText += "]";
    g.drawText(audioInfoText, getWidth() - 250 - 2, 0, 250, 20,
               juce::Justification::right);
}

void AudioPluginAudioProcessorEditor::resized() {
    masterSlider.setBounds((getWidth() / 3) * 2, -8, 250, 70);

    int timeInfoBgWidth = 280;
    int timeInfoBgHeight = 40;

    juce::Rectangle<int> timeInfoRectangle = juce::Rectangle<int>(
        (getWidth() / 2) - (timeInfoBgWidth / 2),
        (track::UI_TOPBAR_HEIGHT / 2) - (timeInfoBgHeight / 2), timeInfoBgWidth,
        timeInfoBgHeight);

    transportStatus.setBounds(timeInfoRectangle);

    int scanBtnWidth = 50;
    scanBtn.setBounds(getWidth() - scanBtnWidth - 300, 1, 50, 20);

    configBtn.setColour(juce::TextButton::ColourIds::textColourOnId,
                        juce::Colours::orange);
    configBtn.setBounds(getWidth() - 70, 22, 60, 20);
}

void AudioPluginAudioProcessorEditor::openFxChain(std::vector<int> route) {
    pluginChainComponents.emplace_back(new track::PluginChainComponent());
    std::unique_ptr<track::PluginChainComponent> &pcc =
        pluginChainComponents.back();

    pcc->nodesWrapper.knownPluginList = &this->knownPluginList;
    pcc->route = route;
    pcc->processor = &processorRef;
    pcc->nodesWrapper.createPluginNodeComponents();

    addAndMakeVisible(*pcc);
    pcc->setBounds(10, 10, 900, 124);
    repaint();
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

void AudioPluginAudioProcessorEditor::scan() {
    if (apfm.getNumFormats() < 1)
        apfm.addDefaultFormats();

    if (pluginListComponent.get() == nullptr) {
        pluginListComponent = std::make_unique<juce::PluginListComponent>(
            apfm, knownPluginList, juce::File(), propertiesFile.get(), true);
    }

    juce::AudioPluginFormat *format = apfm.getFormat(0);
    pluginListComponent->scanFor(*format);
}
