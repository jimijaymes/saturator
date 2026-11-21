#pragma once
#include "PluginProcessor.h"
class SaturatorEditor final : public juce::AudioProcessorEditor,private juce::Timer {
public:
 explicit SaturatorEditor(SaturatorProcessor&);
 void paint(juce::Graphics&) override;
 void resized() override;
private:
 void timerCallback() override;
 SaturatorProcessor& processor;
 struct Dial {
  juce::Slider control;
  juce::Label label;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
 };
 struct Choice {
  juce::ComboBox control;
  juce::Label label;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attach;
 };
 std::array<Dial,14> dials;
 std::array<Choice,7> choices;
 juce::ToggleButton preOn,postOn,wowFlutterOn;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> preAttach,postAttach,wowFlutterAttach;
 float inDb=-100,outDb=-100,trimDb=0;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SaturatorEditor)
};
