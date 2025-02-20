#include "lookandfeel.h"

const juce::Font track::ui::CustomLookAndFeel::getRobotoMonoThin() {
    static auto typeface = Typeface::createSystemTypefaceFor(
        BinaryData::RobotoMonoThin_ttf, BinaryData::RobotoMonoThin_ttfSize);

    return Font(typeface);
}