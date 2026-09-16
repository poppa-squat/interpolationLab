#include "plugin/PluginEditor.h"

#include "dsp/InterpolatorCatalog.h"
#include "plugin/ParameterIds.h"

namespace interpolation_lab::plugin {
namespace {

constexpr auto kBackground = juce::uint32{0xff1a1d21};
constexpr auto kPanel = juce::uint32{0xff2b3036};
constexpr auto kMetalLight = juce::uint32{0xff6a727a};
constexpr auto kMetalDark = juce::uint32{0xff121416};
constexpr auto kNeon = juce::uint32{0xff39ff14};
constexpr auto kSilver = juce::uint32{0xffd5dbdf};
constexpr auto kMuted = juce::uint32{0xff8a9299};

auto titleFont() -> juce::Font {
    return juce::Font{juce::FontOptions(17.0f)}.withExtraKerningFactor(0.16f).boldened();
}

auto captionFont() -> juce::Font {
    return juce::Font{juce::FontOptions(12.0f)}.withExtraKerningFactor(0.18f).boldened();
}

auto endpointFont() -> juce::Font { return juce::Font{juce::FontOptions(13.0f)}.boldened(); }

} // namespace

GunmetalLookAndFeel::GunmetalLookAndFeel() {
    setColour(juce::Slider::textBoxTextColourId, juce::Colour(kNeon));
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(kMetalDark));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(kNeon).withAlpha(0.35f));
    setColour(juce::CaretComponent::caretColourId, juce::Colour(kNeon));
    setColour(juce::TextEditor::textColourId, juce::Colour(kNeon));
    setColour(juce::TextEditor::backgroundColourId, juce::Colour(kMetalDark));
    setColour(juce::TextEditor::outlineColourId, juce::Colour(kNeon).withAlpha(0.45f));
    setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(kNeon));
    setColour(juce::TextEditor::highlightedTextColourId, juce::Colour(kBackground));
    setColour(juce::TextEditor::highlightColourId, juce::Colour(kNeon).withAlpha(0.55f));
    setColour(juce::Label::textColourId, juce::Colour(kSilver));
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff22262b));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(kNeon).withAlpha(0.45f));
    setColour(juce::ComboBox::arrowColourId, juce::Colour(kNeon));
    setColour(juce::ComboBox::textColourId, juce::Colour(kSilver));
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(kMetalDark));
    setColour(juce::PopupMenu::textColourId, juce::Colour(kSilver));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(kNeon).withAlpha(0.22f));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colour(kNeon));
}

void GunmetalLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float rotaryStartAngle,
                                           float rotaryEndAngle, juce::Slider&) {
    const auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                               static_cast<float>(width), static_cast<float>(height));
    const auto centre = bounds.getCentre();
    const auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const auto tickRadius = radius * 0.98f;
    const auto arcRadius = radius * 0.86f;
    const auto knobRadius = radius * 0.7f;
    const auto neon = juce::Colour(kNeon);

    for (int i = 0; i < 11; ++i) {
        const auto t = static_cast<float>(i) / 10.0f;
        const auto angle = rotaryStartAngle + t * (rotaryEndAngle - rotaryStartAngle);
        const auto major = (i % 5 == 0);
        juce::Path tick;
        tick.addRoundedRectangle(-0.7f, -tickRadius, major ? 1.6f : 1.1f, major ? 8.0f : 4.5f, 0.6f);
        tick.applyTransform(juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));
        g.setColour(major ? juce::Colour(kSilver) : juce::Colour(kMuted));
        g.fillPath(tick);
    }

    juce::Path backgroundArc;
    backgroundArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle,
                                rotaryEndAngle, true);
    g.setColour(juce::Colour(kMetalDark));
    g.strokePath(backgroundArc, juce::PathStrokeType(5.5f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));

    if (sliderPos > 0.0f) {
        juce::Path valueArc;
        valueArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle,
                               toAngle, true);
        g.setColour(neon.withAlpha(0.2f));
        g.strokePath(valueArc, juce::PathStrokeType(11.0f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
        g.setColour(neon);
        g.strokePath(valueArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    }

    juce::ColourGradient metal(juce::Colour(kMetalLight), centre.x - knobRadius * 0.45f,
                               centre.y - knobRadius * 0.55f, juce::Colour(kMetalDark),
                               centre.x + knobRadius * 0.38f, centre.y + knobRadius * 0.5f, true);
    g.setGradientFill(metal);
    g.fillEllipse(centre.x - knobRadius, centre.y - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);

    g.setColour(juce::Colour(0x66ffffff));
    g.drawEllipse(centre.x - knobRadius * 0.62f, centre.y - knobRadius * 0.7f, knobRadius * 0.9f,
                  knobRadius * 0.42f, 1.0f);

    g.setColour(juce::Colour(0xff0d0f11));
    g.drawEllipse(centre.x - knobRadius, centre.y - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f,
                  1.6f);

    const auto innerR = knobRadius * 0.8f;
    g.setColour(juce::Colour(0x55000000));
    g.drawEllipse(centre.x - innerR, centre.y - innerR, innerR * 2.0f, innerR * 2.0f, 1.0f);

    const auto capR = knobRadius * 0.16f;
    g.setColour(juce::Colour(kPanel));
    g.fillEllipse(centre.x - capR, centre.y - capR, capR * 2.0f, capR * 2.0f);
    g.setColour(neon);
    g.fillEllipse(centre.x - capR * 0.38f, centre.y - capR * 0.38f, capR * 0.76f, capR * 0.76f);

    juce::Path pointer;
    pointer.addRoundedRectangle(-1.15f, -knobRadius * 0.9f, 2.3f, knobRadius * 0.58f, 1.0f);
    pointer.applyTransform(juce::AffineTransform::rotation(toAngle).translated(centre.x, centre.y));
    g.setColour(neon);
    g.fillPath(pointer);
}

void GunmetalLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                               const juce::Colour& backgroundColour,
                                               bool shouldDrawButtonAsHighlighted,
                                               bool shouldDrawButtonAsDown) {
    juce::ignoreUnused(backgroundColour);
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    const auto neon = juce::Colour(kNeon);
    const auto selected = button.getToggleState();

    if (selected) {
        g.setColour(neon.withAlpha(0.16f));
        g.fillRoundedRectangle(bounds, 7.0f);
        g.setColour(neon);
        g.drawRoundedRectangle(bounds, 7.0f, 1.4f);
    } else {
        g.setColour(juce::Colour(0xff22262b));
        g.fillRoundedRectangle(bounds, 7.0f);
        g.setColour(shouldDrawButtonAsHighlighted ? neon.withAlpha(0.55f) : juce::Colour(0xff4a5158));
        g.drawRoundedRectangle(bounds, 7.0f, 1.0f);
    }

    if (shouldDrawButtonAsDown) {
        g.setColour(juce::Colours::black.withAlpha(0.22f));
        g.fillRoundedRectangle(bounds, 7.0f);
    }
}

void GunmetalLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                         bool shouldDrawButtonAsHighlighted,
                                         bool shouldDrawButtonAsDown) {
    juce::ignoreUnused(shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
    g.setFont(getTextButtonFont(button, button.getHeight()));
    g.setColour(button.getToggleState() ? juce::Colour(kNeon) : juce::Colour(kSilver));
    g.drawText(button.getButtonText(), button.getLocalBounds().reduced(8, 2),
               juce::Justification::centred);
}

void GunmetalLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label) {
    if (dynamic_cast<juce::Slider*>(label.getParentComponent()) == nullptr) {
        g.setColour(label.findColour(juce::Label::textColourId));
        g.setFont(getLabelFont(label));
        g.drawText(label.getText(), label.getLocalBounds(), label.getJustificationType(), true);
        return;
    }

    auto bounds = label.getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(juce::Colour(kMetalDark));
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(juce::Colour(kNeon).withAlpha(0.4f));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    if (!label.isBeingEdited()) {
        g.setColour(juce::Colour(kNeon));
        g.setFont(getLabelFont(label));
        g.drawText(label.getText(), label.getLocalBounds(), juce::Justification::centred, true);
    }
}

void GunmetalLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                                       int buttonX, int buttonY, int buttonW, int buttonH,
                                       juce::ComboBox& box) {
    juce::ignoreUnused(isButtonDown, buttonX, buttonY, buttonW, buttonH);
    auto bounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width),
                                         static_cast<float>(height)).reduced(0.5f);
    const auto neon = juce::Colour(kNeon);
    g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(bounds, 7.0f);
    g.setColour(box.hasKeyboardFocus(true) ? neon : box.findColour(juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle(bounds, 7.0f, 1.2f);

    const auto arrowZone = bounds.removeFromRight(22.0f).reduced(6.0f, 10.0f);
    juce::Path arrow;
    arrow.addTriangle(arrowZone.getX(), arrowZone.getY(), arrowZone.getRight(), arrowZone.getY(),
                      arrowZone.getCentreX(), arrowZone.getBottom());
    g.setColour(neon);
    g.fillPath(arrow);
}

auto GunmetalLookAndFeel::getLabelFont(juce::Label&) -> juce::Font {
    return juce::Font{juce::FontOptions(13.0f)};
}

auto GunmetalLookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight) -> juce::Font {
    return juce::Font{juce::FontOptions(static_cast<float>(juce::jmin(14, buttonHeight - 10)))};
}

auto GunmetalLookAndFeel::getComboBoxFont(juce::ComboBox&) -> juce::Font {
    return juce::Font{juce::FontOptions(14.0f)};
}

InterpolationLabEditor::InterpolationLabEditor(InterpolationLabProcessor& processor)
    : juce::AudioProcessorEditor(processor),
      processor_(processor),
      mixAttachment_(processor.parameters(), ids::mix, mixSlider_) {
    setLookAndFeel(&lookAndFeel_);

    mixSlider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 22);
    mixSlider_.setNumDecimalPlacesToDisplay(2);
    mixSlider_.setRotaryParameters(juce::degreesToRadians(225.0f), juce::degreesToRadians(495.0f),
                                   true);
    mixSlider_.setPopupDisplayEnabled(false, false, nullptr);
    addAndMakeVisible(mixSlider_);

    interpolatorBox_.setJustificationType(juce::Justification::centred);
    interpolatorBox_.setTextWhenNothingSelected("Interpolator");
    interpolatorBox_.setTooltip("Interpolation technique");
    for (int i = 0; i < InterpolatorCatalog::size(); ++i) {
        interpolatorBox_.addItem(juce::String(InterpolatorCatalog::info(i).name.data(),
                                              InterpolatorCatalog::info(i).name.size()),
                                 i + 1);
    }
    addAndMakeVisible(interpolatorBox_);
    interpolatorAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.parameters(), ids::interpolator, interpolatorBox_);

    setSize(480, 320);
    setResizable(false, false);
}

InterpolationLabEditor::~InterpolationLabEditor() { setLookAndFeel(nullptr); }

void InterpolationLabEditor::paint(juce::Graphics& g) {
    const auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient background(juce::Colour(0xff24282d), 0.0f, 0.0f, juce::Colour(kBackground),
                                    0.0f, bounds.getHeight(), false);
    g.setGradientFill(background);
    g.fillRect(bounds);

    auto panel = bounds.reduced(10.0f);
    g.setColour(juce::Colour(kPanel));
    g.fillRoundedRectangle(panel, 12.0f);
    g.setColour(juce::Colour(kNeon).withAlpha(0.22f));
    g.drawRoundedRectangle(panel, 12.0f, 1.0f);

    const auto layout = layoutFor(getLocalBounds());
    g.setColour(juce::Colour(kNeon));
    g.setFont(titleFont());
    g.drawText("INTERPOLATION LAB", layout.title, juce::Justification::centred);

    auto underline = layout.title.toFloat().reduced(118.0f, 0.0f);
    underline.setY(static_cast<float>(layout.title.getBottom()) - 4.0f);
    underline.setHeight(1.2f);
    g.setColour(juce::Colour(kNeon).withAlpha(0.7f));
    g.fillRect(underline);

    g.setColour(juce::Colour(kNeon));
    g.setFont(captionFont());
    g.drawText("MIX", layout.mixCaption, juce::Justification::centred);
    g.drawText("INTERPOLATOR", layout.interpolatorCaption, juce::Justification::centred);

    g.setColour(juce::Colour(kSilver));
    g.setFont(endpointFont());
    g.drawText("Source", layout.source, juce::Justification::centred);
    g.drawText("Target", layout.target, juce::Justification::centred);
}

void InterpolationLabEditor::resized() {
    const auto layout = layoutFor(getLocalBounds());
    mixSlider_.setBounds(layout.mix);
    interpolatorBox_.setBounds(layout.interpolator);
}

auto InterpolationLabEditor::layoutFor(juce::Rectangle<int> bounds) const -> Layout {
    Layout layout;
    auto area = bounds.reduced(22);
    layout.title = area.removeFromTop(30);
    layout.mixCaption = area.removeFromTop(18);
    auto interpolatorRow = area.removeFromBottom(58);
    layout.interpolatorCaption = interpolatorRow.removeFromTop(16);
    layout.interpolator = interpolatorRow.reduced(36, 4);
    layout.source = area.removeFromLeft(78);
    layout.target = area.removeFromRight(78);
    layout.mix = area;
    return layout;
}

} // namespace interpolation_lab::plugin
