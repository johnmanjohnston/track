#include "automation_relay.h"
#include "defs.h"
#include "subwindow.h"
#include "track.h"

float track::relayParam::getValueUsingPercentage(float min, float max) {
    float v = juce::jlimit(0.f, 100.f, this->value);
    float retval = -1.f;

    float mul = v / 100.f;
    return min + mul * (max - min);

    return retval;
}

track::RelayManagerComponent::RelayManagerComponent() : track::Subwindow() {
    rmViewport.setViewedComponent(&rmNodesWrapper);
    addAndMakeVisible(this->rmViewport);

    rmNodesWrapper.createRelayNodes();
}
track::RelayManagerComponent::~RelayManagerComponent() {}

track::audioNode *track::RelayManagerComponent::getCorrespondingTrack() {
    jassert(processor != nullptr);
    jassert(route.size() > 0);

    audioNode *head = &processor->tracks[(size_t)route[0]];
    for (size_t i = 1; i < route.size(); ++i) {
        head = &head->childNodes[(size_t)route[i]];
    }

    return head;
}

std::unique_ptr<track::subplugin> *track::RelayManagerComponent::getPlugin() {
    return &getCorrespondingTrack()->plugins[(size_t)this->pluginIndex];
}

void track::RelayManagerComponent::paint(juce::Graphics &g) {
    Subwindow::paint(g);

    g.setFont(getTitleBarFont());

    g.setColour(juce::Colour(0xFF'A7A7A7));
    g.drawText(getPlugin()->get()->plugin->getName(),
               getTitleBarBounds().withLeft(10).withTop(2),
               juce::Justification::left);

    int leftMargin =
        juce::GlyphArrangement::getStringWidthInt(
            g.getCurrentFont(), getPlugin()->get()->plugin->getName()) +
        10 + 10;
    g.setColour(juce::Colours::grey);
    g.drawText(juce::String(pluginIndex) + "/" +
                   getCorrespondingTrack()->trackName,
               getTitleBarBounds().withLeft(leftMargin).withTop(2),
               juce::Justification::left);
}

void track::RelayManagerComponent::resized() {
    // TODO: this
    Subwindow::resized();

    rmViewport.setBounds(0, UI_SUBWINDOW_TITLEBAR_HEIGHT, getWidth(),
                         getHeight() - UI_SUBWINDOW_TITLEBAR_HEIGHT);
    rmNodesWrapper.setBounds(0, 0, getWidth(), 2000);
}

void track::RelayManagerNodesWrapper::createRelayNodes() {
    for (int i = 0; i < 10; ++i) {
        DBG("creating relay node " << i);
        this->relayNodes.emplace_back(new RelayManagerNode);
        auto &x = relayNodes.back();
        x->setBounds(0, (i * 100), 200, 100 - 10);
        addAndMakeVisible(*x);
    }
}

void track::RelayManagerNode::paint(juce::Graphics &g) {
    // TODO: this
    g.fillAll(juce::Colours::green);
}
