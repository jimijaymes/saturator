#pragma once
#include "PluginProcessor.h"
#include <vector>
class SaturatorEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
 explicit SaturatorEditor(SaturatorProcessor&);
 void paint(juce::Graphics&) override;
 void resized() override;
private:
 void timerCallback() override;
 struct Dial {juce::Slider control;juce::Label label;std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;};
 struct Choice {juce::ComboBox control;juce::Label label;std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attach;};
 SaturatorProcessor& processor;
 std::array<Dial,12> dials;
 std::array<Choice,6> choices;
 juce::ToggleButton preOn,postOn;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> preAttach,postAttach;
 float inDb=-100,outDb=-100,trimDb=0;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SaturatorEditor)
};
