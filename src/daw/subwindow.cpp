#include "subwindow.h"
#include "defs.h"
#include "plugin_chain.h"
#include "track.h"
#include "utility.h"

// custom close button because lookandfeels are annoying
track::CloseButton::CloseButton() : juce::Component(), font() {
    setOpaque(false);
}
track::CloseButton::~CloseButton() {}

void track::CloseButton::paint(juce::Graphics &g) {
    g.fillAll(juce::Colours::transparentWhite);
    g.setFont(font.withHeight(22.f));
    g.setColour(isHoveredOver == true ? hoveredColor : normalColor);
    g.drawText("X", 0, 0, getWidth(), getHeight(), juce::Justification::centred,
               false);
}

void track::CloseButton::mouseUp(const juce::MouseEvent &event) {
    if (!event.mods.isLeftButtonDown())
        return;

    // once upon a tale, here lied absolute cinema--a chunk of code where the
    // elders tell stories about to the little ones--a chunk of code where
    // modern civilization feared to tread...
    if (onClose)
        onClose();
    else {
        juce::Component *componentToRemove =
            (juce::Component *)getParentComponent();
        jassert(componentToRemove != nullptr);

        juce::Component *componentToRemoveParent =
            (juce::Component *)componentToRemove->getParentComponent();
        jassert(componentToRemoveParent != nullptr);

        componentToRemoveParent->removeChildComponent(componentToRemove);
    }
}

void track::CloseButton::mouseEnter(const juce::MouseEvent & /*event*/) {
    isHoveredOver = true;
    repaint();
}
void track::CloseButton::mouseExit(const juce::MouseEvent & /*event*/) {
    isHoveredOver = false;
    repaint();
}

void track::Subwindow::mouseDrag(const juce::MouseEvent &event) {
    if (permitMovement) {
        juce::Rectangle<int> newBounds = this->dragStartBounds;
        newBounds.setX(newBounds.getX() + event.getDistanceFromDragStartX());
        newBounds.setY(newBounds.getY() + event.getDistanceFromDragStartY());
        setBounds(newBounds);
    }
}

void track::Subwindow::mouseDown(const juce::MouseEvent &event) {
    if (event.y <
        getTitleBarBounds().getHeight() + UI_SUBWINDOW_SHADOW_SPREAD) {
        dragStartBounds = getBounds();
        permitMovement = true;
    }
    this->toFront(true);
}

void track::Subwindow::mouseUp(const juce::MouseEvent & /*event*/) {
    permitMovement = false;
}

juce::Rectangle<int> track::Subwindow::getTitleBarBounds() {
    juce::Rectangle<int> titlebarBounds =
        getLocalBounds().reduced((UI_SUBWINDOW_SHADOW_SPREAD / 2) - 1, 0);
    titlebarBounds.setHeight(UI_SUBWINDOW_TITLEBAR_HEIGHT);
    titlebarBounds.setY(UI_SUBWINDOW_SHADOW_SPREAD);
    titlebarBounds.reduce(UI_SUBWINDOW_TITLEBAR_MARGIN, 0);

    return titlebarBounds;
}

void track::Subwindow::resized() {
    int closeBtnSize = UI_SUBWINDOW_TITLEBAR_HEIGHT;
    closeBtn.setBounds(
        getWidth() - closeBtnSize - UI_SUBWINDOW_TITLEBAR_MARGIN -
            UI_SUBWINDOW_SHADOW_SPREAD,
        UI_SUBWINDOW_SHADOW_SPREAD, closeBtnSize + UI_SUBWINDOW_TITLEBAR_MARGIN,
        closeBtnSize);
}

void track::Subwindow::paint(juce::Graphics &g) {
    // bg
    juce::DropShadow shadow =
        juce::DropShadow(juce::Colours::black, 5.f, juce::Point<int>());
    shadow.drawForRectangle(
        g, getLocalBounds().reduced(UI_SUBWINDOW_SHADOW_SPREAD));

    g.setColour(juce::Colour(0xFF'282828));
    g.fillRect(getLocalBounds().reduced(UI_SUBWINDOW_SHADOW_SPREAD));

    // border
    g.setColour(juce::Colour(0xFF'4A4A4A));
    g.drawRect(getTitleBarBounds());
    g.drawRect(getLocalBounds().reduced(UI_SUBWINDOW_SHADOW_SPREAD), 2);
}
