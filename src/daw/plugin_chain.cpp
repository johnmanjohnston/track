#include "plugin_chain.h"
#include "defs.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"

track::PluginChainComponent::PluginChainComponent() : juce::Component() {
    // setSize(500, 500);

    // addPluginBtn.setButtonText("e");
    //  closeBtn.setColour(0x1004011, juce::Colours::red);
    //  closeBtn.getLookAndFeel().setColour(0x1004011, juce::Colours::red);
    //  closeBtn.setLookAndFeel(nullptr);
    addAndMakeVisible(closeBtn);
    // addAndMakeVisible(addPluginBtn);

    closeBtn.font = getInterBoldItalic();
    addAndMakeVisible(closeBtn);

    /*
    addPluginBtn.onClick = [this] {
        DBG("add plugin btn clicked");

        // add actual subplugin
        juce::String pluginPath =
            juce::File("/home/johnston/.vst3/ZL Equalizer.vst3/")
                .getFullPathName();
        getCorrespondingTrack()->addPlugin(pluginPath);

        DBG("opening editor...");
        // open subplugin's editor
        // AudioPluginAudioProcessorEditor *editor =
        //(AudioPluginAudioProcessorEditor *)getParentComponent();
        AudioPluginAudioProcessorEditor *editor =
            this->findParentComponentOfClass<AudioPluginAudioProcessorEditor>();

        int pluginIndex = getCorrespondingTrack()->plugins.size() - 1;

        DBG("trackIndex = " << this->trackIndex);
        DBG("pluginIndex = " << pluginIndex);

        editor->openPluginEditorWindow(trackIndex, pluginIndex);

    };
        */
}

void track::PluginChainComponent::resized() {
    int closeBtnSize = UI_SUBWINDOW_TITLEBAR_HEIGHT;
    closeBtn.setBounds(getWidth() - closeBtnSize - UI_SUBWINDOW_TITLEBAR_MARGIN,
                       0, closeBtnSize + UI_SUBWINDOW_TITLEBAR_MARGIN,
                       closeBtnSize);

    // addPluginBtn.setBounds(10, 10, 100, 100);
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

    // what plugins are added?
    g.setColour(juce::Colours::white);
    for (size_t i = 0; i < getCorrespondingTrack()->plugins.size(); ++i) {
        juce::String name = getCorrespondingTrack()->plugins[i]->getName();
        g.drawText(name, 20, 20 + (i * 12), getWidth(), 8,
                   juce::Justification::left, false);
    }
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

    if (event.mods.isRightButtonDown()) {
        jassert(knownPluginList != nullptr);

        juce::PopupMenu pluginSelector;
        pluginSelector.setLookAndFeel(&getLookAndFeel());

        for (auto pluginDescription : knownPluginList->getTypes()) {
            pluginSelector.addItem(pluginDescription.name, [this,
                                                            pluginDescription] {
                // add plugin
                DBG("adding " << pluginDescription.name);
                getCorrespondingTrack()->addPlugin(
                    pluginDescription.fileOrIdentifier);

                // open its editor
                AudioPluginAudioProcessorEditor *editor =
                    this->findParentComponentOfClass<
                        AudioPluginAudioProcessorEditor>();

                int pluginIndex = getCorrespondingTrack()->plugins.size() - 1;

                DBG("trackIndex = " << this->trackIndex);
                DBG("pluginIndex = " << pluginIndex);

                editor->openPluginEditorWindow(trackIndex, pluginIndex);
            });
        }

        pluginSelector.showMenuAsync(juce::PopupMenu::Options());
    }
}

track::track *track::PluginChainComponent::getCorrespondingTrack() {
    return &processor->tracks[(size_t)trackIndex];
}

// plugin editor window
track::PluginEditorWindow::PluginEditorWindow() : juce::Component() {
    closeBtn.font = getInterBoldItalic();
    addAndMakeVisible(closeBtn);
}
track::PluginEditorWindow::~PluginEditorWindow() {}

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
    juce::String otherInfoText =
        pluginManufacturer + "        " + juce::String(trackIndex) + "/" +
        trackName + "        " + juce::String(latency) + " samples (" +
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
    jassert(trackIndex > -1);
    jassert(pluginIndex > -1);
    jassert(processor != nullptr);

    this->ape = std::unique_ptr<juce::AudioProcessorEditor>(
        getPlugin()->get()->createEditorIfNeeded());
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

    // DBG("ape's dimensions are x,y" << ape->getWidth() << ","
    // << ape->getHeight());

    if (ape->getWidth() != this->getWidth() ||
        ape->getHeight() != (UI_SUBWINDOW_TITLEBAR_HEIGHT + ape->getHeight()))
        setSize(ape->getWidth(),
                UI_SUBWINDOW_TITLEBAR_HEIGHT + ape->getHeight());
}

void track::PluginEditorWindow::mouseDown(const juce::MouseEvent &event) {
    if (juce::ModifierKeys::currentModifiers.isAltDown()) {
        dragStartBounds = getBounds();
        ape->setVisible(false);
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
track::track *track::PluginEditorWindow::getCorrespondingTrack() {
    return &processor->tracks[(size_t)trackIndex];
}

std::unique_ptr<juce::AudioPluginInstance> *
track::PluginEditorWindow::getPlugin() {
    return &getCorrespondingTrack()->plugins[(size_t)this->pluginIndex];
}

// custom close button because lookandfeels are annoying
track::CloseButton::CloseButton() : juce::Component(), font() {}
track::CloseButton::~CloseButton() {}

void track::CloseButton::paint(juce::Graphics &g) {
    DBG("CLoseBUtton::paint()");

    g.setFont(font.withHeight(22.f));
    g.setColour(isHoveredOver == true ? hoveredColor : normalColor);
    g.drawText("X", 0, 0, getWidth(), getHeight(), juce::Justification::centred,
               false);
}

void track::CloseButton::mouseUp(const juce::MouseEvent &event) {
    if (!event.mods.isLeftButtonDown())
        return;

    juce::Component *componentToRemove =
        (juce::Component *)getParentComponent();
    jassert(componentToRemove != nullptr);

    juce::Component *componentToRemoveParent =
        (juce::Component *)componentToRemove->getParentComponent();
    jassert(componentToRemoveParent != nullptr);

    componentToRemoveParent->removeChildComponent(componentToRemove);
}

void track::CloseButton::mouseEnter(const juce::MouseEvent &event) {
    isHoveredOver = true;
    repaint();
}
void track::CloseButton::mouseExit(const juce::MouseEvent &event) {
    isHoveredOver = false;
    repaint();
}
