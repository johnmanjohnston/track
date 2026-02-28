#include "automation_relay.h"
#include "defs.h"
#include "plugin_chain.h"
#include "subwindow.h"
#include "track.h"
#include "utility.h"

track::RelayManagerComponent::RelayManagerComponent() : track::Subwindow() {
    rmViewport.setViewedComponent(&rmNodesWrapper);
    rmViewport.setScrollBarsShown(true, false, true, false);
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

    return utility::getNodeFromRoute(route, processor);
}

std::unique_ptr<track::subplugin> *track::RelayManagerComponent::getPlugin() {
    return &getCorrespondingTrack()->plugins[(size_t)this->pluginIndex];
}

void track::RelayManagerComponent::paint(juce::Graphics &g) {
    Subwindow::paint(g);

    g.setFont(getTitleBarFont());

    g.setColour(juce::Colour(0xFF'A7A7A7));
    g.drawText(pluginName, getTitleBarBounds().withLeft(14),
               juce::Justification::left);

    int leftMargin = juce::GlyphArrangement::getStringWidthInt(
                         g.getCurrentFont(), this->pluginName) +
                     10 + 10;
    g.setColour(juce::Colours::grey);
    g.drawText(juce::String(pluginIndex) + "/" + this->trackName,
               getTitleBarBounds().withLeft(leftMargin).withTop(2),
               juce::Justification::left);
}

void track::RelayManagerComponent::resized() {
    Subwindow::resized();

    int wrapperHeight = (getPlugin()->get()->relayParams.size() + 1) * 64;

    rmViewport.setBounds(UI_SUBWINDOW_SHADOW_SPREAD,
                         UI_SUBWINDOW_TITLEBAR_HEIGHT +
                             UI_SUBWINDOW_SHADOW_SPREAD + 1,
                         getWidth() - (UI_SUBWINDOW_SHADOW_SPREAD)-4 - 2,
                         getHeight() - UI_SUBWINDOW_TITLEBAR_HEIGHT -
                             UI_SUBWINDOW_SHADOW_SPREAD - 4 - 6);
    rmNodesWrapper.setBounds(0, 0, rmViewport.getWidth() - 1,
                             juce::jmax(wrapperHeight, rmViewport.getHeight()));
}

void track::RelayManagerComponent::updateTrackInformation() {
    this->trackName = getCorrespondingTrack()->trackName;
    this->pluginName = getPlugin()->get()->plugin->getName();

    repaint();
}

void track::RelayManagerComponent::removeRelayParam(size_t index) {
    float scroll = rmViewport.getViewPositionY();

    std::unique_ptr<track::subplugin> *plugin = getPlugin();

    // prepare data for action
    pluginClipboardData oldData;
    oldData.bypassed = plugin->get()->bypassed;
    oldData.dryWetMix = plugin->get()->dryWetMix;
    oldData.relayParams = plugin->get()->relayParams;

    pluginClipboardData newData = oldData;
    newData.relayParams.erase(newData.relayParams.begin() + (long)index);

    ActionChangeTrivialPluginData *action = new ActionChangeTrivialPluginData(
        oldData, newData, route, pluginIndex, processor);
    processor->undoManager.beginNewTransaction(
        "action change trivial plugin data (relay param change)");
    processor->undoManager.perform(action);

    rmViewport.setViewPosition(0, scroll);
}

void track::RelayManagerNodesWrapper::createRelayNodes() {
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

    float scroll = rmc->rmViewport.getViewPositionX();

    rmc->resized();

    int nodeHeight = 64 - 1;

    for (size_t i = 0; i < relayNodes.size(); ++i) {
        relayNodes[i]->setBounds(8, 10 + ((int)i * nodeHeight), 270 - 6,
                                 nodeHeight - 10);
    }

    rmc->rmViewport.setViewPosition(0, scroll);
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

            // prepare data for action
            pluginClipboardData oldData;
            oldData.bypassed = plugin->get()->bypassed;
            oldData.dryWetMix = plugin->get()->dryWetMix;
            oldData.relayParams = plugin->get()->relayParams;

            pluginClipboardData newData = oldData;
            oldData.relayParams.pop_back();

            ActionChangeTrivialPluginData *action =
                new ActionChangeTrivialPluginData(oldData, newData, rmc->route,
                                                  rmc->pluginIndex,
                                                  rmc->processor);
            rmc->processor->undoManager.beginNewTransaction(
                "action change trivial plugin data (relay param change)");
            rmc->processor->undoManager.perform(action);

            float scroll = rmc->rmViewport.getViewHeight();
            DBG("scroll = " << scroll);
            // set scroll to very end
            rmc->rmViewport.setViewPosition(0, scroll);
        });

        addRelayParamMenu.setLookAndFeel(&getLookAndFeel());

        addRelayParamMenu.showMenuAsync(juce::PopupMenu::Options());
    }
}

track::RelayManagerNode::RelayManagerNode()
    : SubwindowChildFocusGrabber(), juce::ComboBox::Listener() {
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

    ActionChangeTrivialPluginData *action = new ActionChangeTrivialPluginData(
        oldData, newData, rmc->route, rmc->pluginIndex, rmc->processor);
    rmc->processor->undoManager.beginNewTransaction(
        "action change trivial plugin data (relay param change)");
    rmc->processor->undoManager.perform(action);
}

void track::RelayManagerNode::mouseDown(const juce::MouseEvent &event) {
    SubwindowChildFocusGrabber::mouseDown(event);

    if (event.mods.isRightButtonDown()) {
        juce::PopupMenu menu;

        menu.addItem("Remove this relay parameter",
                     [this] { this->removeThisRelayParam(); });

        menu.setLookAndFeel(&getLookAndFeel());

        menu.showMenuAsync(juce::PopupMenu::Options());
    }
}

void track::RelayManagerNode::removeThisRelayParam() {
    rmc->removeRelayParam((size_t)paramVectorIndex);
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
    rpiViewport.setScrollBarsShown(true, false, true, false);
    rpiViewport.setViewedComponent(&rpiComponent);
};
track::RelayParamInspector::~RelayParamInspector(){};

track::RelayParamInspectorComponent::RelayParamInspectorComponent()
    : SubwindowChildFocusGrabber(){};
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

        paramSliderAttachments.emplace_back(new juce::SliderParameterAttachment(
            *rangedParam, *slider, nullptr));

        slider->setRange(0, 100);

        slider->setNumDecimalPlacesToDisplay(3);
        slider->setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxLeft,
                                false, 52 + 12, 16);

        addAndMakeVisible(*slider);

        slider->onDragStart = [this] { bringOverOtherSubwindows(); };
    }
}

void track::RelayParamInspectorComponent::paint(juce::Graphics &g) {
    // needs to align with resized()

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
        paramSliders[i]->setBounds(80, 4 + ((int)i * h), w, h);
    }
}

void track::RelayParamInspector::paint(juce::Graphics &g) {
    Subwindow::paint(g);

    g.setFont(getTitleBarFont());
    g.setColour(juce::Colour(0xFF'A7A7A7));
    g.drawText("Relay Params Inspector", getTitleBarBounds().withLeft(8 + 4),
               juce::Justification::left);
}

void track::RelayParamInspector::resized() {
    Subwindow::resized();

    rpiViewport.setBounds(UI_SUBWINDOW_SHADOW_SPREAD + 4,
                          UI_SUBWINDOW_TITLEBAR_HEIGHT +
                              UI_SUBWINDOW_SHADOW_SPREAD + 1,
                          getWidth() - (UI_SUBWINDOW_SHADOW_SPREAD * 2) - 4 - 2,
                          getHeight() - UI_SUBWINDOW_TITLEBAR_HEIGHT -
                              (UI_SUBWINDOW_SHADOW_SPREAD * 2) - 5);
    rpiComponent.setBounds(0, 0, 2000, 3100);
}

void track::RelayParamInspector::initSliders() { rpiComponent.initSliders(); }
