#include "plugin_chain.h"
#include "clipboard.h"
#include "defs.h"
#include "subwindow.h"
#include "track.h"
#include "utility.h"
#include <cstddef>
#include <cstdint>
#include <fcntl.h>

track::ActionAddPlugin::ActionAddPlugin(std::vector<int> route,
                                        juce::String identifier,
                                        void *processor)
    : juce::UndoableAction() {
    this->nodeRoute = route;
    this->pluginIdentifier = identifier;
    this->p = processor;
}
track::ActionAddPlugin::~ActionAddPlugin() {}

bool track::ActionAddPlugin::perform() {
    audioNode *node = utility::getNodeFromRoute(nodeRoute, p);
    validPlugin = node->addPlugin(pluginIdentifier);

    updateGUI();

    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->requireSaving();

    return validPlugin;
}

bool track::ActionAddPlugin::undo() {
    if (!validPlugin)
        return false;

    audioNode *node = utility::getNodeFromRoute(nodeRoute, p);

    // close editor, then remove plugin
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->dispatchGUIInstruction(
        UI_INSTRUCTION_CLOSE_PEW, (void *)(uintptr_t)(node->plugins.size() - 1),
        nodeRoute);
    processor->dispatchGUIInstruction(
        UI_INSTRUCTION_CLOSE_ALL_RMC_WITH_ROUTE_AND_INDEX,
        (void *)(uintptr_t)(node->plugins.size() - 1), nodeRoute);

    node->removePlugin(node->plugins.size() - 1);

    updateGUI();
    processor->requireSaving();

    return true;
}

void track::ActionAddPlugin::updateGUI() {
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->dispatchGUIInstruction(UI_INSTRUCTION_RECREATE_ALL_PNCS, nullptr,
                                      nodeRoute);
}

track::ActionRemovePlugin::ActionRemovePlugin(track::pluginClipboardData data,
                                              std::vector<int> route, int index,
                                              void *processor)
    : juce::UndoableAction() {
    this->subpluginData = data;
    this->nodeRoute = route;
    this->p = processor;
    this->pluginIndex = index;
};
track::ActionRemovePlugin::~ActionRemovePlugin(){};

bool track::ActionRemovePlugin::perform() {
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->dispatchGUIInstruction(UI_INSTRUCTION_CLOSE_OPENED_EDITORS,
                                      nullptr, nodeRoute);
    processor->dispatchGUIInstruction(
        UI_INSTRUCTION_CLOSE_OPENED_RELAY_PARAM_WINDOWS, nullptr, nodeRoute);

    audioNode *node = utility::getNodeFromRoute(nodeRoute, p);
    node->removePlugin(pluginIndex);

    updateGUI();

    processor->dispatchGUIInstruction(UI_INSTRUCTION_OPEN_CLOSED_EDITORS,
                                      nullptr, nodeRoute);
    processor->dispatchGUIInstruction(
        UI_INSTRUCTION_OPEN_CLOSED_RELAY_PARAM_WINDOWS, nullptr, nodeRoute);
    processor->requireSaving();

    return true;
}

bool track::ActionRemovePlugin::undo() {
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->dispatchGUIInstruction(UI_INSTRUCTION_CLOSE_OPENED_EDITORS,
                                      nullptr, nodeRoute);
    processor->dispatchGUIInstruction(
        UI_INSTRUCTION_CLOSE_OPENED_RELAY_PARAM_WINDOWS, nullptr, nodeRoute);

    audioNode *node = utility::getNodeFromRoute(nodeRoute, p);

    // readd plugin
    node->addPlugin(this->subpluginData.identifier.upToLastOccurrenceOf(
        ".vst3", true, true));
    utility::reorderPlugin(node->plugins.size() - 1, pluginIndex, node);

    // copy subplugin data back
    std::unique_ptr<track::subplugin> &plugin =
        node->plugins[(size_t)pluginIndex];

    plugin->bypassed = subpluginData.bypassed;
    plugin->dryWetMix = subpluginData.dryWetMix;
    plugin->relayParams = subpluginData.relayParams;

    juce::MemoryBlock pluginStateData;
    bool dataRetrieved =
        pluginStateData.fromBase64Encoding((juce::String)subpluginData.data);

    jassert(dataRetrieved);

    plugin->plugin->setStateInformation(pluginStateData.getData(),
                                        pluginStateData.getSize());

    // plugin is readded, how handle UI stuff
    updateGUI();

    processor->dispatchGUIInstruction(UI_INSTRUCTION_OPEN_CLOSED_EDITORS,
                                      nullptr, nodeRoute);
    processor->dispatchGUIInstruction(
        UI_INSTRUCTION_OPEN_CLOSED_RELAY_PARAM_WINDOWS, nullptr, nodeRoute);
    processor->requireSaving();

    return true;
}

void track::ActionRemovePlugin::updateGUI() {
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->dispatchGUIInstruction(UI_INSTRUCTION_RECREATE_ALL_PNCS, nullptr,
                                      nodeRoute);
}

track::ActionReorderPlugin::ActionReorderPlugin(std::vector<int> nodeRoute,
                                                int sourceIndex,
                                                int destinationIndex,
                                                void *processor)
    : juce::UndoableAction() {
    this->route = nodeRoute;
    this->srcIndex = sourceIndex;
    this->destIndex = destinationIndex;
    this->p = processor;
}
track::ActionReorderPlugin::~ActionReorderPlugin(){};

bool track::ActionReorderPlugin::perform() {
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->dispatchGUIInstruction(UI_INSTRUCTION_CLOSE_OPENED_EDITORS,
                                      nullptr, route);
    processor->dispatchGUIInstruction(
        UI_INSTRUCTION_CLOSE_OPENED_RELAY_PARAM_WINDOWS, nullptr, route);

    audioNode *node = utility::getNodeFromRoute(route, p);
    utility::reorderPlugin(srcIndex, destIndex, node);

    updateGUI();

    processor->dispatchGUIInstruction(UI_INSTRUCTION_OPEN_CLOSED_EDITORS,
                                      nullptr, route);
    processor->dispatchGUIInstruction(
        UI_INSTRUCTION_OPEN_CLOSED_RELAY_PARAM_WINDOWS, nullptr, route);

    processor->requireSaving();

    return true;
}

bool track::ActionReorderPlugin::undo() {
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->dispatchGUIInstruction(UI_INSTRUCTION_CLOSE_OPENED_EDITORS,
                                      nullptr, route);
    processor->dispatchGUIInstruction(
        UI_INSTRUCTION_CLOSE_OPENED_RELAY_PARAM_WINDOWS, nullptr, route);

    audioNode *node = utility::getNodeFromRoute(route, p);
    utility::reorderPlugin(srcIndex, destIndex, node);

    updateGUI();

    processor->dispatchGUIInstruction(UI_INSTRUCTION_OPEN_CLOSED_EDITORS,
                                      nullptr, route);
    processor->dispatchGUIInstruction(
        UI_INSTRUCTION_OPEN_CLOSED_RELAY_PARAM_WINDOWS, nullptr, route);

    processor->requireSaving();

    return true;
}

void track::ActionReorderPlugin::updateGUI() {
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->dispatchGUIInstruction(UI_INSTRUCTION_RECREATE_ALL_PNCS, nullptr,
                                      route);
}

track::ActionPastePlugin::ActionPastePlugin(track::pluginClipboardData data,
                                            std::vector<int> route,
                                            void *processor)
    : juce::UndoableAction() {
    this->subpluginData = data;
    this->nodeRoute = route;
    this->p = processor;
}
track::ActionPastePlugin::~ActionPastePlugin() {}

bool track::ActionPastePlugin::perform() {
    audioNode *node = utility::getNodeFromRoute(nodeRoute, p);
    juce::String cleanedIdentifier =
        subpluginData.identifier.upToLastOccurrenceOf(".vst3", true, true);
    node->addPlugin(cleanedIdentifier);
    juce::MemoryBlock pluginData;

    std::unique_ptr<track::subplugin> &plugin = node->plugins.back();

    plugin->bypassed = subpluginData.bypassed;
    plugin->dryWetMix = subpluginData.dryWetMix;
    plugin->relayParams = subpluginData.relayParams;

    bool dataRetrieved =
        pluginData.fromBase64Encoding((juce::String)subpluginData.data);

    DBG("dataRetrieved = " << (dataRetrieved ? "true" : "false"));

    plugin->plugin->setStateInformation(pluginData.getData(),
                                        pluginData.getSize());

    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->requireSaving();

    updateGUI();

    return true;
}

bool track::ActionPastePlugin::undo() {
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->dispatchGUIInstruction(UI_INSTRUCTION_CLOSE_OPENED_EDITORS,
                                      nullptr, nodeRoute);

    audioNode *node = utility::getNodeFromRoute(nodeRoute, p);
    processor->dispatchGUIInstruction(
        UI_INSTRUCTION_CLOSE_ALL_RMC_WITH_ROUTE_AND_INDEX,
        (void *)((uintptr_t)node->plugins.size() - 1), nodeRoute);

    node->removePlugin(node->plugins.size() - 1);

    updateGUI();

    processor->dispatchGUIInstruction(UI_INSTRUCTION_OPEN_CLOSED_EDITORS,
                                      nullptr, nodeRoute);
    processor->requireSaving();

    return true;
}

void track::ActionPastePlugin::updateGUI() {
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->dispatchGUIInstruction(UI_INSTRUCTION_RECREATE_ALL_PNCS, nullptr,
                                      nodeRoute);
}

// it's that time of the year again where track is the only
// thing keeping me going :thumbsup:
track::ActionPastePluginChain::ActionPastePluginChain(
    pluginChainClipboardData data, std::vector<int> route, void *processor) {
    this->chainData = data;
    this->nodeRoute = route;
    this->p = processor;
}
track::ActionPastePluginChain::~ActionPastePluginChain() {}

bool track::ActionPastePluginChain::perform() {
    audioNode *node = utility::getNodeFromRoute(nodeRoute, p);

    for (pluginClipboardData &pluginClipboardData : chainData.plugins) {
        juce::String cleanedIdentifier =
            pluginClipboardData.identifier.upToLastOccurrenceOf(".vst3", true,
                                                                true);
        node->addPlugin(cleanedIdentifier);

        juce::MemoryBlock pluginData;

        std::unique_ptr<track::subplugin> &plugin = node->plugins.back();

        plugin->bypassed = pluginClipboardData.bypassed;
        plugin->dryWetMix = pluginClipboardData.dryWetMix;
        plugin->relayParams = pluginClipboardData.relayParams;

        bool dataRetrieved = pluginData.fromBase64Encoding(
            (juce::String)pluginClipboardData.data);

        DBG("dataRetrieved = " << (dataRetrieved ? "true" : "false"));

        plugin->plugin->setStateInformation(pluginData.getData(),
                                            pluginData.getSize());
    }

    updateGUI();

    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->requireSaving();

    return true;
}

bool track::ActionPastePluginChain::undo() {
    DBG("ActionPastePluginChain::undo()");

    audioNode *node = utility::getNodeFromRoute(nodeRoute, p);
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;

    for (size_t i = 0; i < chainData.plugins.size(); ++i) {
        DBG("node->plugins.size() - 1 = " << node->plugins.size() - 1);

        processor->dispatchGUIInstruction(
            UI_INSTRUCTION_CLOSE_PEW,
            (void *)((uintptr_t)node->plugins.size() - 1), nodeRoute);

        processor->dispatchGUIInstruction(
            UI_INSTRUCTION_CLOSE_ALL_RMC_WITH_ROUTE_AND_INDEX,
            (void *)((uintptr_t)node->plugins.size() - 1), nodeRoute);
    }

    for (size_t i = 0; i < chainData.plugins.size(); ++i) {
        node->removePlugin(node->plugins.size() - 1);
    }

    updateGUI();
    processor->requireSaving();

    return true;
}

void track::ActionPastePluginChain::updateGUI() {
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->dispatchGUIInstruction(UI_INSTRUCTION_RECREATE_ALL_PNCS, nullptr,
                                      nodeRoute);
}

track::ActionChangeTrivialPluginData::ActionChangeTrivialPluginData(
    pluginClipboardData oldData, pluginClipboardData newData,
    std::vector<int> nodeRoute, int pluginIndex, void *processor)
    : juce::UndoableAction() {
    this->oldPluginData = oldData;
    this->newPluginData = newData;
    this->route = nodeRoute;
    this->index = pluginIndex;
    this->p = processor;
}
track::ActionChangeTrivialPluginData::~ActionChangeTrivialPluginData() {}

bool track::ActionChangeTrivialPluginData::perform() {
    audioNode *node = utility::getNodeFromRoute(route, p);
    subplugin *plugin = node->plugins[(size_t)index].get();

    plugin->bypassed = newPluginData.bypassed;
    plugin->dryWetMix = newPluginData.dryWetMix;
    plugin->relayParams = newPluginData.relayParams;

    updateGUI();

    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->requireSaving();

    return true;
}
bool track::ActionChangeTrivialPluginData::undo() {
    audioNode *node = utility::getNodeFromRoute(route, p);
    subplugin *plugin = node->plugins[(size_t)index].get();

    plugin->bypassed = oldPluginData.bypassed;
    plugin->dryWetMix = oldPluginData.dryWetMix;
    plugin->relayParams = oldPluginData.relayParams;

    updateGUI();

    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->requireSaving();

    return true;
}
void track::ActionChangeTrivialPluginData::updateGUI() {
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    processor->dispatchGUIInstruction(UI_INSTRUCTION_RECREATE_ALL_PNCS, nullptr,
                                      route);

    processor->dispatchGUIInstruction(UI_INSTRUCTION_RECREATE_RELAY_NODES,
                                      nullptr, route);
};

track::PluginNodeComponent::PluginNodeComponent() : juce::Component() {
    setWantsKeyboardFocus(true);

    this->openEditorBtn.setButtonText("EDITOR");
    addAndMakeVisible(openEditorBtn);

    this->removePluginBtn.setButtonText("X");
    addAndMakeVisible(removePluginBtn);

    this->bypassBtn.setButtonText("BYPASS");
    addAndMakeVisible(bypassBtn);

    this->automationButton.setButtonText("AUTOMATE");
    addAndMakeVisible(automationButton);

    this->dryWetSlider.setRange(0.f, 1.f);
    addAndMakeVisible(dryWetSlider);

    dryWetSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox,
                                 true, 0, 0);
    dryWetSlider.setSliderStyle(
        juce::Slider::SliderStyle::RotaryHorizontalDrag);
    dryWetSlider.onValueChange = [this] {
        getPlugin()->get()->dryWetMix = dryWetSlider.getValue();
    };

    // for un/redoing dry/wet mix changes
    dryWetSlider.onDragStart = [this] {
        this->dryWetMixAtDragStart = dryWetSlider.getValue();
    };
    dryWetSlider.onDragEnd = [this] {
        // prepare all this shit man idfk
        pluginClipboardData pluginData;
        pluginData.bypassed = getPlugin()->get()->bypassed;
        pluginData.dryWetMix = getPlugin()->get()->dryWetMix;
        pluginData.relayParams = getPlugin()->get()->relayParams;

        pluginClipboardData oldPluginData = pluginData;
        oldPluginData.dryWetMix = dryWetMixAtDragStart;

        // finally, execute action
        PluginChainComponent *pcc =
            findParentComponentOfClass<PluginChainComponent>();

        ActionChangeTrivialPluginData *action =
            new ActionChangeTrivialPluginData(oldPluginData, pluginData,
                                              pcc->route, pluginIndex,
                                              pcc->processor);
        pcc->processor->undoManager.beginNewTransaction(
            "action change trivial plugin data");
        pcc->processor->undoManager.perform(action);
    };

    openEditorBtn.onClick = [this] { openThisPluginsEditor(); };

    removePluginBtn.onClick = [this] { removeThisPlugin(); };

    bypassBtn.onClick = [this] { toggleBypass(); };

    automationButton.onClick = [this] { openThisPluginsRelayMenu(); };
}
track::PluginNodeComponent::~PluginNodeComponent() {}
track::PluginNodesWrapper::PluginNodesWrapper() : juce::Component() {
    addAndMakeVisible(insertIndicator);
    setWantsKeyboardFocus(true);
}
track::PluginNodesWrapper::~PluginNodesWrapper() {}
track::PluginNodesViewport::PluginNodesViewport() : juce::Viewport() {}
track::PluginNodesViewport::~PluginNodesViewport() {}

void track::PluginNodeComponent::mouseDown(const juce::MouseEvent &event) {
    if (event.mods.isRightButtonDown()) {
        if (event.mods.isRightButtonDown()) {
            PluginChainComponent *pcc =
                findParentComponentOfClass<PluginChainComponent>();

            AudioPluginAudioProcessorEditor *editor =
                pcc->findParentComponentOfClass<
                    AudioPluginAudioProcessorEditor>();
            jassert(pcc != nullptr);
            jassert(editor != nullptr);

            juce::PopupMenu menu;

            menu.addItem("Copy plugin", [this] { copyPluginToClipboard(); });
            menu.addSeparator();
            menu.addItem(
                "Close editor",
                editor->isPluginEditorWindowOpen(pcc->route, pluginIndex),
                false, [this, pcc, editor] {
                    editor->closePluginEditorWindow(pcc->route, pluginIndex);
                });

            menu.setLookAndFeel(&getLookAndFeel());
            menu.showMenuAsync(juce::PopupMenu::Options());
        }
    } else {
        if (event.getNumberOfClicks() == 2) {
            openThisPluginsEditor();
        }
    }
}

void track::PluginNodeComponent::mouseUp(const juce::MouseEvent &event) {
    if (event.mouseWasDraggedSinceMouseDown() &&
        event.mods.isLeftButtonDown()) {
        PluginChainComponent *pcc = (PluginChainComponent *)
            findParentComponentOfClass<PluginChainComponent>();

        float mouseX = event.getEventRelativeTo(pcc).position.getX() +
                       (UI_PLUGIN_NODE_WIDTH + UI_PLUGIN_NODE_MARGIN) +
                       (UI_PLUGIN_NODE_MARGIN / 2.f);
        int destIndex = (int)(mouseX / (float)(getWidth() + 4)) - 1;

        if (this->pluginIndex != destIndex) {
            pcc->reorderPlugin(this->pluginIndex, destIndex);
        }

        pcc->updateInsertIndicator(-1);
    }
}

void track::PluginNodeComponent::mouseDrag(const juce::MouseEvent &event) {
    if (event.mouseWasDraggedSinceMouseDown() &&
        event.mods.isLeftButtonDown()) {
        PluginChainComponent *pcc = (PluginChainComponent *)
            findParentComponentOfClass<PluginChainComponent>();

        float mouseX = event.getEventRelativeTo(pcc).position.getX() +
                       (UI_PLUGIN_NODE_WIDTH + UI_PLUGIN_NODE_MARGIN) +
                       (UI_PLUGIN_NODE_MARGIN / 2.f);
        int displayNodes =
            (int)(mouseX / (float)(UI_PLUGIN_NODE_WIDTH + 4)) - 1;
        // DBG("displayNodes = " << displayNodes);

        pcc->updateInsertIndicator(displayNodes);
    }
}

void track::PluginNodeComponent::setDryWetSliderValue() {
    this->dryWetSlider.setValue(getPlugin()->get()->dryWetMix);
}

void track::PluginNodeComponent::copyPluginToClipboard() {
    // create clipboard data
    pluginClipboardData *data = new pluginClipboardData;
    std::unique_ptr<juce::AudioPluginInstance> *pluginInstance =
        &getPlugin()->get()->plugin;

    // set trivial data
    data->identifier =
        pluginInstance->get()->getPluginDescription().fileOrIdentifier;
    data->bypassed = getPlugin()->get()->bypassed;
    data->dryWetMix = getPlugin()->get()->dryWetMix;
    data->relayParams = getPlugin()->get()->relayParams;

    // set plugin data
    juce::MemoryBlock pluginData;
    pluginInstance->get()->getStateInformation(pluginData);
    data->data = pluginData.toBase64Encoding();

    clipboard::setData(data, TYPECODE_PLUGIN);

    // visual feedback
    coolColors = true;
    repaint();

    juce::Timer::callAfterDelay(UI_VISUAL_FEEDBACK_FLASH_DURATION_MS, [this] {
        coolColors = false;
        repaint();
    });
}

void track::PluginNodeComponent::removeThisPlugin() {
    PluginChainComponent *pcc =
        findParentComponentOfClass<PluginChainComponent>();
    jassert(pcc != nullptr);

    // close editor before removing plugin otherwise segfault
    AudioPluginAudioProcessorEditor *editor =
        this->findParentComponentOfClass<AudioPluginAudioProcessorEditor>();
    editor->closePluginEditorWindow(pcc->route, pluginIndex);
    editor->closeAllRelayMenusWithRouteAndPluginIndex(pcc->route, pluginIndex);

    float scroll = pcc->nodesViewport.getViewPositionX();

    pcc->removePlugin(this->pluginIndex);
    pcc->resized();

    pcc->nodesViewport.setViewPosition(scroll, 0);
}

void track::PluginNodeComponent::toggleBypass() {
    getPlugin()->get()->bypassed = !getPlugin()->get()->bypassed;

    // prepare all this shit man idfk
    pluginClipboardData pluginData;
    pluginData.bypassed = getPlugin()->get()->bypassed;
    pluginData.dryWetMix = getPlugin()->get()->dryWetMix;
    pluginData.relayParams = getPlugin()->get()->relayParams;

    pluginClipboardData oldPluginData = pluginData;
    oldPluginData.bypassed = !oldPluginData.bypassed;

    // finally, execute action
    PluginChainComponent *pcc =
        findParentComponentOfClass<PluginChainComponent>();

    ActionChangeTrivialPluginData *action = new ActionChangeTrivialPluginData(
        oldPluginData, pluginData, pcc->route, pluginIndex, pcc->processor);
    pcc->processor->undoManager.beginNewTransaction(
        "action change trivial plugin data");
    pcc->processor->undoManager.perform(action);

    repaint();
}

void track::PluginNodeComponent::openThisPluginsEditor() {
    PluginChainComponent *pcc =
        findParentComponentOfClass<PluginChainComponent>();
    AudioPluginAudioProcessorEditor *editor =
        this->findParentComponentOfClass<AudioPluginAudioProcessorEditor>();

    DBG("pluginIndex = " << pluginIndex);

    if (editor->isPluginEditorWindowOpen(pcc->route, pluginIndex))
        editor->closePluginEditorWindow(pcc->route, pluginIndex);

    editor->openPluginEditorWindow(pcc->route, pluginIndex);

    repaint();
}

void track::PluginNodeComponent::openThisPluginsRelayMenu() {
    DBG("automation button clicked");

    PluginChainComponent *pcc =
        findParentComponentOfClass<PluginChainComponent>();
    jassert(pcc != nullptr);
    AudioPluginAudioProcessorEditor *editor =
        this->findParentComponentOfClass<AudioPluginAudioProcessorEditor>();
    editor->openRelayMenu(pcc->route, this->pluginIndex);

    repaint();
}

void track::PluginNodeComponent::paint(juce::Graphics &g) {
    if (getPlugin() && getPlugin()->get()) {
        if (!coolColors)
            g.fillAll(juce::Colour(hasKeyboardFocus(true) ? 0xFF'292929
                                                          : 0xFF'121212));
        else
            g.fillAll(juce::Colour(0xFF'FFFFFF));

        // border
        g.setColour(juce::Colours::lightgrey.withAlpha(.3f));
        g.drawRect(getLocalBounds(), 1);

        g.setColour(juce::Colours::white);

        g.setColour(juce::Colour(0xFF'A7A7A7)
                        .withAlpha(getPluginBypassedStatus() ? .3f : 1.f));

#if JUCE_WINDOWS
        auto pluginDataFont = track::ui::CustomLookAndFeel::getInterSemiBold()
                                  .withExtraKerningFactor(-0.02f);
#else
        auto pluginDataFont = track::ui::CustomLookAndFeel::getInterSemiBold();
        pluginDataFont =
            pluginDataFont.italicised().boldened().withExtraKerningFactor(
                -0.03f);
#endif

        // draw name
        g.setFont(pluginDataFont.withHeight(22.f));
        g.drawText(getPlugin()->get()->plugin->getName(), 10, 8, getWidth(), 20,
                   juce::Justification::left);

        // draw manufacturer name
        g.setColour(juce::Colour(0xFF'595959));
        g.setFont(pluginDataFont.withHeight(16.f));
        g.drawText(
            getPlugin()->get()->plugin->getPluginDescription().manufacturerName,
            10, 30, getWidth(), 20, juce::Justification::left);
    }
}

void track::PluginNodeComponent::resized() {
    this->dryWetSlider.setBounds(211, 20, 40, 40);

    int btnWidth = 76;
    int btnHeight = 21;
    int initialLeftMargin = 7;
    int bottomMargin = 7;
    int buttonSideMargin = 4;

    this->automationButton.setBounds(initialLeftMargin,
                                     getHeight() - btnHeight - bottomMargin,
                                     btnWidth, btnHeight);

    this->openEditorBtn.setBounds(
        initialLeftMargin + (btnWidth + buttonSideMargin),
        getHeight() - btnHeight - bottomMargin, btnWidth, btnHeight);

    this->bypassBtn.setBounds(
        initialLeftMargin + ((btnWidth + buttonSideMargin) * 2),
        getHeight() - btnHeight - bottomMargin, btnWidth, btnHeight);

    int closeBtnSize = 22;
    int closeBtnMargin = 2;
    this->removePluginBtn.setBounds(getWidth() - closeBtnSize - closeBtnMargin,
                                    closeBtnMargin, closeBtnSize, closeBtnSize);
}

void track::PluginNodesWrapper::createPluginNodeComponents() {
    jassert(pcc != nullptr);

    audioNode *node = pcc->getCorrespondingTrack();

    for (size_t i = 0; i < node->plugins.size(); ++i) {
        DBG("creating plugin node for "
            << node->plugins[i].get()->plugin->getName());

        this->pluginNodeComponents.emplace_back(new track::PluginNodeComponent);
        PluginNodeComponent &nc = *this->pluginNodeComponents.back();
        nc.pluginIndex = i;

        addAndMakeVisible(nc);
        nc.setBounds(getBoundsForPluginNodeComponent(i));
        nc.setDryWetSliderValue();
    }

    DBG("pluginNodeComponents.size() = " << this->pluginNodeComponents.size());
    if (pluginNodeComponents.size() > 0) {
        DBG("first plugin node compnoent bounds are "
            << this->rectToStr(pluginNodeComponents[0]->getBounds()));
    }
}

void track::PluginNodesWrapper::mouseDown(const juce::MouseEvent &event) {
    if (event.mods.isRightButtonDown()) {
        jassert(knownPluginList != nullptr);

        if (knownPluginList->getTypes().size() == 0) {
            DBG("No plugins in known plugin list");
        }

        juce::PopupMenu pluginMenu;
        juce::PopupMenu pluginSelector;
        pluginMenu.setLookAndFeel(&getLookAndFeel());

        audioNode *node = pcc->getCorrespondingTrack();

        for (auto pluginDescription : knownPluginList->getTypes()) {
            pluginSelector.addItem(
                pluginDescription.name, [this, pluginDescription, node] {
                    // add plugin
                    DBG("adding " << pluginDescription.name);

                    AudioPluginAudioProcessorEditor *editor =
                        this->findParentComponentOfClass<
                            AudioPluginAudioProcessorEditor>();

                    // add plugin using action
                    ActionAddPlugin *action = new ActionAddPlugin(
                        pcc->route, pluginDescription.fileOrIdentifier,
                        pcc->processor);
                    pcc->processor->undoManager.beginNewTransaction(
                        "action add plugin");
                    pcc->processor->undoManager.perform(action);

                    if (action->validPlugin) {
                        int pluginIndex = node->plugins.size() - 1;

                        editor->openPluginEditorWindow(pcc->route, pluginIndex);

                        pcc->resized();
                        pcc->nodesViewport.setViewPosition(
                            pcc->nodesWrapper.getWidth(), 0);
                    } else {
                        juce::NativeMessageBox::showMessageBoxAsync(
                            juce::MessageBoxIconType::WarningIcon,
                            "Failed to add plugin",
                            "Couldn't add the plugin at \"" +
                                action->pluginIdentifier + "\"");
                    }
                });
        }

        pluginMenu.addSubMenu("Add plugin", pluginSelector);
        pluginMenu.addItem(
            "Paste plugin", clipboard::typecode == TYPECODE_PLUGIN, false,
            [this] {
                if (clipboard::typecode == TYPECODE_PLUGIN) {
                    pluginClipboardData *clipboardData =
                        (pluginClipboardData *)clipboard::retrieveData();

                    ActionPastePlugin *action = new ActionPastePlugin(
                        *clipboardData, pcc->route, pcc->processor);

                    pcc->processor->undoManager.beginNewTransaction(
                        "action paste plugin");
                    pcc->processor->undoManager.perform(action);

                    return;
                }
            });

        pluginMenu.addItem(
            "Paste plugin chain", clipboard::typecode == TYPECODE_PLUGIN_CHAIN,
            false, [this] {
                DBG("paste plugin chain");

                if (clipboard::typecode == TYPECODE_PLUGIN_CHAIN) {
                    pluginChainClipboardData *data =
                        (pluginChainClipboardData *)clipboard::retrieveData();

                    ActionPastePluginChain *action = new ActionPastePluginChain(
                        *data, pcc->route, pcc->processor);
                    pcc->processor->undoManager.beginNewTransaction(
                        "action paste plugin chain");
                    pcc->processor->undoManager.perform(action);
                }
            });

        pluginMenu.addItem("Copy plugin chain", [this] {
            DBG("copying plugin chain");

            pluginChainClipboardData *chainClipboardData =
                new pluginChainClipboardData;

            chainClipboardData->plugins.reserve(
                pcc->getCorrespondingTrack()->plugins.size());

            for (auto &plugin : pcc->getCorrespondingTrack()->plugins) {
                DBG("copying plugin into plugin chain...");
                pluginClipboardData *data =
                    &chainClipboardData->plugins.emplace_back();

                std::unique_ptr<juce::AudioPluginInstance> *pluginInstance =
                    &plugin->plugin;

                // set trivial data
                data->identifier = pluginInstance->get()
                                       ->getPluginDescription()
                                       .fileOrIdentifier;
                data->bypassed = plugin->bypassed;
                data->dryWetMix = plugin->dryWetMix;
                data->relayParams = plugin->relayParams;

                // set plugin data
                juce::MemoryBlock pluginData;
                pluginInstance->get()->getStateInformation(pluginData);
                data->data = pluginData.toBase64Encoding();
            }

            clipboard::setData(chainClipboardData, TYPECODE_PLUGIN_CHAIN);
            DBG("plugin chain copied");

            // visual feedback
            for (auto &pnc : pluginNodeComponents) {
                pnc->coolColors = true;
            }
            repaint();

            juce::Timer::callAfterDelay(
                UI_VISUAL_FEEDBACK_FLASH_DURATION_MS, [this] {
                    for (auto &pnc : pluginNodeComponents) {
                        pnc->coolColors = false;
                    }
                    repaint();
                });
        });

        pluginMenu.showMenuAsync(juce::PopupMenu::Options());
    }
}

juce::Rectangle<int>
track::PluginNodesWrapper::getBoundsForPluginNodeComponent(int index) {
    return juce::Rectangle<int>(
        ((UI_PLUGIN_NODE_WIDTH + UI_PLUGIN_NODE_MARGIN) * index) + 5, 0,
        UI_PLUGIN_NODE_WIDTH, 88);
}

std::unique_ptr<track::subplugin> *track::PluginNodeComponent::getPlugin() {
    PluginChainComponent *pcc =
        findParentComponentOfClass<PluginChainComponent>();
    jassert(pcc != nullptr);

    audioNode *node = pcc->getCorrespondingTrack();
    jassert(node != nullptr);

    return &node->plugins[(size_t)this->pluginIndex];
}

bool track::PluginNodeComponent::keyStateChanged(bool isKeyDown) {
    if (isKeyDown) {
        // c
        if (juce::KeyPress::isKeyCurrentlyDown(67)) {
            copyPluginToClipboard();
            return true;
        }

        // x, backspace, delete
        else if ((juce::KeyPress::isKeyCurrentlyDown(88) ||
                  juce::KeyPress::isKeyCurrentlyDown(8) ||
                  juce::KeyPress::isKeyCurrentlyDown(268435711))) {
            removeThisPlugin();
            return true;
        }

        // 0
        else if (juce::KeyPress::isKeyCurrentlyDown(48)) {
            toggleBypass();
            return true;
        }

        // e
        // also, N I C E
        else if (juce::KeyPress::isKeyCurrentlyDown(69)) {
            openThisPluginsEditor();
            return true;
        }

        // a
        else if (juce::KeyPress::isKeyCurrentlyDown(65)) {
            openThisPluginsRelayMenu();
            return true;
        }
    }

    return false;
}

bool track::PluginNodeComponent::getPluginBypassedStatus() {
    PluginChainComponent *pcc =
        findParentComponentOfClass<PluginChainComponent>();
    jassert(pcc != nullptr);

    audioNode *node = pcc->getCorrespondingTrack();
    jassert(node != nullptr);

    return node->plugins[(size_t)pluginIndex]->bypassed;
}

track::PluginChainComponent::PluginChainComponent() : Subwindow() {
    addAndMakeVisible(closeBtn);

    nodesWrapper.pcc = this;
    nodesViewport.setViewedComponent(&nodesWrapper);
    addAndMakeVisible(nodesViewport);

    closeBtn.onClose = [this] {
        AudioPluginAudioProcessorEditor *editor =
            findParentComponentOfClass<AudioPluginAudioProcessorEditor>();

        for (size_t i = 0; i < editor->pluginChainComponents.size(); ++i) {
            if (editor->pluginChainComponents[i].get() == this) {
                editor->pluginChainComponents.erase(
                    editor->pluginChainComponents.begin() + (long)i);
                break;
            }
        }
    };

    // addAndMakeVisible(insertIndicator);
}

void track::PluginChainComponent::resized() {
    Subwindow::resized();

    nodesViewport.setScrollBarsShown(false, true, false, true);

    juce::Rectangle<int> nodesViewportBounds = juce::Rectangle<int>(
        UI_SUBWINDOW_SHADOW_SPREAD,
        UI_SUBWINDOW_TITLEBAR_HEIGHT + UI_SUBWINDOW_SHADOW_SPREAD + 4,
        getWidth() - (UI_SUBWINDOW_SHADOW_SPREAD)-4 - 2,
        getHeight() - UI_SUBWINDOW_TITLEBAR_HEIGHT -
            UI_SUBWINDOW_SHADOW_SPREAD - 10 - 2);
    nodesViewportBounds.reduce(2, 0);
    nodesViewport.setBounds(nodesViewportBounds);

    juce::Rectangle<int> nodesWrapperBounds = juce::Rectangle<int>(
        0, 0,
        jmax(getWidth() - 30,
             ((int)this->nodesWrapper.pluginNodeComponents.size() + 1) *
                 (UI_PLUGIN_NODE_WIDTH + UI_PLUGIN_NODE_MARGIN)),
        getHeight() - UI_SUBWINDOW_TITLEBAR_HEIGHT -
            UI_SUBWINDOW_SHADOW_SPREAD - 10);

    nodesWrapper.setBounds(nodesWrapperBounds);
}

track::PluginChainComponent::~PluginChainComponent() {}

void track::PluginChainComponent::paint(juce::Graphics &g) {
    Subwindow::paint(g);

    // titlebar
    g.setColour(juce::Colours::green);
    // g.fillRect(titlebarBounds);

    g.setFont(getTitleBarFont());
    g.setColour(juce::Colour(0xFF'A7A7A7)); // track name text color
    // juce::String x = juce::String(getCorrespondingTrack()->clips.size());
    g.drawText(getCorrespondingTrack()->trackName,
               getTitleBarBounds().withLeft(37 + 6), juce::Justification::left);

    // fx logo
    // outline
    juce::Rectangle<int> fxLogoBounds = juce::Rectangle<int>(
        UI_SUBWINDOW_SHADOW_SPREAD + 8, UI_SUBWINDOW_SHADOW_SPREAD, 30,
        getTitleBarBounds().getHeight());

    juce::Path textPath;
    juce::GlyphArrangement glyphs;
    glyphs.addFittedText(this->getInterBoldItalic().withHeight(22.f), "FX",
                         fxLogoBounds.getX(), fxLogoBounds.getY(),
                         fxLogoBounds.getWidth(), fxLogoBounds.getHeight() - 1,
                         juce::Justification::left, 2);
    glyphs.createPath(textPath);

    g.setColour(juce::Colour(0xFF'6C6C6C)); // "FX" text outline color
    juce::PathStrokeType strokeType(2.f);
    g.strokePath(textPath, strokeType);
    g.setColour(juce::Colours::black);
    g.fillPath(textPath);

    // gradient text fill
    // g.setFont(24.f);

    // gradient stops
    juce::Colour g1 = juce::Colour(0xFF'3B3B3B);
    juce::Colour g2 = juce::Colour(0xFF'565656);
    juce::Colour g3 = juce::Colour(0xFF'313131);

    juce::ColourGradient gradient = juce::ColourGradient::vertical(
        g1, 0.f, g1, getTitleBarBounds().getHeight());
    gradient.addColour(.3f, g2);
    gradient.addColour(.4f, g3);
    gradient.addColour(.6f, g3);
    gradient.addColour(0.9f, g2);

    g.setGradientFill(gradient);
    g.setFont(this->getInterBoldItalic().withHeight(22.f));
    g.drawText("FX", fxLogoBounds, juce::Justification::left, false);
}

track::audioNode *track::PluginChainComponent::getCorrespondingTrack() {
    jassert(processor != nullptr);
    jassert(route.size() > 0);

    return utility::getNodeFromRoute(this->route, processor);
}

void track::PluginChainComponent::removePlugin(int pluginIndex) {
    // getCorrespondingTrack()->removePlugin(pluginIndex);

    pluginClipboardData data;

    std::unique_ptr<track::subplugin> *plugin =
        &getCorrespondingTrack()->plugins[(size_t)pluginIndex];

    // set trivial data
    data.identifier =
        plugin->get()->plugin->getPluginDescription().fileOrIdentifier;
    data.dryWetMix = plugin->get()->dryWetMix;
    data.relayParams = plugin->get()->relayParams;

    // set plugin data
    juce::MemoryBlock pluginData;
    plugin->get()->plugin->getStateInformation(pluginData);
    data.data = pluginData.toBase64Encoding();

    // now remove with action
    ActionRemovePlugin *action =
        new ActionRemovePlugin(data, route, pluginIndex, processor);
    processor->undoManager.beginNewTransaction("action remove plugin");
    processor->undoManager.perform(action);

    nodesWrapper.pluginNodeComponents.clear();
    nodesWrapper.createPluginNodeComponents();
}

void track::PluginChainComponent::updateInsertIndicator(int index) {
    // DBG("pcc updateInsertIndicator() called");

    if (index > -1) {
        this->nodesWrapper.insertIndicator.setVisible(true);
        this->nodesWrapper.insertIndicator.toFront(false);
        this->nodesWrapper.insertIndicator.setBounds(
            ((UI_PLUGIN_NODE_WIDTH + UI_PLUGIN_NODE_MARGIN) * index) +
                (UI_PLUGIN_NODE_MARGIN / 2) - 2,
            0, 1, getHeight());
        repaint();
        return;
    }

    this->nodesWrapper.insertIndicator.setVisible(false);
}

void track::PluginChainComponent::reorderPlugin(int srcIndex, int destIndex) {
    destIndex = juce::jlimit(
        0, (int)getCorrespondingTrack()->plugins.size() - 1, destIndex);

    ActionReorderPlugin *action =
        new ActionReorderPlugin(route, srcIndex, destIndex, processor);
    processor->undoManager.beginNewTransaction("action reorder plugin");
    processor->undoManager.perform(action);
}

// plugin editor window
track::PluginEditorWindow::PluginEditorWindow() : juce::Component() {
    closeBtn.font = getInterBoldItalic();

    closeBtn.onClose = [this] {
        AudioPluginAudioProcessorEditor *editor =
            findParentComponentOfClass<AudioPluginAudioProcessorEditor>();

        for (size_t i = 0; i < editor->pluginEditorWindows.size(); ++i) {
            if (editor->pluginEditorWindows[i].get() == this) {
                editor->pluginEditorWindows.erase(
                    editor->pluginEditorWindows.begin() + (long)i);
                break;
            }
        }
    };

    // closeBtn.behaveLikeANormalCloseButton = false;
    addAndMakeVisible(closeBtn);
}
track::PluginEditorWindow::~PluginEditorWindow() {
    DBG("PEW's destrcutor is called ");
    if (!getPlugin())
        return;
    if (!getPlugin()->get())
        return;

    if (getPlugin()->get()->plugin->getActiveEditor() != nullptr) {
        DBG("active plugin's editor is not nullptr. deleting active "
            "editor");
        delete getPlugin()->get()->plugin->getActiveEditor();

        getPlugin()->get()->plugin->editorBeingDeleted(
            getPlugin()->get()->plugin->getActiveEditor());
    }
}

void track::PluginEditorWindow::paint(juce::Graphics &g) {
#if JUCE_LINUX
    g.setColour(juce::Colour(0xFF'1F1F1F));
    g.drawRect(getLocalBounds(), 2);
#endif

    g.setColour(juce::Colour(0xFF'1F1F1F));
    g.fillRect(0, 0, getWidth(), UI_SUBWINDOW_TITLEBAR_HEIGHT);

    g.setColour(juce::Colour(0xFF'838383));
    g.setFont(
        getInterBoldItalic().withHeight(21.f).withExtraKerningFactor(-.02f));

    int titlebarLeftMargin = UI_SUBWINDOW_TITLEBAR_MARGIN + 5;
    int pluginNameWidth = g.getCurrentFont().getStringWidth(pluginName);

    // plugin name
    g.drawText(pluginName, titlebarLeftMargin, 0, pluginNameWidth,
               UI_SUBWINDOW_TITLEBAR_HEIGHT, juce::Justification::left, false);

    // other info
    g.setColour(juce::Colour(0xFF'595959));
    g.setFont(g.getCurrentFont().withHeight(17.f));

    int latency = getPlugin()->get()->plugin->getLatencySamples();
    float latencyMs =
        (latency / getPlugin()->get()->plugin->getSampleRate()) * 1000.f;
    juce::String otherInfoText = pluginManufacturer + "        " +
                                 juce::String(route[route.size() - 1]) + "/" +
                                 trackName + "        " +
                                 juce::String(latency) + " samples (" +
                                 juce::String(latencyMs, 2, false) + "ms)";
    g.drawText(otherInfoText, pluginNameWidth + titlebarLeftMargin + 8, 0,
               getWidth() / 2, UI_SUBWINDOW_TITLEBAR_HEIGHT,
               juce::Justification::left, false);
}

void track::PluginEditorWindow::resized() {
    int closeBtnSize = UI_SUBWINDOW_TITLEBAR_HEIGHT;
    closeBtn.setBounds(getWidth() - closeBtnSize - UI_SUBWINDOW_TITLEBAR_MARGIN,
                       0, closeBtnSize + UI_SUBWINDOW_TITLEBAR_MARGIN,
                       closeBtnSize);
}

void track::PluginEditorWindow::createEditor() {
    jassert(route.size() > 0);
    jassert(pluginIndex > -1);
    jassert(processor != nullptr);

    this->ape = getPlugin()->get()->plugin->createEditorIfNeeded();
    ape->setBounds(0, UI_SUBWINDOW_TITLEBAR_HEIGHT, 100, 100);

    addAndMakeVisible(*ape);

    // assign data to show on titlebar
    pluginName = getPlugin()->get()->plugin->getPluginDescription().name;
    pluginManufacturer =
        getPlugin()->get()->plugin->getPluginDescription().manufacturerName;
    trackName = getCorrespondingTrack()->trackName;

    DBG(pluginName);
    DBG(pluginManufacturer);

    startTimer(60);
}

void track::PluginEditorWindow::timerCallback() {
    if (ape->getWidth() != this->getWidth() ||
        ape->getHeight() != (UI_SUBWINDOW_TITLEBAR_HEIGHT + ape->getHeight()))
        setSize(ape->getWidth(),
                UI_SUBWINDOW_TITLEBAR_HEIGHT + ape->getHeight());
}

void track::PluginEditorWindow::mouseDown(const juce::MouseEvent & /*event*/) {
    dragStartBounds = getBounds();

#if JUCE_LINUX
    ape->setVisible(false);
#endif

    this->toFront(true);
    this->ape->toFront(true);
}

void track::PluginEditorWindow::mouseDrag(const juce::MouseEvent &event) {
    juce::Rectangle<int> newBounds = this->dragStartBounds;
    newBounds.setX(newBounds.getX() + event.getDistanceFromDragStartX());
    newBounds.setY(newBounds.getY() + event.getDistanceFromDragStartY());
    setBounds(newBounds);

    ape->setBounds(0, UI_SUBWINDOW_TITLEBAR_HEIGHT, getWidth(),
                   getHeight() - UI_SUBWINDOW_TITLEBAR_HEIGHT);

    ape->resized();
    ape->repaint();
}

void track::PluginEditorWindow::mouseUp(const juce::MouseEvent &event) {
    if (event.mouseWasDraggedSinceMouseDown())

        DBG("DRAG STOPPED");

    ape->setVisible(true);

#if JUCE_LINUX
    ape->postCommandMessage(COMMAND_UPDATE_VST3_EMBEDDED_BOUNDS);
#endif
}

// get current track and plugin util functions
track::audioNode *track::PluginEditorWindow::getCorrespondingTrack() {
    return utility::getNodeFromRoute(this->route, processor);
}

std::unique_ptr<track::subplugin> *track::PluginEditorWindow::getPlugin() {
    return &getCorrespondingTrack()->plugins[(size_t)this->pluginIndex];
}
