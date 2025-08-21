#include "plugin_chain.h"
#include "defs.h"
#include "track.h"

track::PluginNodeComponent::PluginNodeComponent() : juce::Component() {
    this->openEditorBtn.setButtonText("EDITOR");
    addAndMakeVisible(openEditorBtn);

    this->removePluginBtn.setButtonText("REMOVE");
    addAndMakeVisible(removePluginBtn);

    this->bypassBtn.setButtonText("BYPASS");
    addAndMakeVisible(bypassBtn);

    openEditorBtn.onClick = [this] {
        PluginChainComponent *pcc =
            findParentComponentOfClass<PluginChainComponent>();
        AudioPluginAudioProcessorEditor *editor =
            this->findParentComponentOfClass<AudioPluginAudioProcessorEditor>();

        DBG("pluginIndex = " << pluginIndex);

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

        pcc->removePlugin(this->pluginIndex);
        pcc->resized();
    };

    bypassBtn.onClick = [this] {
        PluginChainComponent *pcc =
            findParentComponentOfClass<PluginChainComponent>();
        jassert(pcc != nullptr);

        audioNode *node = pcc->getCorrespondingTrack();
        jassert(node != nullptr);

        node->bypassedPlugins[(size_t)this->pluginIndex] =
            !getPluginBypassedStatus();

        repaint();
    };
}
track::PluginNodeComponent::~PluginNodeComponent() {}
track::PluginNodesWrapper::PluginNodesWrapper() : juce::Component() {}
track::PluginNodesWrapper::~PluginNodesWrapper() {}
track::PluginNodesViewport::PluginNodesViewport() : juce::Viewport() {}
track::PluginNodesViewport::~PluginNodesViewport() {}

void track::PluginNodeComponent::paint(juce::Graphics &g) {
    if (getPlugin() && getPlugin()->get()) {
        g.fillAll(juce::Colour(0xFF'121212));

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
        g.drawText(getPlugin()->get()->getName(), 10, 8, getWidth(), 20,
                   juce::Justification::left);
    }
}

void track::PluginNodeComponent::resized() {
    int btnWidth = 76;
    int btnHeight = 21;
    int initialLeftMargin = 7;
    int bottomMargin = 7;
    int buttonSideMargin = 4;

    this->openEditorBtn.setBounds(initialLeftMargin,
                                  getHeight() - btnHeight - bottomMargin,
                                  btnWidth, btnHeight);

    this->removePluginBtn.setBounds(
        initialLeftMargin + btnWidth + buttonSideMargin,
        getHeight() - btnHeight - bottomMargin, btnWidth, btnHeight);

    this->bypassBtn.setBounds(
        initialLeftMargin + ((btnWidth + buttonSideMargin) * 2),
        getHeight() - btnHeight - bottomMargin, btnWidth, btnHeight);
}

void track::PluginNodesWrapper::createPluginNodeComponents() {
    jassert(pcc != nullptr);

    audioNode *node = pcc->getCorrespondingTrack();

    for (size_t i = 0; i < node->plugins.size(); ++i) {
        DBG("creating plugin node for " << node->plugins[i].get()->getName());

        this->pluginNodeComponents.emplace_back(new track::PluginNodeComponent);
        PluginNodeComponent &nc = *this->pluginNodeComponents.back();
        nc.pluginIndex = i;

        addAndMakeVisible(nc);
        nc.setBounds(getBoundsForPluginNodeComponent(i));
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
                    node->addPlugin(pluginDescription.fileOrIdentifier);

                    // open its editor
                    AudioPluginAudioProcessorEditor *editor =
                        this->findParentComponentOfClass<
                            AudioPluginAudioProcessorEditor>();

                    int pluginIndex = node->plugins.size() - 1;

                    DBG("pluginIndex = " << pluginIndex);

                    editor->openPluginEditorWindow(pcc->route, pluginIndex);

                    // create node component
                    this->pluginNodeComponents.emplace_back(
                        new track::PluginNodeComponent);
                    PluginNodeComponent &nc =
                        *this->pluginNodeComponents.back();
                    nc.pluginIndex = pluginIndex;

                    addAndMakeVisible(nc);
                    nc.setBounds(getBoundsForPluginNodeComponent(pluginIndex));

                    pcc->resized();
                    pcc->nodesViewport.setViewPosition(
                        pcc->nodesWrapper.getWidth(), 0);
                });
        }

        pluginMenu.addSubMenu("Add plugin", pluginSelector);
        pluginMenu.showMenuAsync(juce::PopupMenu::Options());
    }
}

juce::Rectangle<int>
track::PluginNodesWrapper::getBoundsForPluginNodeComponent(int index) {
    int width = 250;
    int sideMargin = 4;
    return juce::Rectangle<int>((width + sideMargin) * index, 0, width, 88);
}

std::unique_ptr<juce::AudioPluginInstance> *
track::PluginNodeComponent::getPlugin() {
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

    return node->bypassedPlugins[(size_t)pluginIndex];
}

track::PluginChainComponent::PluginChainComponent() : juce::Component() {
    addAndMakeVisible(closeBtn);

    closeBtn.font = getInterBoldItalic();
    addAndMakeVisible(closeBtn);

    nodesWrapper.pcc = this;
    nodesViewport.setViewedComponent(&nodesWrapper);
    addAndMakeVisible(nodesViewport);
}

void track::PluginChainComponent::resized() {
    int closeBtnSize = UI_SUBWINDOW_TITLEBAR_HEIGHT;
    closeBtn.setBounds(getWidth() - closeBtnSize - UI_SUBWINDOW_TITLEBAR_MARGIN,
                       0, closeBtnSize + UI_SUBWINDOW_TITLEBAR_MARGIN,
                       closeBtnSize);

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
                 (250 + 4)),
        getHeight() - UI_SUBWINDOW_TITLEBAR_HEIGHT - 1);

    nodesWrapper.setBounds(nodesWrapperBounds);
}

track::PluginChainComponent::~PluginChainComponent() {}

void track::PluginChainComponent::paint(juce::Graphics &g) {
    // bg
    g.fillAll(juce::Colour(0xFF'282828));

    // titlebar
    juce::Rectangle<int> titlebarBounds = getLocalBounds();
    titlebarBounds.setHeight(UI_SUBWINDOW_TITLEBAR_HEIGHT);
    titlebarBounds.reduce(UI_SUBWINDOW_TITLEBAR_MARGIN, 0);
    g.setColour(juce::Colours::green);
    // g.fillRect(titlebarBounds);

    g.setFont(this->getInterBoldItalic().withHeight(19.f));
    g.setColour(juce::Colour(0xFF'A7A7A7)); // track name text color
    // juce::String x = juce::String(getCorrespondingTrack()->clips.size());
    g.drawText(getCorrespondingTrack()->trackName,
               titlebarBounds.withLeft(37).withTop(2),
               juce::Justification::left);

    // fx logo
    // outline
    juce::Rectangle<int> fxLogoBounds =
        juce::Rectangle<int>(8, 1, 30, titlebarBounds.getHeight());

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

    juce::ColourGradient gradient =
        juce::ColourGradient::vertical(g1, 0.f, g1, titlebarBounds.getHeight());
    gradient.addColour(.3f, g2);
    gradient.addColour(.4f, g3);
    gradient.addColour(.6f, g3);
    gradient.addColour(0.9f, g2);

    g.setGradientFill(gradient);
    g.setFont(this->getInterBoldItalic().withHeight(22.f));
    g.drawText("FX", fxLogoBounds, juce::Justification::left, false);

    // border
    g.setColour(juce::Colour(0xFF'4A4A4A));
    // g.drawHorizontalLine(titlebarBounds.getHeight(), 0, getWidth());
    g.fillRect(0, titlebarBounds.getHeight(), getWidth(), 2);
    g.drawRect(getLocalBounds(), 2);

    g.setColour(juce::Colours::white);
    g.setFont(g.getCurrentFont().withHeight(18.f));
    g.drawText(juce::String(getCorrespondingTrack()->getTotalLatencySamples()),
               50, 0, 100, 50, juce::Justification::right);
}

void track::PluginChainComponent::mouseDrag(const juce::MouseEvent &event) {
    if (juce::ModifierKeys::currentModifiers.isAltDown()) {
        juce::Rectangle<int> newBounds = this->dragStartBounds;
        newBounds.setX(newBounds.getX() + event.getDistanceFromDragStartX());
        newBounds.setY(newBounds.getY() + event.getDistanceFromDragStartY());
        setBounds(newBounds);
    }
}

void track::PluginChainComponent::mouseDown(const juce::MouseEvent &event) {
    if (juce::ModifierKeys::currentModifiers.isAltDown())
        dragStartBounds = getBounds();

    this->toFront(true);
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
    getCorrespondingTrack()->removePlugin(pluginIndex);

    // TODO: optimize this instead of using lazy where to recreate all p
    nodesWrapper.pluginNodeComponents.clear();
    nodesWrapper.createPluginNodeComponents();
}

// plugin editor window
track::PluginEditorWindow::PluginEditorWindow() : juce::Component() {
    closeBtn.font = getInterBoldItalic();
    closeBtn.behaveLikeANormalCloseButton = false;
    addAndMakeVisible(closeBtn);
}
track::PluginEditorWindow::~PluginEditorWindow() {
    DBG("PEW's destrcutor is called ");
    if (!getPlugin())
        return;
    if (!getPlugin()->get())
        return;

    if (getPlugin()->get()->getActiveEditor() != nullptr) {
        DBG("active plugin's editor is not nullptr. deleting active "
            "editor");
        delete getPlugin()->get()->getActiveEditor();

        getPlugin()->get()->editorBeingDeleted(
            getPlugin()->get()->getActiveEditor());
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

    int latency = getPlugin()->get()->getLatencySamples();
    float latencyMs = (latency / getPlugin()->get()->getSampleRate()) * 1000.f;
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

    this->ape = getPlugin()->get()->createEditorIfNeeded();
    ape->setBounds(0, UI_SUBWINDOW_TITLEBAR_HEIGHT, 100, 100);

    addAndMakeVisible(*ape);

    // assign data to show on titlebar
    pluginName = getPlugin()->get()->getPluginDescription().name;
    pluginManufacturer =
        getPlugin()->get()->getPluginDescription().manufacturerName;
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

        ape->setBounds(0, UI_SUBWINDOW_TITLEBAR_HEIGHT, getWidth() - 1,
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

std::unique_ptr<juce::AudioPluginInstance> *
track::PluginEditorWindow::getPlugin() {
    return &getCorrespondingTrack()->plugins[(size_t)this->pluginIndex];
}

// custom close button because lookandfeels are annoying
track::CloseButton::CloseButton() : juce::Component(), font() {}
track::CloseButton::~CloseButton() {}

void track::CloseButton::paint(juce::Graphics &g) {
    g.setFont(font.withHeight(22.f));
    g.setColour(isHoveredOver == true ? hoveredColor : normalColor);
    g.drawText("X", 0, 0, getWidth(), getHeight(), juce::Justification::centred,
               false);
}

void track::CloseButton::mouseUp(const juce::MouseEvent &event) {
    if (!event.mods.isLeftButtonDown())
        return;

    // absolute cinema
    if (behaveLikeANormalCloseButton) {
        juce::Component *componentToRemove =
            (juce::Component *)getParentComponent();
        jassert(componentToRemove != nullptr);

        juce::Component *componentToRemoveParent =
            (juce::Component *)componentToRemove->getParentComponent();
        jassert(componentToRemoveParent != nullptr);

        componentToRemoveParent->removeChildComponent(componentToRemove);
    } else {
        track::PluginEditorWindow *pew =
            (track::PluginEditorWindow *)getParentComponent();

        AudioPluginAudioProcessorEditor *editor =
            findParentComponentOfClass<AudioPluginAudioProcessorEditor>();

        for (size_t i = 0; i < editor->pluginEditorWindows.size(); ++i) {
            if (editor->pluginEditorWindows[i].get() == pew) {
                editor->pluginEditorWindows.erase(
                    editor->pluginEditorWindows.begin() + (long)i);
                break;
            }
        }
    }
}

void track::CloseButton::mouseEnter(const juce::MouseEvent &event) {
    isHoveredOver = true;
    repaint();
}
void track::CloseButton::mouseExit(const juce::MouseEvent &event) {
    isHoveredOver = false;
    repaint();
}
