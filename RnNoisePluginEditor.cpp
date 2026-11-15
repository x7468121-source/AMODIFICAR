#include "RnNoisePluginEditor.h"

namespace {
const juce::Colour ink(0xff20232a), orange(0xffff8a00), red(0xffe53935), pale(0xfff4f5f7);
void setupKnob(juce::Slider& s) {
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 26);
    s.setColour(juce::Slider::rotarySliderFillColourId, orange);
    s.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xffd6d9de));
    s.setColour(juce::Slider::thumbColourId, red);
    s.setColour(juce::Slider::textBoxTextColourId, ink);
    s.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::white);
    s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xffe1e3e7));
}
void setupLabel(juce::Label& l, const juce::String& text, float size, juce::Colour c, bool bold=true) {
    l.setText(text, juce::dontSendNotification);
    l.setFont(juce::Font(size, bold ? juce::Font::bold : juce::Font::plain));
    l.setColour(juce::Label::textColourId, c);
    l.setJustificationType(juce::Justification::centred);
}
}

RnNoiseAudioProcessorEditor::RnNoiseAudioProcessorEditor(RnNoiseAudioProcessor& p,
    juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor(&p), state(vts), processor(p) {
    setSize(900, 570);
    setupLabel(title, "NOISE SHIELD", 34.0f, ink);
    setupLabel(subtitle, "REAL-TIME VOICE NOISE SUPPRESSION", 13.0f, juce::Colour(0xff626873), false);
    setupLabel(thresholdLabel, "VOICE THRESHOLD", 16.0f, ink);
    setupLabel(holdLabel, "VOICE HOLD / RELEASE", 16.0f, ink);
    setupLabel(lookaheadLabel, "WORD START PROTECTION", 16.0f, ink);
    setupLabel(footer, "RNNoise engine  •  48 kHz required  •  GPL-3.0", 12.0f, juce::Colour(0xff737985), false);
    addAndMakeVisible(title);
    addAndMakeVisible(subtitle);
    addAndMakeVisible(thresholdLabel);
    addAndMakeVisible(holdLabel);
    addAndMakeVisible(lookaheadLabel);
    addAndMakeVisible(footer);

    setupKnob(threshold); setupKnob(hold); setupKnob(lookahead);
    addAndMakeVisible(threshold); addAndMakeVisible(hold); addAndMakeVisible(lookahead);

    auto* t = p.m_parameters.getParameter("vad_threshold");
    auto* h = p.m_parameters.getParameter("vad_grace_period");
    auto* r = p.m_parameters.getParameter("vad_retroactive_grace_period");
    threshold.setName("VAD Threshold");
    hold.setName("VAD Grace Period");
    lookahead.setName("Retroactive VAD Grace Period");
    thresholdAttachment = std::make_unique<SliderAttachment>(state, t->getParameterID(), threshold);
    holdAttachment = std::make_unique<SliderAttachment>(state, h->getParameterID(), hold);
    lookaheadAttachment = std::make_unique<SliderAttachment>(state, r->getParameterID(), lookahead);

    autoButton.setButtonText("AUTO SETUP");
    autoButton.setColour(juce::TextButton::buttonColourId, ink);
    autoButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    autoButton.onClick = [this] {
        threshold.setValue(0.90, juce::sendNotification);
        hold.setValue(20, juce::sendNotification);
        lookahead.setValue(0, juce::sendNotification);
    };
    addAndMakeVisible(autoButton);
}
RnNoiseAudioProcessorEditor::~RnNoiseAudioProcessorEditor() = default;

void RnNoiseAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(pale);
    auto b = getLocalBounds().toFloat();
    g.setColour(juce::Colours::white);
    g.fillRoundedRectangle(b.reduced(14), 20.0f);
    g.setColour(orange);
    g.fillRoundedRectangle(juce::Rectangle<float>(14,14,(float)getWidth()-28,8), 4.0f);
    g.setColour(red);
    g.fillRoundedRectangle(juce::Rectangle<float>(14,14,170,8), 4.0f);

    g.setColour(juce::Colour(0xffe8eaee));
    g.drawRoundedRectangle(juce::Rectangle<float>(34,150,(float)getWidth()-68,300), 16.0f, 1.2f);
    g.setColour(juce::Colour(0xfffff0df));
    g.fillRoundedRectangle(juce::Rectangle<float>(48,164,(float)getWidth()-96,58), 10.0f);
    g.setColour(orange);
    g.setFont(juce::Font(15.0f, juce::Font::bold));
    g.drawText("CLEANER VOICE. LESS BACKGROUND NOISE.", 60, 177, getWidth()-120, 28, juce::Justification::centred);
    g.setColour(juce::Colour(0xff777d86));
    g.setFont(juce::Font(12.0f));
    g.drawText("Controls below map directly to the original RNNoise parameters.", 40, 460, getWidth()-80, 22, juce::Justification::centred);
}
void RnNoiseAudioProcessorEditor::resized() {
    auto area = getLocalBounds().reduced(34);
    title.setBounds(area.removeFromTop(58));
    subtitle.setBounds(area.removeFromTop(28));
    area.removeFromTop(120);
    auto row = area.removeFromTop(245);
    const int col = row.getWidth()/3;
    thresholdLabel.setBounds(row.removeFromLeft(col).removeFromTop(28));
    holdLabel.setBounds(row.removeFromLeft(col).removeFromTop(28));
    lookaheadLabel.setBounds(row.removeFromLeft(col).removeFromTop(28));
    auto knobArea = getLocalBounds().withTrimmedTop(265).withTrimmedBottom(190).reduced(72,0);
    const int w = knobArea.getWidth()/3;
    threshold.setBounds(knobArea.removeFromLeft(w).reduced(20,0));
    hold.setBounds(knobArea.removeFromLeft(w).reduced(20,0));
    lookahead.setBounds(knobArea.removeFromLeft(w).reduced(20,0));
    autoButton.setBounds(getWidth()/2-90, getHeight()-105, 180, 42);
    footer.setBounds(30, getHeight()-48, getWidth()-60, 24);
}
void RnNoiseAudioProcessorEditor::timerCallback() {}
