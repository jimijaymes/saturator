#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
class SaturatorProcessor final : public juce::AudioProcessor {
public:
 SaturatorProcessor();
 void prepareToPlay(double,int) override;
 void releaseResources() override {}
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 juce::AudioProcessorEditor* createEditor() override;
 bool hasEditor() const override {return true;}
 const juce::String getName() const override {return "Saturator 0.1";}
 bool acceptsMidi() const override {return false;}
 bool producesMidi() const override {return false;}
 bool isMidiEffect() const override {return false;}
 double getTailLengthSeconds() const override {return 0;}
 int getNumPrograms() override {return 1;}
 int getCurrentProgram() override {return 0;}
 void setCurrentProgram(int) override {}
 const juce::String getProgramName(int) override {return {};}
 void changeProgramName(int,const juce::String&) override {}
 void getStateInformation(juce::MemoryBlock&) override;
 void setStateInformation(const void*,int) override;
 juce::AudioProcessorValueTreeState parameters;
 std::atomic<float> inputMeter{-100},outputMeter{-100},gainReduction{0};
private:
 static juce::AudioProcessorValueTreeState::ParameterLayout layout();
 struct Filter {float lp=0,hp=0; float process(float x,float hpHz,float lpHz,float sr);};
 struct State {Filter pre,post;float flux=0,magnet=0,tapePrev=0,tapeLow=0,tapeHigh=0,coreLow=0,coreHigh=0,circuitCap=0,circuitBias=0;};
 std::array<State,2> states{};
 std::array<std::unique_ptr<juce::dsp::Oversampling<float>>,4> oversamplers;
 juce::AudioBuffer<float> scratch;
 double sr=44100;
 int maxBlock=512;
 float stage(float,int,int,float,float,float,float);
 void processChunk(juce::AudioBuffer<float>&,int,int,int);
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SaturatorProcessor)
};
