#include "plugin/PluginProcessor.h"

#include "dsp/Weights.h"
#include "plugin/ParameterIds.h"
#include "plugin/PluginEditor.h"

#include <algorithm>

namespace interpolation_lab::plugin {

InterpolationLabProcessor::InterpolationLabProcessor()
    : AudioProcessor(createBusesProperties()),
      parameters_(*this, nullptr, "PARAMETERS", createParameterLayout()) {
    mixParameter_ = parameters_.getRawParameterValue(ids::mix);
    interpolatorParameter_ = parameters_.getRawParameterValue(ids::interpolator);
}

auto InterpolationLabProcessor::createBusesProperties() -> BusesProperties {
    // Two live buses for now (Source, Target). processBlock iterates input buses, so
    // extra sources can be added here later without changing the interpolator seam.
    return BusesProperties()
        .withInput("Source", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
        .withInput("Target", juce::AudioChannelSet::stereo(), true);
}

auto InterpolationLabProcessor::createParameterLayout()
    -> juce::AudioProcessorValueTreeState::ParameterLayout {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ids::mix, 1}, "Mix",
        juce::NormalisableRange<float>{0.0F, 1.0F, 0.0001F}, 0.0F));

    juce::StringArray interpolatorNames;
    for (int i = 0; i < InterpolatorCatalog::size(); ++i) {
        interpolatorNames.add(juce::String(InterpolatorCatalog::info(i).name.data(),
                                           InterpolatorCatalog::info(i).name.size()));
    }
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ids::interpolator, 1}, "Interpolator", interpolatorNames, 0));
    return {params.begin(), params.end()};
}

void InterpolationLabProcessor::allocateSourceBuffers(const int numSources, const int numChannels,
                                                      const int maxBlockSize) {
    sourceCopies_.assign(static_cast<std::size_t>(numSources), juce::AudioBuffer<float>{});
    sourcePtrs_.assign(static_cast<std::size_t>(numSources), {});
    sourceViews_.assign(static_cast<std::size_t>(numSources), {});
    weights_.assign(static_cast<std::size_t>(numSources), 0.0F);

    for (int source = 0; source < numSources; ++source) {
        sourceCopies_[static_cast<std::size_t>(source)].setSize(numChannels, maxBlockSize);
        sourcePtrs_[static_cast<std::size_t>(source)].assign(static_cast<std::size_t>(numChannels),
                                                             nullptr);
    }
    outputCopy_.setSize(numChannels, maxBlockSize);
    outputPtrs_.assign(static_cast<std::size_t>(numChannels), nullptr);
}

void InterpolationLabProcessor::prepareToPlay(const double sampleRate, const int samplesPerBlock) {
    const auto channels = std::max(getTotalNumInputChannels(), getTotalNumOutputChannels());
    const auto numSources = std::max(1, getBusCount(true));
    spec_ = ProcessSpec{sampleRate, samplesPerBlock, std::max(1, channels), numSources};
    allocateSourceBuffers(spec_.maxSources, spec_.numChannels, spec_.maxBlockSize);
    currentInterpolator_ = -1;
    updateInterpolator(static_cast<int>(interpolatorParameter_->load()));
}

void InterpolationLabProcessor::releaseResources() {
    if (interpolator_ != nullptr) {
        interpolator_->reset();
    }
}

auto InterpolationLabProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const -> bool {
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    if (input != output) {
        return false;
    }
    if (input != juce::AudioChannelSet::mono() && input != juce::AudioChannelSet::stereo()) {
        return false;
    }

    for (int bus = 1; bus < layouts.inputBuses.size(); ++bus) {
        const auto source = layouts.getChannelSet(true, bus);
        if (!(source.isDisabled() || source == juce::AudioChannelSet::mono() ||
              source == juce::AudioChannelSet::stereo())) {
            return false;
        }
    }
    return true;
}

void InterpolationLabProcessor::updateInterpolator(const int interpolatorIndex) {
    const auto index = std::clamp(interpolatorIndex, 0, std::max(0, InterpolatorCatalog::size() - 1));
    if (index == currentInterpolator_ && interpolator_ != nullptr) {
        return;
    }
    currentInterpolator_ = index;
    interpolator_ = InterpolatorCatalog::create(index);
    interpolator_->prepare(spec_);
    interpolator_->reset();
    setLatencySamples(interpolator_->latencySamples());
}

void InterpolationLabProcessor::copyInputBus(juce::AudioBuffer<float>& buffer, const int busIndex,
                                             const int samples, const int channelsToFill) {
    auto& copy = sourceCopies_[static_cast<std::size_t>(busIndex)];
    copy.clear();

    const auto hasBus = busIndex < getBusCount(true) &&
                        getBus(true, busIndex) != nullptr &&
                        !getBus(true, busIndex)->getCurrentLayout().isDisabled();
    if (!hasBus || samples <= 0) {
        return;
    }

    auto input = getBusBuffer(buffer, true, busIndex);
    for (int channel = 0; channel < channelsToFill; ++channel) {
        if (channel < input.getNumChannels()) {
            copy.copyFrom(channel, 0, input, channel, 0, samples);
        } else if (input.getNumChannels() > 0) {
            copy.copyFrom(channel, 0, input, input.getNumChannels() - 1, 0, samples);
        }
    }
}

void InterpolationLabProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midi) {
    juce::ignoreUnused(midi);
    juce::ScopedNoDenormals noDenormals;

    const auto interpolatorIndex = static_cast<int>(interpolatorParameter_->load());
    updateInterpolator(interpolatorIndex);
    if (interpolator_ == nullptr || sourceCopies_.empty()) {
        buffer.clear();
        return;
    }

    const auto samples = std::min(buffer.getNumSamples(), outputCopy_.getNumSamples());
    const auto outputChannels = getTotalNumOutputChannels();
    if (samples <= 0 || outputChannels <= 0) {
        return;
    }

    const auto channelsToFill = std::min(outputChannels, outputCopy_.getNumChannels());
    const auto numSources = static_cast<int>(sourceCopies_.size());
    for (int source = 0; source < numSources; ++source) {
        copyInputBus(buffer, source, samples, channelsToFill);
        auto& ptrs = sourcePtrs_[static_cast<std::size_t>(source)];
        for (int channel = 0; channel < channelsToFill; ++channel) {
            ptrs[static_cast<std::size_t>(channel)] =
                sourceCopies_[static_cast<std::size_t>(source)].getReadPointer(channel);
        }
        sourceViews_[static_cast<std::size_t>(source)] =
            ConstAudioView{ptrs.data(), channelsToFill, samples};
    }

    std::fill(weights_.begin(), weights_.end(), 0.0F);
    if (numSources == 1) {
        weights_[0] = 1.0F;
    } else {
        twoSourceWeights(mixParameter_->load(), weights_[0], weights_[1]);
    }

    for (int channel = 0; channel < channelsToFill; ++channel) {
        outputPtrs_[static_cast<std::size_t>(channel)] = outputCopy_.getWritePointer(channel);
    }

    interpolator_->process(sourceViews_, weights_,
                           AudioView{outputPtrs_.data(), channelsToFill, samples});

    auto output = getBusBuffer(buffer, false, 0);
    const auto copyChannels = std::min(output.getNumChannels(), channelsToFill);
    for (int channel = 0; channel < copyChannels; ++channel) {
        output.copyFrom(channel, 0, outputCopy_, channel, 0, samples);
    }
    for (int channel = copyChannels; channel < output.getNumChannels(); ++channel) {
        output.clear(channel, 0, samples);
    }
}

auto InterpolationLabProcessor::createEditor() -> juce::AudioProcessorEditor* {
    return new InterpolationLabEditor(*this);
}

auto InterpolationLabProcessor::hasEditor() const -> bool { return true; }

auto InterpolationLabProcessor::getName() const -> const juce::String { return JucePlugin_Name; }

auto InterpolationLabProcessor::acceptsMidi() const -> bool { return false; }

auto InterpolationLabProcessor::producesMidi() const -> bool { return false; }

auto InterpolationLabProcessor::isMidiEffect() const -> bool { return false; }

auto InterpolationLabProcessor::getTailLengthSeconds() const -> double {
    if (interpolator_ == nullptr || spec_.sampleRate <= 0.0) {
        return 0.0;
    }
    return static_cast<double>(interpolator_->latencySamples()) / spec_.sampleRate;
}

auto InterpolationLabProcessor::getNumPrograms() -> int { return 1; }

auto InterpolationLabProcessor::getCurrentProgram() -> int { return 0; }

void InterpolationLabProcessor::setCurrentProgram(int) {}

auto InterpolationLabProcessor::getProgramName(int) -> const juce::String { return {}; }

void InterpolationLabProcessor::changeProgramName(int, const juce::String&) {}

void InterpolationLabProcessor::getStateInformation(juce::MemoryBlock& destination) {
    if (auto xml = parameters_.copyState().createXml()) {
        copyXmlToBinary(*xml, destination);
    }
}

void InterpolationLabProcessor::setStateInformation(const void* data, const int sizeInBytes) {
    if (auto xml = getXmlFromBinary(data, sizeInBytes)) {
        parameters_.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

} // namespace interpolation_lab::plugin

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new interpolation_lab::plugin::InterpolationLabProcessor();
}
