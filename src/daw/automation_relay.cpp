#include "automation_relay.h"
#include "defs.h"
#include "juce_events/juce_events.h"
#include "plugin_chain.h"
#include "subwindow.h"
#include "track.h"

track::RelayManagerComponent::RelayManagerComponent() : track::Subwindow() {
    rmViewport.setViewedComponent(&rmNodesWrapper);
    addAndMakeVisible(this->rmViewport);

    closeBtn.onClose = [this] {
        AudioPluginAudioProcessorEditor *editor =
            findParentComponentOfClass<AudioPluginAudioProcessorEditor>();

        for (size_t i = 0; i < editor->relayManagerCompnoents.size(); ++i) {
            if (editor->relayManagerCompnoents[i].get() == this) {
                editor->relayManagerCompnoents.erase(
                    editor->relayManagerCompnoents.begin() + (long)i);
                break;
            }
        }
    };
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

track::RelayManagerNode::RelayManagerNode()
    : juce::Component(), juce::ComboBox::Listener() {
    addAndMakeVisible(relaySelector);
    addAndMakeVisible(hostedPluginParamSelector);

    // add listener for the sole purpose of handling un/redoable actions; the
    // actual logic where you change plugins' relay param values happens in
    // lambda functions below; this works because the LISTENERS get called
    // BEFORE THE LAMBDAS
    hostedPluginParamSelector.addListener(this);
    relaySelector.addListener(this);

    hostedPluginParamSelector.onChange = [this] {
        DBG("this is the lambda!");
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
        // DBG(params[i]->getName(64) << "/" << juce::String(i));
        hostedPluginParamSelector.addItem(params[i]->getName(64), 1 + i);
    }

    // set selected
    int hostedSelectedID = plugin->get()
                               ->relayParams[(size_t)this->paramVectorIndex]
                               .pluginParamIndex;

    if (hostedSelectedID != -1)
        hostedPluginParamSelector.setSelectedId(
            hostedSelectedID, juce::NotificationType::dontSendNotification);

    int relayedID = plugin->get()
                        ->relayParams[(size_t)this->paramVectorIndex]
                        .outputParamID;
    if (relayedID != -1)
        relaySelector.setSelectedId(
            relayedID, juce::NotificationType::dontSendNotification);
}

void track::RelayManagerNode::comboBoxChanged(juce::ComboBox *box) {
    DBG("this is the listener");
    DBG("comboBoxChanged() called");

    jassert(box == &this->hostedPluginParamSelector ||
            box == &this->relaySelector);

    std::unique_ptr<track::subplugin> *plugin = rmc->getPlugin();

    // prepare data for action
    pluginClipboardData oldData;
    oldData.bypassed = plugin->get()->bypassed;
    oldData.dryWetMix = plugin->get()->dryWetMix;
    oldData.relayParams = plugin->get()->relayParams;

    pluginClipboardData newData = oldData;

    if (box == &this->hostedPluginParamSelector) {
        newData.relayParams[(size_t)this->paramVectorIndex].pluginParamIndex =
            hostedPluginParamSelector.getSelectedId();
    } else {
        newData.relayParams[(size_t)this->paramVectorIndex].outputParamID =
            relaySelector.getSelectedId();
    }

    AudioPluginAudioProcessorEditor *editor =
        rmc->findParentComponentOfClass<AudioPluginAudioProcessorEditor>();

    ActionChangeTrivialPluginData *action = new ActionChangeTrivialPluginData(
        oldData, newData, rmc->route, rmc->pluginIndex, rmc->processor, editor);
    rmc->processor->undoManager.beginNewTransaction(
        "action change trivial plugin data (relay param change)");
    rmc->processor->undoManager.perform(action);
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
    std::unique_ptr<track::subplugin> *plugin = rmc->getPlugin();

    // prepare data for action
    pluginClipboardData oldData;
    oldData.bypassed = plugin->get()->bypassed;
    oldData.dryWetMix = plugin->get()->dryWetMix;
    oldData.relayParams = plugin->get()->relayParams;

    pluginClipboardData newData = oldData;
    newData.relayParams.erase(newData.relayParams.begin() + paramVectorIndex);

    AudioPluginAudioProcessorEditor *editor =
        rmc->findParentComponentOfClass<AudioPluginAudioProcessorEditor>();

    ActionChangeTrivialPluginData *action = new ActionChangeTrivialPluginData(
        oldData, newData, rmc->route, rmc->pluginIndex, rmc->processor, editor);
    rmc->processor->undoManager.beginNewTransaction(
        "action change trivial plugin data (relay param change)");
    rmc->processor->undoManager.perform(action);
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

// WARN: this shitshow gets REAL messy with the confusing class names

track::RelayParamInspector::RelayParamInspector() : track::Subwindow() {
    addAndMakeVisible(rpiViewport);

    rpiViewport.setViewedComponent(&rpiComponent);
};
track::RelayParamInspector::~RelayParamInspector(){};

track::RelayParamInspectorComponent::RelayParamInspectorComponent()
    : juce::Component(){};
track::RelayParamInspectorComponent::~RelayParamInspectorComponent() {}

void track::RelayParamInspectorComponent::initSliders() {
    jassert(processor != nullptr);

    int params = 128;
    for (int i = 0; i < params; ++i) {
        std::unique_ptr<juce::Slider> &slider =
            this->paramSliders.emplace_back(new juce::Slider());

        juce::Array<juce::AudioProcessorParameter *> processorParams =
            processor->getParameters();

        juce::AudioProcessorParameter *rawParam =
            processorParams[processor->automatableParametersIndexOffset + i];

        juce::RangedAudioParameter *rangedParam =
            (juce::RangedAudioParameter *)rawParam;

        auto &x = paramSliderAttachments.emplace_back(
            new juce::SliderParameterAttachment(*rangedParam, *slider,
                                                nullptr));

        slider->setRange(0, 100);

        slider->setNumDecimalPlacesToDisplay(3);
        slider->setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxLeft,
                                false, 52 + 12, 16);

        addAndMakeVisible(*slider);
    }
}

void track::RelayParamInspectorComponent::paint(juce::Graphics &g) {
    // needs to align with resized()

    int w = 200;
    int h = 24;

    g.setFont(getInterSemiBold().withHeight(18.f));
    g.setColour(juce::Colours::white);

    for (size_t i = 0; i < paramSliders.size(); ++i) {
        g.drawText("param_" + juce::String(i), 8, 4 + ((int)i * h), 90, h,
                   juce::Justification::left, false);
    }
}

void track::RelayParamInspectorComponent::resized() {
    int h = 24;
    int w = 200;
    for (size_t i = 0; i < paramSliders.size(); ++i) {
        paramSliders[i]->setBounds(100, 4 + ((int)i * h), w, h);
    }
}

void track::RelayParamInspector::paint(juce::Graphics &g) {
    Subwindow::paint(g);

    g.setFont(getTitleBarFont());
    g.setColour(juce::Colour(0xFF'A7A7A7));
    g.drawText("Relay Params Inspector",
               getTitleBarBounds().withLeft(8).withTop(2),
               juce::Justification::left);
}

void track::RelayParamInspector::resized() {
    Subwindow::resized();

    rpiViewport.setBounds(0, UI_SUBWINDOW_TITLEBAR_HEIGHT, getWidth(),
                          getHeight() - UI_SUBWINDOW_TITLEBAR_HEIGHT);
    rpiComponent.setBounds(0, 0, 2000, 3100);

    /*
    int params = 128;

    for (int i = 0; i < params; ++i) {
        paramSliders[i].setBounds(2, i * 20, 100, 20);
    }*/
}

void track::RelayParamInspector::initSliders() {
    // int params = 128;

    /*
    // rangedaudioparameter
    juce::Array<juce::AudioProcessorParameter*> processorParams =
    processor->getParameters();

    for (int i = 0; i < params; ++i) {
    }

    //juce::RangedAudioParameter* = processor->getParam
    */

    rpiComponent.initSliders();
}
