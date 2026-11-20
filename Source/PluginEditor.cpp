#include "PluginEditor.h"
namespace {const juce::Colour bg(0xff151b21),panel(0xff253039),cream(0xfff3e7cf),amber(0xffd9a866),teal(0xff69c6b9);}
SaturatorEditor::SaturatorEditor(SaturatorProcessor& p):AudioProcessorEditor(p),processor(p){
 const char* ids[]={"input","output","drive","mix","preHP","preLP","postHP","postLP","character","bias","transformerDrive","tapeDrive","circuitDrive"};
 const char* labels[]={"INPUT","OUTPUT","DRIVE","MIX","PRE HP","PRE LP","POST HP","POST LP","CHARACTER","BIAS","XFMR DRIVE","TAPE DRIVE","CIRCUIT DRIVE"};
 for(int i=0;i<13;++i){auto& k=dials[(size_t)i];k.control.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);k.control.setTextBoxStyle(juce::Slider::TextBoxBelow,false,86,20);k.control.setColour(juce::Slider::rotarySliderFillColourId,i==2?teal:amber);k.control.setColour(juce::Slider::rotarySliderOutlineColourId,juce::Colour(0xff45515a));k.control.setColour(juce::Slider::textBoxTextColourId,cream);k.control.setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);k.control.setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::transparentBlack);addAndMakeVisible(k.control);k.label.setText(labels[i],juce::dontSendNotification);k.label.setJustificationType(juce::Justification::centred);k.label.setColour(juce::Label::textColourId,cream);addAndMakeVisible(k.label);k.attach=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters,ids[i],k.control);}
 const char* cids[]={"transformer","tape","circuit","order","oversampling","compensation"};
 const char* clabels[]={"TRANSFORMER MODEL","TAPE MODEL","CIRCUIT TOPOLOGY","SIGNAL CHAIN ORDER","OVERSAMPLING","GAIN COMPENSATION"};
 const juce::StringArray options[]={
  {"Off","Neve 10468 Input","Neve LO1166 Output","API 2622 + 2520","API 2520 + 2503","1176 UTC Input","1176 Class A Output","1176 Rev F Output"},
  {"Off","Studer A80 15 IPS","Studer A80 30 IPS","Nakamichi Dragon Metal","Studer J37 Abbey Road 15 IPS","Budget Cassette Ferric","Budget Cassette Metal"},
  {"Off","Discrete Class A","Class AB","Op-Amp","Triode Tube","Pentode Tube"},
  {"Transformer > Tape > Circuit","Transformer > Circuit > Tape","Tape > Transformer > Circuit","Tape > Circuit > Transformer","Circuit > Transformer > Tape","Circuit > Tape > Transformer"},
  {"1x","2x","4x","8x"},{"Off","Static","RMS Auto"}};
 for(int i=0;i<6;++i){auto& k=choices[(size_t)i];k.label.setText(clabels[i],juce::dontSendNotification);k.label.setColour(juce::Label::textColourId,cream);k.label.setJustificationType(juce::Justification::centredLeft);addAndMakeVisible(k.label);for(int j=0;j<options[i].size();++j)k.control.addItem(options[i][j],j+1);k.control.setColour(juce::ComboBox::backgroundColourId,panel);k.control.setColour(juce::ComboBox::textColourId,cream);k.control.setColour(juce::ComboBox::outlineColourId,amber);addAndMakeVisible(k.control);k.attach=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.parameters,cids[i],k.control);}
 preOn.setButtonText("PRE FILTER ENABLED");postOn.setButtonText("POST FILTER ENABLED");
 wowFlutterOn.setButtonText("WOW / FLUTTER (CASSETTE ALWAYS ON)");wowFlutterOn.setColour(juce::ToggleButton::textColourId,cream);addAndMakeVisible(wowFlutterOn);
 wowFlutterAttach=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,"wowFlutterOn",wowFlutterOn);
 for(auto* b:{&preOn,&postOn}){b->setColour(juce::ToggleButton::textColourId,cream);b->setColour(juce::ToggleButton::tickColourId,teal);addAndMakeVisible(*b);}
 preAttach=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,"preOn",preOn);
 postAttach=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,"postOn",postOn);
 setSize(1000,880);startTimerHz(20);
}
void SaturatorEditor::timerCallback(){inDb=processor.inputMeter.load();outDb=processor.outputMeter.load();trimDb=processor.gainReduction.load();repaint(juce::Rectangle<int>(730,20,245,125));}
void SaturatorEditor::paint(juce::Graphics& g){
 g.fillAll(bg);g.setColour(panel);g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(12),12);g.setColour(amber);g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(12),12,2);
 g.setColour(cream);g.setFont(juce::Font(juce::FontOptions(31,juce::Font::bold)));g.drawText("SATURATOR   0.3",30,24,560,50,juce::Justification::centredLeft);
 g.setFont(juce::Font(juce::FontOptions(12)));g.drawText("TRANSFORMER  /  TAPE  /  ANALOG CIRCUIT",32,76,600,24,juce::Justification::centredLeft);
 g.setColour(amber);g.drawLine(30,112,970,112,2);
 g.setColour(cream);g.setFont(juce::Font(juce::FontOptions(12)));
 g.drawText("IN",748,29,30,20,juce::Justification::centredLeft);g.drawText("OUT",748,69,40,20,juce::Justification::centredLeft);
 auto drawMeter=[&](float db,int y){g.setColour(juce::Colour(0xff12181d));g.fillRoundedRectangle(792.f,(float)y,166.f,16.f,3.f);float w=juce::jlimit(0.f,1.f,(db+60.f)/66.f);g.setColour(db>-3.f?juce::Colours::orangered:teal);g.fillRoundedRectangle(792.f,(float)y,166.f*w,16.f,3.f);g.setColour(cream);g.drawText(juce::String(db,1)+" dBFS",790,y+18,166,17,juce::Justification::centredRight);};
 drawMeter(inDb,27);drawMeter(outDb,67);g.setColour(cream);g.drawText("TRIM  "+juce::String(trimDb,1)+" dB",748,105,212,20,juce::Justification::centredRight);
 g.setColour(amber);g.drawLine(30,300,970,300,1);g.drawLine(30,489,970,489,1);g.drawLine(30,700,970,700,1);
 g.setColour(cream);g.drawText("INPUT / DRIVE / OUTPUT",36,126,420,22,juce::Justification::centredLeft);g.drawText("PRE / POST FILTERING",36,316,420,22,juce::Justification::centredLeft);g.drawText("CHARACTER / BIAS / SIGNAL ROUTING",36,502,520,22,juce::Justification::centredLeft);g.drawText("INDIVIDUAL STAGE DRIVE OFFSETS",36,703,520,22,juce::Justification::centredLeft);
 g.setFont(juce::Font(juce::FontOptions(11)));g.drawText("Stage drives offset master drive (0 dB = linked). Class AB uses smooth biased crossover.",510,835,460,28,juce::Justification::centredRight);
}
void SaturatorEditor::resized(){
 for(int i=0;i<4;++i){auto& k=dials[(size_t)i];int x=35+i*155;k.control.setBounds(x,162,136,102);k.label.setBounds(x,263,136,21);}
 for(int i=4;i<8;++i){auto& k=dials[(size_t)i];int x=35+(i-4)*155;k.control.setBounds(x,350,136,99);k.label.setBounds(x,449,136,21);}
 for(int i=8;i<10;++i){auto& k=dials[(size_t)i];int x=35+(i-8)*155;k.control.setBounds(x,531,136,96);k.label.setBounds(x,626,136,20);}
 for(int i=10;i<13;++i){auto& k=dials[(size_t)i];int x=35+(i-10)*155;k.control.setBounds(x,720,136,88);k.label.setBounds(x,808,136,20);}
 for(int i=0;i<3;++i){auto& k=choices[(size_t)i];k.label.setBounds(655,148+i*48,305,18);k.control.setBounds(655,166+i*48,305,27);}
 for(int i=3;i<6;++i){auto& k=choices[(size_t)i];k.label.setBounds(655,334+(i-3)*53,305,19);k.control.setBounds(655,353+(i-3)*53,305,27);}
 preOn.setBounds(370,549,220,35);postOn.setBounds(370,589,220,35);wowFlutterOn.setBounds(655,516,300,30);
}
