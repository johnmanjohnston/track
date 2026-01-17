#include "plugin_chain.h"
#include "clipboard.h"
#include "defs.h"
#include "subwindow.h"
#include "track.h"
#include "utility.h"
#include <cstddef>

track::ActionAddPlugin::ActionAddPlugin(std::vector<int> route,
                                        juce::String identifier,
                                        void *processor, void *editor)
    : juce::UndoableAction() {
    this->nodeRoute = route;
    this->pluginIdentifier = identifier;
    this->p = processor;
    this->e = editor;
}
track::ActionAddPlugin::~ActionAddPlugin() {}

bool track::ActionAddPlugin::perform() {
    audioNode *node = utility::getNodeFromRoute(nodeRoute, p);
    validPlugin = node->addPlugin(pluginIdentifier);

    updateGUI();

    return validPlugin;
}

bool track::ActionAddPlugin::undo() {
    if (!validPlugin)
        return false;

    // close editor, then remove plugin
    AudioPluginAudioProcessorEditor *editor =
        (AudioPluginAudioProcessorEditor *)e;

    audioNode *node = utility::getNodeFromRoute(nodeRoute, p);
    editor->closePluginEditorWindow(nodeRoute, node->plugins.size() - 1);

    node->removePlugin(node->plugins.size() - 1);

    updateGUI();

    return true;
}

void track::ActionAddPlugin::updateGUI() {
    AudioPluginAudioProcessorEditor *editor =
        (AudioPluginAudioProcessorEditor *)e;

    for (size_t i = 0; i < editor->pluginChainComponents.size(); ++i) {
        if (editor->pluginChainComponents[i]->route == nodeRoute) {
            // recreate plugin node components

            editor->pluginChainComponents[i]
                ->nodesWrapper.pluginNodeComponents.clear();
            editor->pluginChainComponents[i]
                ->nodesWrapper.createPluginNodeComponents();
        }
    }
}

track::ActionRemovePlugin::ActionRemovePlugin(track::pluginClipboardData data,
                                              std::vector<int> route, int index,
                                              void *processor, void *editor)
    : juce::UndoableAction() {
    this->subpluginData = data;
    this->nodeRoute = route;
    this->p = processor;
    this->e = editor;
    this->pluginIndex = index;
};
track::ActionRemovePlugin::~ActionRemovePlugin(){};

bool track::ActionRemovePlugin::perform() {
    utility::closeOpenedEditors(nodeRoute, &openedPlugins, p, e);

    audioNode *node = utility::getNodeFromRoute(nodeRoute, p);
    node->removePlugin(pluginIndex);

    updateGUI();
    utility::openEditors(nodeRoute, openedPlugins, p, e);

    return true;
}

bool track::ActionRemovePlugin::undo() {
    utility::closeOpenedEditors(nodeRoute, &openedPlugins, p, e);

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
    utility::openEditors(nodeRoute, openedPlugins, p, e);

    return true;
}

void track::ActionRemovePlugin::updateGUI() {
    AudioPluginAudioProcessorEditor *editor =
        (AudioPluginAudioProcessorEditor *)e;

    for (size_t i = 0; i < editor->pluginChainComponents.size(); ++i) {
        if (editor->pluginChainComponents[i]->route == nodeRoute) {
            editor->pluginChainComponents[i]
                ->nodesWrapper.pluginNodeComponents.clear();
            editor->pluginChainComponents[i]
                ->nodesWrapper.createPluginNodeComponents();
        }
    }
}

track::ActionReorderPlugin::ActionReorderPlugin(std::vector<int> nodeRoute,
                                                int sourceIndex,
                                                int destinationIndex,
                                                void *processor, void *editor)
    : juce::UndoableAction() {
    this->route = nodeRoute;
    this->srcIndex = sourceIndex;
    this->destIndex = destinationIndex;
    this->p = processor;
    this->e = editor;
}
track::ActionReorderPlugin::~ActionReorderPlugin(){};

bool track::ActionReorderPlugin::perform() {
    utility::closeOpenedEditors(route, &openEditorsPlugins, p, e);
    utility::closeOpenedRelayParamWindows(route, &openRelayMenuPlugins, p, e);

    audioNode *node = utility::getNodeFromRoute(route, p);
    utility::reorderPlugin(srcIndex, destIndex, node);

    updateGUI();

    utility::openEditors(route, openEditorsPlugins, p, e);
    utility::openRelayParamWindows(route, openRelayMenuPlugins, p, e);

    this->openEditorsPlugins.clear();
    this->openRelayMenuPlugins.clear();

    return true;
}

bool track::ActionReorderPlugin::undo() {
    utility::closeOpenedEditors(route, &openEditorsPlugins, p, e);
    utility::closeOpenedRelayParamWindows(route, &openRelayMenuPlugins, p, e);

    audioNode *node = utility::getNodeFromRoute(route, p);
    utility::reorderPlugin(srcIndex, destIndex, node);

    updateGUI();

    utility::openEditors(route, openEditorsPlugins, p, e);
    utility::openRelayParamWindows(route, openRelayMenuPlugins, p, e);

    this->openEditorsPlugins.clear();
    this->openRelayMenuPlugins.clear();

    return true;
}

void track::ActionReorderPlugin::updateGUI() {
    AudioPluginAudioProcessorEditor *editor =
        (AudioPluginAudioProcessorEditor *)e;

    for (size_t i = 0; i < editor->pluginChainComponents.size(); ++i) {
        if (editor->pluginChainComponents[i]->route == route) {
            editor->pluginChainComponents[i]
                ->nodesWrapper.pluginNodeComponents.clear();
            editor->pluginChainComponents[i]
                ->nodesWrapper.createPluginNodeComponents();
        }
    }
}

track::ActionChangeTrivialPluginData::ActionChangeTrivialPluginData(
    pluginClipboardData oldData, pluginClipboardData newData,
    std::vector<int> nodeRoute, int pluginIndex, void *processor, void *editor)
    : juce::UndoableAction() {
    this->oldPluginData = oldData;
    this->newPluginData = newData;
    this->route = nodeRoute;
    this->index = pluginIndex;
    this->p = processor;
    this->e = editor;
}
track::ActionChangeTrivialPluginData::~ActionChangeTrivialPluginData() {}

bool track::ActionChangeTrivialPluginData::perform() {
    audioNode *node = utility::getNodeFromRoute(route, p);
    subplugin *plugin = node->plugins[(size_t)index].get();

    plugin->bypassed = newPluginData.bypassed;
    plugin->dryWetMix = newPluginData.dryWetMix;
    plugin->relayParams = newPluginData.relayParams;

    updateGUI();

    return true;
}
bool track::ActionChangeTrivialPluginData::undo() {
    audioNode *node = utility::getNodeFromRoute(route, p);
    subplugin *plugin = node->plugins[(size_t)index].get();

    plugin->bypassed = oldPluginData.bypassed;
    plugin->dryWetMix = oldPluginData.dryWetMix;
    plugin->relayParams = oldPluginData.relayParams;

    updateGUI();

    return true;
}
void track::ActionChangeTrivialPluginData::updateGUI() {
    AudioPluginAudioProcessorEditor *editor =
        (AudioPluginAudioProcessorEditor *)e;

    for (size_t i = 0; i < editor->pluginChainComponents.size(); ++i) {
        if (editor->pluginChainComponents[i]->route == route) {
            editor->pluginChainComponents[i]
                ->nodesWrapper.pluginNodeComponents.clear();

            editor->pluginChainComponents[i]
                ->nodesWrapper.createPluginNodeComponents();
        }
    }

    for (size_t i = 0; i < editor->relayManagerCompnoents.size(); ++i) {
        if (editor->relayManagerCompnoents[i]->route == route)
            editor->relayManagerCompnoents[i]
                ->rmNodesWrapper.createRelayNodes();
    }
};

track::PluginNodeComponent::PluginNodeComponent() : juce::Component() {
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
        AudioPluginAudioProcessorEditor *editor =
            pcc->findParentComponentOfClass<AudioPluginAudioProcessorEditor>();

        ActionChangeTrivialPluginData *action =
            new ActionChangeTrivialPluginData(oldPluginData, pluginData,
                                              pcc->route, pluginIndex,
                                              pcc->processor, editor);
        pcc->processor->undoManager.beginNewTransaction(
            "action change trivial plugin data");
        pcc->processor->undoManager.perform(action);
    };

    openEditorBtn.onClick = [this] {
        PluginChainComponent *pcc =
            findParentComponentOfClass<PluginChainComponent>();
        AudioPluginAudioProcessorEditor *editor =
            this->findParentComponentOfClass<AudioPluginAudioProcessorEditor>();

        DBG("pluginIndex = " << pluginIndex);

        if (editor->isPluginEditorWindowOpen(pcc->route, pluginIndex))
            editor->closePluginEditorWindow(pcc->route, pluginIndex);

        editor->openPluginEditorWindow(pcc->route, pluginIndex);
    };

    removePluginBtn.onClick = [this] {
        PluginChainComponent *pcc =
            findParentComponentOfClass<PluginChainComponent>();
        jassert(pcc != nullptr);

        // close editor before removing plugin otherwise segfault
        AudioPluginAudioProcessorEditor *editor =
            this->findParentComponentOfClass<AudioPluginAudioProcessorEditor>();
        editor->closePluginEditorWindow(pcc->route, pluginIndex);
        editor->closeAllRelayMenusWithRouteAndPluginIndex(pcc->route,
                                                          pluginIndex);

        pcc->removePlugin(this->pluginIndex);
        pcc->resized();
    };

    bypassBtn.onClick = [this] {
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
        AudioPluginAudioProcessorEditor *editor =
            pcc->findParentComponentOfClass<AudioPluginAudioProcessorEditor>();

        ActionChangeTrivialPluginData *action =
            new ActionChangeTrivialPluginData(oldPluginData, pluginData,
                                              pcc->route, pluginIndex,
                                              pcc->processor, editor);
        pcc->processor->undoManager.beginNewTransaction(
            "action change trivial plugin data");
        pcc->processor->undoManager.perform(action);

        repaint();
    };

    automationButton.onClick = [this] {
        DBG("automation button clicked");

        PluginChainComponent *pcc =
            findParentComponentOfClass<PluginChainComponent>();
        jassert(pcc != nullptr);
        AudioPluginAudioProcessorEditor *editor =
            this->findParentComponentOfClass<AudioPluginAudioProcessorEditor>();
        editor->openRelayMenu(pcc->route, this->pluginIndex);
    };
}
track::PluginNodeComponent::~PluginNodeComponent() {}
track::PluginNodesWrapper::PluginNodesWrapper() : juce::Component() {
    addAndMakeVisible(insertIndicator);
}
track::PluginNodesWrapper::~PluginNodesWrapper() {}
track::PluginNodesViewport::PluginNodesViewport() : juce::Viewport() {}
track::PluginNodesViewport::~PluginNodesViewport() {}

void track::PluginNodeComponent::mouseDown(const juce::MouseEvent &event) {
    if (event.mods.isRightButtonDown()) {
        if (event.mods.isRightButtonDown()) {
            juce::PopupMenu menu;

            menu.addItem("Copy plugin", [this] {
                // create clipboard data
                pluginClipboardData *data = new pluginClipboardData;
                std::unique_ptr<juce::AudioPluginInstance> *pluginInstance =
                    &getPlugin()->get()->plugin;

                // set trivial data
                data->identifier = pluginInstance->get()
                                       ->getPluginDescription()
                                       .fileOrIdentifier;
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

                juce::Timer::callAfterDelay(
                    UI_VISUAL_FEEDBACK_FLASH_DURATION_MS, [this] {
                        coolColors = false;
                        repaint();
                    });
            });

            menu.setLookAndFeel(&getLookAndFeel());
            menu.showMenuAsync(juce::PopupMenu::Options());
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
        int displayNodes = (int)(mouseX / (float)(getWidth() + 4)) - 1;

        DBG("FINAL NODE INDEX = " << displayNodes);

        pcc->updateInsertIndicator(-1);

        pcc->reorderPlugin(this->pluginIndex, displayNodes);
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

void track::PluginNodeComponent::paint(juce::Graphics &g) {
    if (getPlugin() && getPlugin()->get()) {
        if (!coolColors)
            g.fillAll(juce::Colour(0xFF'121212));
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
                        pcc->processor, editor);
                    pcc->processor->undoManager.beginNewTransaction(
                        "action add plugin");
                    pcc->processor->undoManager.perform(action);

                    if (action->validPlugin) {
                        int pluginIndex = node->plugins.size() - 1;

                        editor->openPluginEditorWindow(pcc->route, pluginIndex);

                        /*
                        // very lazy way but i couldn't be bothered now
                        pluginNodeComponents.clear();
                        createPluginNodeComponents();
                        */

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
            [this, node] {
                if (clipboard::typecode == TYPECODE_PLUGIN) {
                    pluginClipboardData *clipboardData =
                        (pluginClipboardData *)clipboard::retrieveData();

                    juce::String cleanedIdentifier =
                        clipboardData->identifier.upToLastOccurrenceOf(
                            ".vst3", true, true);
                    node->addPlugin(cleanedIdentifier);
                    juce::MemoryBlock pluginData;

                    std::unique_ptr<track::subplugin> &plugin =
                        node->plugins.back();

                    plugin->bypassed = clipboardData->bypassed;
                    plugin->dryWetMix = clipboardData->dryWetMix;
                    plugin->relayParams = clipboardData->relayParams;

                    bool dataRetrieved = pluginData.fromBase64Encoding(
                        (juce::String)clipboardData->data);

                    DBG("dataRetrieved = " << (dataRetrieved ? "true"
                                                             : "false"));

                    plugin->plugin->setStateInformation(pluginData.getData(),
                                                        pluginData.getSize());
                    // very lazy way but i couldn't be bothered now
                    pluginNodeComponents.clear();
                    createPluginNodeComponents();

                    pcc->resized();
                    pcc->nodesViewport.setViewPosition(
                        pcc->nodesWrapper.getWidth(), 0);

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
                    audioNode *node = pcc->getCorrespondingTrack();

                    for (pluginClipboardData &pluginClipboardData :
                         data->plugins) {

                        juce::String cleanedIdentifier =
                            pluginClipboardData.identifier.upToLastOccurrenceOf(
                                ".vst3", true, true);
                        node->addPlugin(cleanedIdentifier);

                        juce::MemoryBlock pluginData;

                        std::unique_ptr<track::subplugin> &plugin =
                            node->plugins.back();

                        plugin->bypassed = pluginClipboardData.bypassed;
                        plugin->dryWetMix = pluginClipboardData.dryWetMix;
                        plugin->relayParams = pluginClipboardData.relayParams;

                        bool dataRetrieved = pluginData.fromBase64Encoding(
                            (juce::String)pluginClipboardData.data);

                        DBG("dataRetrieved = " << (dataRetrieved ? "true"
                                                                 : "false"));

                        plugin->plugin->setStateInformation(
                            pluginData.getData(), pluginData.getSize());
                        // very lazy way but i couldn't be bothered now
                        pluginNodeComponents.clear();
                        createPluginNodeComponents();

                        pcc->resized();
                        pcc->nodesViewport.setViewPosition(
                            pcc->nodesWrapper.getWidth(), 0);
                    }
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
    return juce::Rectangle<int>((UI_PLUGIN_NODE_WIDTH + UI_PLUGIN_NODE_MARGIN) *
                                    index,
                                0, UI_PLUGIN_NODE_WIDTH, 88);
}

std::unique_ptr<track::subplugin> *track::PluginNodeComponent::getPlugin() {
    PluginChainComponent *pcc =
        findParentComponentOfClass<PluginChainComponent>();
    jassert(pcc != nullptr);

    audioNode *node = pcc->getCorrespondingTrack();
    jassert(node != nullptr);

    return &node->plugins[(size_t)this->pluginIndex];
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

    juce::Rectangle<int> nodesViewportBounds =
        juce::Rectangle<int>(1, UI_SUBWINDOW_TITLEBAR_HEIGHT + 5, getWidth(),
                             getHeight() - UI_SUBWINDOW_TITLEBAR_HEIGHT);
    nodesViewportBounds.reduce(5, 0);
    nodesViewport.setBounds(nodesViewportBounds);

    juce::Rectangle<int> nodesWrapperBounds = juce::Rectangle<int>(
        0, 0,
        jmax(getWidth() - 30,
             ((int)this->nodesWrapper.pluginNodeComponents.size() + 1) *
                 (UI_PLUGIN_NODE_WIDTH + UI_PLUGIN_NODE_MARGIN)),
        getHeight() - UI_SUBWINDOW_TITLEBAR_HEIGHT - 1);

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
               getTitleBarBounds().withLeft(37).withTop(2),
               juce::Justification::left);

    // fx logo
    // outline
    juce::Rectangle<int> fxLogoBounds =
        juce::Rectangle<int>(8, 1, 30, getTitleBarBounds().getHeight());

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

    audioNode *head = &processor->tracks[(size_t)route[0]];
    for (size_t i = 1; i < route.size(); ++i) {
        head = &head->childNodes[(size_t)route[i]];
    }

    return head;
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
    ActionRemovePlugin *action = new ActionRemovePlugin(
        data, route, pluginIndex, processor,
        findParentComponentOfClass<AudioPluginAudioProcessorEditor>());
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
    AudioPluginAudioProcessorEditor *editor =
        this->findParentComponentOfClass<AudioPluginAudioProcessorEditor>();

    destIndex = juce::jlimit(
        0, (int)getCorrespondingTrack()->plugins.size() - 1, destIndex);

    ActionReorderPlugin *action =
        new ActionReorderPlugin(route, srcIndex, destIndex, processor, editor);
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
    g.setColour(juce::Colours::black);
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

void track::PluginEditorWindow::mouseDown(const juce::MouseEvent &event) {
    if (juce::ModifierKeys::currentModifiers.isAltDown()) {
        dragStartBounds = getBounds();

#if JUCE_LINUX
        ape->setVisible(false);
#endif
    }

    this->toFront(true);
    this->ape->toFront(true);
}

void track::PluginEditorWindow::mouseDrag(const juce::MouseEvent &event) {
    if (juce::ModifierKeys::currentModifiers.isAltDown()) {
        juce::Rectangle<int> newBounds = this->dragStartBounds;
        newBounds.setX(newBounds.getX() + event.getDistanceFromDragStartX());
        newBounds.setY(newBounds.getY() + event.getDistanceFromDragStartY());
        setBounds(newBounds);

        ape->setBounds(0, UI_SUBWINDOW_TITLEBAR_HEIGHT, getWidth(),
                       getHeight() - UI_SUBWINDOW_TITLEBAR_HEIGHT);
    }

    ape->resized();
    ape->repaint();
}

void track::PluginEditorWindow::mouseUp(const juce::MouseEvent &event) {
    if (event.mouseWasDraggedSinceMouseDown() ||
        juce::ModifierKeys::currentModifiers.isAltDown()) {
        DBG("DRAG STOPPED");

        ape->setVisible(true);

#if JUCE_LINUX
        ape->postCommandMessage(COMMAND_UPDATE_VST3_EMBEDDED_BOUNDS);
#endif
    }
}

// get current track and plugin util functions
track::audioNode *track::PluginEditorWindow::getCorrespondingTrack() {
    audioNode *head = &processor->tracks[(size_t)route[0]];
    for (size_t i = 1; i < route.size(); ++i) {
        head = &head->childNodes[(size_t)route[i]];
    }

    return head;
}

std::unique_ptr<track::subplugin> *track::PluginEditorWindow::getPlugin() {
    return &getCorrespondingTrack()->plugins[(size_t)this->pluginIndex];
}
