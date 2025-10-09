#include "automation_relay.h"
#include "defs.h"
#include "subwindow.h"
#include "track.h"

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
    Subwindow::resized();

    rmViewport.setBounds(0, UI_SUBWINDOW_TITLEBAR_HEIGHT, getWidth(),
                         getHeight() - UI_SUBWINDOW_TITLEBAR_HEIGHT);
    rmNodesWrapper.setBounds(0, 0, getWidth() - 8,
                             (getPlugin()->get()->relayParams.size() + 1) * 64);
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

void track::RelayManagerNodesWrapper::removeRelayNode(int index) {
    // relayNodes.erase(relayNodes.begin() + index);

    RelayManagerComponent *rmc = (RelayManagerComponent *)
        findParentComponentOfClass<RelayManagerComponent>();
    rmc->getPlugin()->get()->relayParams.erase(
        rmc->getPlugin()->get()->relayParams.begin() + index);

    relayNodes.clear();
    createRelayNodes();
}

void track::RelayManagerNodesWrapper::setRelayNodesBounds() {
    RelayManagerComponent *rmc =
        findParentComponentOfClass<RelayManagerComponent>();

    float scrollRatio = (float)rmc->rmViewport.getViewPositionY() /
                        (getHeight() - rmc->rmViewport.getHeight());

    DBG("scrollRatio is " << scrollRatio);

    rmc->resized();

    int nodeHeight = 64 - 1;

    for (size_t i = 0; i < relayNodes.size(); ++i) {
        relayNodes[i]->setBounds(8, 10 + ((int)i * nodeHeight), 270 - 6,
                                 nodeHeight - 10);
    }

    rmc->rmViewport.setViewPosition(
        0, scrollRatio * (getHeight() - rmc->rmViewport.getHeight()));
}

void track::RelayManagerNodesWrapper::mouseDown(const juce::MouseEvent &event) {
    if (event.mods.isRightButtonDown()) {
        juce::PopupMenu addRelayParamMenu;

        addRelayParamMenu.addItem("Add relayed parameter", [this] {
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
            .pluginParamIndex = hostedPluginParamSelector.getSelectedId();
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

void track::RelayManagerNode::mouseDown(const juce::MouseEvent &event) {
    if (event.mods.isRightButtonDown()) {
        juce::PopupMenu menu;

        menu.addItem("Remove this relay parameter",
                     [this] { this->removeThisRelayParam(); });

        menu.setLookAndFeel(&getLookAndFeel());

        menu.showMenuAsync(juce::PopupMenu::Options());
    }
}

void track::RelayManagerNode::removeThisRelayParam() {
    RelayManagerNodesWrapper *nodesWrapper = (RelayManagerNodesWrapper *)
        findParentComponentOfClass<RelayManagerNodesWrapper>();
    nodesWrapper->removeRelayNode(this->paramVectorIndex);
}

void track::RelayManagerNode::paint(juce::Graphics &g) {
    g.setColour(juce::Colour(0xFF'585858));
    g.drawRect(getLocalBounds());

    g.setColour(juce::Colours::grey);
    g.setFont(getInterBoldItalic());
    g.drawText("IN", 4, 5, 30 - 2, 20, juce::Justification::right);
    g.drawText("OUT", 4, 22 + 5, 30 - 2, 20, juce::Justification::right);
}

void track::RelayManagerNode::resized() {
    this->relaySelector.setBounds(10 + 30, 5, 220, 20);
    this->hostedPluginParamSelector.setBounds(10 + 30, 22 + 7, 220, 20);
}
