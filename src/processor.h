#pragma once
#include "daw/track.h"
#include <JuceHeader.h>
#include <juce_audio_processors/juce_audio_processors.h>

class AudioPluginAudioProcessor : public juce::AudioProcessor {
  public:
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String &newName) override;

    void getStateInformation(juce::MemoryBlock &destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

     juce::String path =
    "C:/Users/USER/Downloads/hyperpop-trap-drums_151bpm_B_major.wav";
    // juce::String path = "/home/johnston/Downloads/tracktest.wav";

    std::vector<track::track> tracks;

    juce::AudioParameterFloat *masterGain;
  private:
    juce::Random random;

    juce::AudioFormatManager afm;
    // juce::String path = "/home/johnston/Downloads/tracktest.wav";
    juce::AudioBuffer<float> fileBuffer;
    // int position = 0;
    int startSample = 88200;

    bool prepared = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
};
