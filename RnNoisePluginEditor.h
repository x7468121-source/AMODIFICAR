#pragma once

#include "RnNoiseAudioProcessor.h"

class RnNoiseAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer {
public:
    explicit RnNoiseAudioProcessorEditor(RnNoiseAudioProcessor&, juce::AudioProcessorValueTreeState&);
    ~RnNoiseAudioProcessorEditor() override;

    void resized() override;
    void paint(juce::Graphics&) override;
    void timerCallback() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    juce::AudioProcessorValueTreeState& state;
    RnNoiseAudioProcessor& processor;

    juce::Slider threshold, hold, lookahead;
    juce::Label brand, subtitle, version, thresholdLabel, holdLabel, lookaheadLabel, footer;
    std::unique_ptr<SliderAttachment> thresholdAttachment, holdAttachment, lookaheadAttachment;
    juce::TextButton autoButton;

    float inputMeter = 0.0f;
    float outputMeter = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RnNoiseAudioProcessorEditor)
};
