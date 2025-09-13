#include "subwindow.h"
#include "defs.h"
#include "plugin_chain.h"
#include "track.h"

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

void track::Subwindow::mouseDrag(const juce::MouseEvent &event) {
    if (juce::ModifierKeys::currentModifiers.isAltDown()) {
        juce::Rectangle<int> newBounds = this->dragStartBounds;
        newBounds.setX(newBounds.getX() + event.getDistanceFromDragStartX());
        newBounds.setY(newBounds.getY() + event.getDistanceFromDragStartY());
        setBounds(newBounds);
    }
}

void track::Subwindow::mouseDown(const juce::MouseEvent &event) {
    if (juce::ModifierKeys::currentModifiers.isAltDown())
        dragStartBounds = getBounds();

    this->toFront(true);
}

juce::Rectangle<int> track::Subwindow::getTitleBarBounds() {
    juce::Rectangle<int> titlebarBounds = getLocalBounds();
    titlebarBounds.setHeight(UI_SUBWINDOW_TITLEBAR_HEIGHT);
    titlebarBounds.reduce(UI_SUBWINDOW_TITLEBAR_MARGIN, 0);

    return titlebarBounds;
}

void track::Subwindow::resized() {
    int closeBtnSize = UI_SUBWINDOW_TITLEBAR_HEIGHT;
    closeBtn.setBounds(getWidth() - closeBtnSize - UI_SUBWINDOW_TITLEBAR_MARGIN,
                       0, closeBtnSize + UI_SUBWINDOW_TITLEBAR_MARGIN,
                       closeBtnSize);
}

void track::Subwindow::paint(juce::Graphics &g) {
    // bg
    g.fillAll(juce::Colour(0xFF'282828));

    // border
    g.setColour(juce::Colour(0xFF'4A4A4A));
    // g.drawHorizontalLine(titlebarBounds.getHeight(), 0, getWidth());
    g.fillRect(0, getTitleBarBounds().getHeight(), getWidth(), 2);
    g.drawRect(getLocalBounds(), 2);
}
