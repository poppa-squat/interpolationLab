#pragma once

#include "plugin/PluginProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

namespace interpolation_lab::plugin {

class GunmetalLookAndFeel final : public juce::LookAndFeel_V4 {
  public:
    GunmetalLookAndFeel();

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override;
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour, bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;
    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                        bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    void drawLabel(juce::Graphics& g, juce::Label& label) override;
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown, int buttonX,
                      int buttonY, int buttonW, int buttonH, juce::ComboBox& box) override;
    auto getLabelFont(juce::Label& label) -> juce::Font override;
    auto getTextButtonFont(juce::TextButton& button, int buttonHeight) -> juce::Font override;
    auto getComboBoxFont(juce::ComboBox& box) -> juce::Font override;
};

class InterpolationLabEditor final : public juce::AudioProcessorEditor {
  public:
    explicit InterpolationLabEditor(InterpolationLabProcessor& processor);
    ~InterpolationLabEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

  private:
    struct Layout {
        juce::Rectangle<int> title;
        juce::Rectangle<int> mixCaption;
        juce::Rectangle<int> source;
        juce::Rectangle<int> target;
        juce::Rectangle<int> mix;
        juce::Rectangle<int> interpolatorCaption;
        juce::Rectangle<int> interpolator;
    };

    [[nodiscard]] auto layoutFor(juce::Rectangle<int> bounds) const -> Layout;

    InterpolationLabProcessor& processor_;
    GunmetalLookAndFeel lookAndFeel_;
    juce::Slider mixSlider_;
    juce::ComboBox interpolatorBox_;
    juce::AudioProcessorValueTreeState::SliderAttachment mixAttachment_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> interpolatorAttachment_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InterpolationLabEditor)
};

} // namespace interpolation_lab::plugin
