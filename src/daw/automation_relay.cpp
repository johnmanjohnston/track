#include "automation_relay.h"
#include "defs.h"
#include "subwindow.h"
#include "track.h"

float track::relayParam::getValueUsingPercentage(float min, float max) {
    float v = juce::jlimit(0.f, 100.f, this->percentage);
    float retval = -1.f;

    float mul = v / 100.f;
    return min + mul * (max - min);

    return retval;
}

track::RelayManagerComponent::RelayManagerComponent() : track::Subwindow() {
    rmViewport.setViewedComponent(&rmNodesWrapper);
    addAndMakeVisible(this->rmViewport);
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
    /*
    for (int i = 0; i < 10; ++i) {
        DBG("creating relay node " << i);
        this->relayNodes.emplace_back(new RelayManagerNode);
        auto &x = relayNodes.back();
        addAndMakeVisible(*x);
    }
    */

    relayNodes.clear();

    track::RelayManagerComponent *rmc = (RelayManagerComponent *)
        findParentComponentOfClass<RelayManagerComponent>();
    jassert(rmc != nullptr);

    jassert(rmc->processor != nullptr);

    std::unique_ptr<track::subplugin> *plugin = rmc->getPlugin();
    jassert(plugin != nullptr);

    for (size_t i = 0; i < plugin->get()->relayParams.size(); ++i) {
        relayNodes.emplace_back(new RelayManagerNode());
        auto &x = relayNodes.back();
        x->paramVectorIndex = (int)i;
        x->rmc = rmc;
        x->createMenuEntries();
        addAndMakeVisible(*x);
    }

    setRelayNodesBounds();
}

void track::RelayManagerNodesWrapper::setRelayNodesBounds() {
    int nodeHeight = 50;

    for (size_t i = 0; i < relayNodes.size(); ++i) {
        relayNodes[i]->setBounds(8, 10 + ((int)i * nodeHeight), 272,
                                 nodeHeight - 10);
    }
}

void track::RelayManagerNodesWrapper::mouseDown(const juce::MouseEvent &event) {
    if (event.mods.isRightButtonDown()) {
        juce::PopupMenu addRelayParamMenu;

        addRelayParamMenu.addItem("Add relayed parameter", [this] {
            // TODO: this
            DBG("add relayed param clicked");

            track::RelayManagerComponent *rmc = (RelayManagerComponent *)
                findParentComponentOfClass<RelayManagerComponent>();
            jassert(rmc != nullptr);

            std::unique_ptr<track::subplugin> *plugin = rmc->getPlugin();
            jassert(plugin != nullptr);

            plugin->get()->relayParams.emplace_back();

            createRelayNodes();
        });

        addRelayParamMenu.setLookAndFeel(&getLookAndFeel());

        addRelayParamMenu.showMenuAsync(juce::PopupMenu::Options());
    }
}

track::RelayManagerNode::RelayManagerNode() : juce::Component() {
    addAndMakeVisible(relaySelector);
    addAndMakeVisible(hostedPluginParamSelector);

    hostedPluginParamSelector.onChange = [this] {
        DBG(hostedPluginParamSelector.getSelectedId());

        std::unique_ptr<track::subplugin> *plugin = rmc->getPlugin();
        jassert(plugin != nullptr);

        plugin->get()
            ->relayParams[(size_t)this->paramVectorIndex]
            .pluginParamIndex = hostedPluginParamSelector.getSelectedId() - 1;
    };

    relaySelector.onChange = [this] {
        std::unique_ptr<track::subplugin> *plugin = rmc->getPlugin();
        jassert(plugin != nullptr);

        plugin->get()
            ->relayParams[(size_t)this->paramVectorIndex]
            .outputParamID = relaySelector.getSelectedId();
    };
}
track::RelayManagerNode::~RelayManagerNode() {}

void track::RelayManagerNode::createMenuEntries() {
    DBG("createMenuEntries() called");

    for (int i = 0; i < 128; ++i) {
        relaySelector.addItem("param_" + juce::String(i), i + 1);
    }

    std::unique_ptr<track::subplugin> *plugin = rmc->getPlugin();
    jassert(plugin != nullptr);

    std::unique_ptr<juce::AudioPluginInstance> *pluginInstance =
        &plugin->get()->plugin;
    jassert(pluginInstance != nullptr);

    params = pluginInstance->get()->getParameters();

    for (int i = 0; i < params.size(); ++i) {
        DBG(params[i]->getName(64) << "/" << juce::String(i));
        hostedPluginParamSelector.addItem(params[i]->getName(64), 1 + i);
    }

    // set selected
    int hostedSelectedID = plugin->get()
                               ->relayParams[(size_t)this->paramVectorIndex]
                               .pluginParamIndex;

    if (hostedSelectedID != -1)
        hostedPluginParamSelector.setSelectedId(hostedSelectedID);

    int relayedID = plugin->get()
                        ->relayParams[(size_t)this->paramVectorIndex]
                        .outputParamID;
    if (relayedID != -1)
        relaySelector.setSelectedId(relayedID);
}

void track::RelayManagerNode::paint(juce::Graphics &g) {
    // TODO: this
    g.fillAll(juce::Colours::green);
}

void track::RelayManagerNode::resized() {
    this->hostedPluginParamSelector.setBounds(110, 0, 100, 20);
    this->relaySelector.setBounds(0, 0, 110, 20);
}
