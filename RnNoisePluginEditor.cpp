#include "RnNoisePluginEditor.h"

namespace {
const juce::Colour ink(0xff20232a);
const juce::Colour orange(0xffff8a00);
const juce::Colour red(0xffe53935);
const juce::Colour pale(0xfff2f3f5);
const juce::Colour muted(0xff737985);

void setupKnob(juce::Slider& slider) {
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 22);
    slider.setColour(juce::Slider::rotarySliderFillColourId, orange);
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xffd6d9de));
    slider.setColour(juce::Slider::thumbColourId, red);
    slider.setColour(juce::Slider::textBoxTextColourId, ink);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::white);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xffe1e3e7));
}

void setupLabel(juce::Label& label, const juce::String& text, float size,
                juce::Colour colour, bool bold = true,
                juce::Justification justification = juce::Justification::centredLeft) {
    label.setText(text, juce::dontSendNotification);
    label.setFont(juce::Font(size, bold ? juce::Font::bold : juce::Font::plain));
    label.setColour(juce::Label::textColourId, colour);
    label.setJustificationType(justification);
}

float toMeterWidth(float peak) {
    if (peak <= 0.001f)
        return 0.0f;
    const float db = juce::Decibels::gainToDecibels(peak, -60.0f);
    return juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 60.0f);
}
}

RnNoiseAudioProcessorEditor::RnNoiseAudioProcessorEditor(
    RnNoiseAudioProcessor& p, juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor(&p), state(vts), processor(p) {
    setSize(640, 430);

    setupLabel(brand, juce::String::fromUTF8("$𝔥𝔗"), 39.0f, ink, true);
    setupLabel(subtitle, "NOISE REDUCTION", 15.0f, orange, true);
    setupLabel(version, "V.2", 15.0f, red, true, juce::Justification::centred);
    setupLabel(thresholdLabel, "VOICE THRESHOLD", 11.0f, ink, true, juce::Justification::centred);
    setupLabel(holdLabel, "VOICE HOLD", 11.0f, ink, true, juce::Justification::centred);
    setupLabel(lookaheadLabel, "WORD START", 11.0f, ink, true, juce::Justification::centred);
    setupLabel(footer, "by $ht The Human   •   Original project by @werman", 10.5f, muted, false,
               juce::Justification::centred);

    addAndMakeVisible(brand);
    addAndMakeVisible(subtitle);
    addAndMakeVisible(version);
    addAndMakeVisible(thresholdLabel);
    addAndMakeVisible(holdLabel);
    addAndMakeVisible(lookaheadLabel);
    addAndMakeVisible(footer);

    setupKnob(threshold);
    setupKnob(hold);
    setupKnob(lookahead);
    addAndMakeVisible(threshold);
    addAndMakeVisible(hold);
    addAndMakeVisible(lookahead);

    auto* thresholdParameter = p.m_parameters.getParameter("vad_threshold");
    auto* holdParameter = p.m_parameters.getParameter("vad_grace_period");
    auto* lookaheadParameter = p.m_parameters.getParameter("vad_retroactive_grace_period");

    threshold.setName("VAD Threshold");
    hold.setName("VAD Grace Period");
    lookahead.setName("Retroactive VAD Grace Period");

    thresholdAttachment = std::make_unique<SliderAttachment>(
        state, thresholdParameter->getParameterID(), threshold);
    holdAttachment = std::make_unique<SliderAttachment>(
        state, holdParameter->getParameterID(), hold);
    lookaheadAttachment = std::make_unique<SliderAttachment>(
        state, lookaheadParameter->getParameterID(), lookahead);

    autoButton.setButtonText("AUTO SETUP");
    autoButton.setColour(juce::TextButton::buttonColourId, ink);
    autoButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    autoButton.setColour(juce::TextButton::buttonOnColourId, orange);
    autoButton.onClick = [this] {
        threshold.setValue(0.90, juce::sendNotification);
        hold.setValue(20, juce::sendNotification);
        lookahead.setValue(0, juce::sendNotification);
    };
    addAndMakeVisible(autoButton);

    startTimerHz(30);
}

RnNoiseAudioProcessorEditor::~RnNoiseAudioProcessorEditor() {
    stopTimer();
}

void RnNoiseAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(pale);

    const auto bounds = getLocalBounds().toFloat();
    g.setColour(juce::Colours::white);
    g.fillRoundedRectangle(bounds.reduced(12.0f), 17.0f);

    g.setColour(orange);
    g.fillRoundedRectangle(juce::Rectangle<float>(12.0f, 12.0f, getWidth() - 24.0f, 6.0f), 3.0f);
    g.setColour(red);
    g.fillRoundedRectangle(juce::Rectangle<float>(12.0f, 12.0f, 112.0f, 6.0f), 3.0f);

    // Real-time input/output peak meters, fed by processBlock().
    g.setColour(juce::Colour(0xfff5f6f8));
    g.fillRoundedRectangle(juce::Rectangle<float>(30.0f, 112.0f, getWidth() - 60.0f, 82.0f), 10.0f);

    const float meterX = 126.0f;
    const float meterWidth = static_cast<float>(getWidth()) - meterX - 52.0f;
    const float meterHeight = 13.0f;
    const float inputWidth = meterWidth * toMeterWidth(inputMeter);
    const float outputWidth = meterWidth * toMeterWidth(outputMeter);

    g.setColour(ink);
    g.setFont(juce::Font(10.5f, juce::Font::bold));
    g.drawText("INPUT", 43, 126, 70, 16, juce::Justification::centredLeft);
    g.drawText("OUTPUT", 43, 158, 70, 16, juce::Justification::centredLeft);

    g.setColour(juce::Colour(0xffdfe2e7));
    g.fillRoundedRectangle(juce::Rectangle<float>(meterX, 127.0f, meterWidth, meterHeight), 5.0f);
    g.fillRoundedRectangle(juce::Rectangle<float>(meterX, 159.0f, meterWidth, meterHeight), 5.0f);

    g.setColour(orange);
    if (inputWidth > 0.0f)
        g.fillRoundedRectangle(juce::Rectangle<float>(meterX, 127.0f, inputWidth, meterHeight), 5.0f);
    g.setColour(red);
    if (outputWidth > 0.0f)
        g.fillRoundedRectangle(juce::Rectangle<float>(meterX, 159.0f, outputWidth, meterHeight), 5.0f);

    g.setColour(muted);
    g.setFont(juce::Font(9.0f));
    g.drawText("LIVE PEAK  •  -60 dB TO 0 dB", 34, 180, getWidth() - 68, 12,
               juce::Justification::centredRight);

    g.setColour(juce::Colour(0xffe8eaee));
    g.drawRoundedRectangle(juce::Rectangle<float>(30.0f, 211.0f, getWidth() - 60.0f, 142.0f), 12.0f, 1.0f);

    g.setColour(juce::Colour(0xfffff2e3));
    g.fillRoundedRectangle(juce::Rectangle<float>(30.0f, 365.0f, getWidth() - 60.0f, 34.0f), 8.0f);
    g.setColour(orange);
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.drawText("RNNOISE ENGINE  •  REAL-TIME VOICE CLEANUP", 40, 374, getWidth() - 80, 16,
               juce::Justification::centred);
}

void RnNoiseAudioProcessorEditor::resized() {
    brand.setBounds(34, 25, 180, 48);
    subtitle.setBounds(38, 72, 220, 22);
    version.setBounds(getWidth() - 88, 32, 48, 28);

    thresholdLabel.setBounds(42, 214, 170, 20);
    holdLabel.setBounds(235, 214, 170, 20);
    lookaheadLabel.setBounds(428, 214, 170, 20);

    threshold.setBounds(62, 235, 130, 112);
    hold.setBounds(255, 235, 130, 112);
    lookahead.setBounds(448, 235, 130, 112);

    autoButton.setBounds(getWidth() / 2 - 72, 402, 144, 22);
    footer.setBounds(30, getHeight() - 25, getWidth() - 60, 18);
}

void RnNoiseAudioProcessorEditor::timerCallback() {
    const float inputNow = processor.inputLevel.load(std::memory_order_relaxed);
    const float outputNow = processor.outputLevel.load(std::memory_order_relaxed);

    // Peak-hold style falloff keeps the meter readable between audio blocks.
    inputMeter = juce::jmax(inputNow, inputMeter * 0.88f);
    outputMeter = juce::jmax(outputNow, outputMeter * 0.88f);
    repaint();
}
