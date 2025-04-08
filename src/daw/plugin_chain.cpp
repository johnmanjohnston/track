#include "plugin_chain.h"

track::PluginChainComponent::PluginChainComponent() : juce::Component() {
    setSize(500, 500);
}

track::PluginChainComponent::~PluginChainComponent() {}

void track::PluginChainComponent::paint(juce::Graphics &g) {
    g.fillAll(juce::Colours::red);
}
