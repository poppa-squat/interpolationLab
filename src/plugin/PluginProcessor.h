#pragma once

#include "dsp/Interpolator.h"
#include "dsp/InterpolatorCatalog.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>
#include <memory>
#include <vector>

namespace interpolation_lab::plugin {

class InterpolationLabProcessor final : public juce::AudioProcessor {
  public:
    InterpolationLabProcessor();
    ~InterpolationLabProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    [[nodiscard]] auto isBusesLayoutSupported(const BusesLayout& layouts) const -> bool override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;

    [[nodiscard]] auto createEditor() -> juce::AudioProcessorEditor* override;
    [[nodiscard]] auto hasEditor() const -> bool override;
    [[nodiscard]] auto getName() const -> const juce::String override;
    [[nodiscard]] auto acceptsMidi() const -> bool override;
    [[nodiscard]] auto producesMidi() const -> bool override;
    [[nodiscard]] auto isMidiEffect() const -> bool override;
    [[nodiscard]] auto getTailLengthSeconds() const -> double override;
    [[nodiscard]] auto getNumPrograms() -> int override;
    [[nodiscard]] auto getCurrentProgram() -> int override;
    void setCurrentProgram(int index) override;
    [[nodiscard]] auto getProgramName(int index) -> const juce::String override;
    void changeProgramName(int index, const juce::String& name) override;
    void getStateInformation(juce::MemoryBlock& destination) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    [[nodiscard]] auto parameters() -> juce::AudioProcessorValueTreeState& { return parameters_; }

  private:
    static auto createParameterLayout() -> juce::AudioProcessorValueTreeState::ParameterLayout;
    static auto createBusesProperties() -> BusesProperties;
    void updateInterpolator(int interpolatorIndex);
    void allocateSourceBuffers(int numSources, int numChannels, int maxBlockSize);
    void copyInputBus(juce::AudioBuffer<float>& buffer, int busIndex, int samples,
                      int channelsToFill);

    juce::AudioProcessorValueTreeState parameters_;
    std::atomic<float>* mixParameter_{};
    std::atomic<float>* interpolatorParameter_{};
    ProcessSpec spec_{};
    std::unique_ptr<Interpolator> interpolator_;
    int currentInterpolator_{-1};
    std::vector<juce::AudioBuffer<float>> sourceCopies_;
    juce::AudioBuffer<float> outputCopy_;
    std::vector<std::vector<const float*>> sourcePtrs_;
    std::vector<float*> outputPtrs_;
    std::vector<ConstAudioView> sourceViews_;
    std::vector<float> weights_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InterpolationLabProcessor)
};

} // namespace interpolation_lab::plugin
