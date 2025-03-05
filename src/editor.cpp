#include "editor.h"
#include "daw/defs.h"
#include "daw/timeline.h"
#include "daw/track.h"
#include "juce_gui_extra/misc/juce_PushNotifications.h"
#include "processor.h"

AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(
    AudioPluginAudioProcessor &p)
    : AudioProcessorEditor(&p), processorRef(p),
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
    tracklist.createTrackComponents();

    trackViewport.setViewedComponent(&tracklist, false);
    trackViewport.setBounds(0, 60, track::UI_TRACK_WIDTH, 655);
    trackViewport.setScrollBarsShown(true, false);
    addAndMakeVisible(trackViewport);

    trackViewport.timelineViewport = &timelineViewport;
    timelineViewport.trackViewport = &trackViewport;
    timelineViewport.tracklist = &tracklist;

    setLookAndFeel(&lnf);

    addAndMakeVisible(masterSlider);

    transportStatus.processorRef = &processorRef;
    addAndMakeVisible(transportStatus);

    startTimerHz(20);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() {
    setLookAndFeel(nullptr);
}

void AudioPluginAudioProcessorEditor::paint(juce::Graphics &g) {
    g.fillAll(juce::Colour(0xFF3D3D3D));

    g.setColour(juce::Colours::white);
    g.setFont(15.0f);

    // === TOP BAR ===
    g.setFont(lnf.getRobotoMonoThin());

    // draw top bar bg
    g.setColour(juce::Colour(0xFFCBCBCB));
    g.fillRect(0, 0, getWidth(), track::UI_TOPBAR_HEIGHT);

    // draw "track" logo
    g.setColour(juce::Colour(0xFF727272));
    g.setFont(24.f);
    g.drawFittedText("track", juce::Rectangle<int>(34, 1, 100, 50),
                     juce::Justification::left, 1);

    /*
    // draw time info bg
    g.setColour(juce::Colour(0xFF1B1E2D));
    int timeInfoBgWidth = 350;
    int timeInfoBgHeight = 40;

    juce::Rectangle<int> timeInfoRectangle = juce::Rectangle<int>(
        (getWidth() / 2) - (timeInfoBgWidth / 2),
        (track::UI_TOPBAR_HEIGHT / 2) - (timeInfoBgHeight / 2), timeInfoBgWidth,
        timeInfoBgHeight);
    g.fillRoundedRectangle(timeInfoRectangle.toFloat(), 4.f);

    // draw text for time info
    // bar.beat.division
    g.setColour(juce::Colour(0xFFCDD8E4));
    juce::String tempoInformationToDisplay = juce::String(bar);
    tempoInformationToDisplay.append(".", 1);
    tempoInformationToDisplay.append(juce::String(beat), 3);
    tempoInformationToDisplay.append(".", 1);
    tempoInformationToDisplay.append(juce::String(division), 3);

    juce::Rectangle<int> timeInfoTextRectangle = timeInfoRectangle;
    timeInfoTextRectangle.setWidth(timeInfoTextRectangle.getWidth() - 10);
    timeInfoTextRectangle.setX(timeInfoTextRectangle.getX() + 10);
    g.drawFittedText(tempoInformationToDisplay, timeInfoTextRectangle,
                     juce::Justification::left, 1);
    */
}

void AudioPluginAudioProcessorEditor::resized() {
    masterSlider.setBounds((getWidth() / 3) * 2, 2, 250, 70);

    int timeInfoBgWidth = 350;
    int timeInfoBgHeight = 40;

    juce::Rectangle<int> timeInfoRectangle = juce::Rectangle<int>(
        (getWidth() / 2) - (timeInfoBgWidth / 2),
        (track::UI_TOPBAR_HEIGHT / 2) - (timeInfoBgHeight / 2), timeInfoBgWidth,
        timeInfoBgHeight);

    // transportStatus.setBounds(10, 10, timeInfoBgWidth, timeInfoBgHeight);
    transportStatus.setBounds(timeInfoRectangle);
}
