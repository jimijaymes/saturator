#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
namespace {
float p(float hz,float sr){return std::exp(-juce::MathConstants<float>::twoPi*juce::jlimit(1.f,sr*.45f,hz)/sr);}
float d(float db){return juce::Decibels::decibelsToGain(db);}
float sat(float x){return std::tanh(x);}
float meter(float x){return juce::Decibels::gainToDecibels(juce::jmax(x,1e-7f),-100.f);}
}
float SaturatorProcessor::Filter::process(float x,float hpHz,float lpHz,float sr){
 float h=p(hpHz,sr),l=p(lpHz,sr);
 hp=h*hp+(1-h)*x;
 float high=x-hp;
 lp=l*lp+(1-l)*high;
 return lp;
}
juce::AudioProcessorValueTreeState::ParameterLayout SaturatorProcessor::layout(){
 using F=juce::AudioParameterFloat;using C=juce::AudioParameterChoice;using B=juce::AudioParameterBool;
 juce::AudioProcessorValueTreeState::ParameterLayout v;
 auto add=[&](const char* id,const char* name,float low,float high,float def,float skew=1.f){v.add(std::make_unique<F>(juce::ParameterID{id,1},name,juce::NormalisableRange<float>(low,high,.1f,skew),def));};
 add("input","Input dB",-24,24,0);add("output","Output dB",-24,24,0);
 add("drive","Drive dB",0,36,12);add("mix","Mix %",0,100,100);
 add("transformerDrive","Transformer Drive dB",-24,24,0);
 add("transformer2Drive","Transformer 2 Drive dB",-24,24,0);
 add("tapeDrive","Tape Drive dB",-24,24,0);
 add("circuitDrive","Circuit Drive dB",-24,24,0);
 add("preHP","Pre HP Hz",10,1500,20,.4f);add("preLP","Pre LP Hz",1000,20000,20000,.5f);
 add("postHP","Post HP Hz",10,1500,20,.4f);add("postLP","Post LP Hz",1000,20000,20000,.5f);
 add("character","Character %",0,100,50);add("bias","Bias %",-100,100,0);
 add("broken","Broken %",0,100,0);
 v.add(std::make_unique<C>(juce::ParameterID{"transformer",1},"Transformer",juce::StringArray{"Off","Neve 10468 Input","Neve LO1166 Output","API 2622 + 2520","API 2520 + 2503","1176 UTC Input","1176 Class A Output","1176 Rev F Output"},1));
 v.add(std::make_unique<C>(juce::ParameterID{"transformer2",1},"Transformer 2",juce::StringArray{"Off","Neve 10468 Input","Neve LO1166 Output","API 2622 + 2520","API 2520 + 2503","1176 UTC Input","1176 Class A Output","1176 Rev F Output"},0));
 v.add(std::make_unique<C>(juce::ParameterID{"tape",1},"Tape",juce::StringArray{"Off","Studer A80 15 IPS","Studer A80 30 IPS","Nakamichi Dragon Metal","Studer J37 Abbey Road 15 IPS","Budget Cassette Ferric","Budget Cassette Metal"},1));
 v.add(std::make_unique<C>(juce::ParameterID{"circuit",1},"Circuit",juce::StringArray{"Off","Discrete Class A","Class AB","Op-Amp","Triode Tube","Pentode Tube"},1));
 v.add(std::make_unique<C>(juce::ParameterID{"order",1},"Stage Order",juce::StringArray{"Transformer > Tape > Circuit","Transformer > Circuit > Tape","Tape > Transformer > Circuit","Tape > Circuit > Transformer","Circuit > Transformer > Tape","Circuit > Tape > Transformer"},0));
 v.add(std::make_unique<C>(juce::ParameterID{"oversampling",1},"Oversampling",juce::StringArray{"1x","2x","4x","8x"},2));
 v.add(std::make_unique<C>(juce::ParameterID{"compensation",1},"Gain Compensation",juce::StringArray{"Off","Static","RMS Auto"},1));
 v.add(std::make_unique<B>(juce::ParameterID{"preOn",1},"Pre Filter",true));
 v.add(std::make_unique<B>(juce::ParameterID{"postOn",1},"Post Filter",true));
 v.add(std::make_unique<B>(juce::ParameterID{"wowFlutterOn",1},"Tape Wow and Flutter",false));
 return v;
}
SaturatorProcessor::SaturatorProcessor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),parameters(*this,nullptr,"STATE",layout()){}
void SaturatorProcessor::prepareToPlay(double sampleRate,int block){
 sr=sampleRate;maxBlock=juce::jmax(1,block);states={};transformer2States={};scratch.setSize(2,maxBlock);
 tapeDelayLength=juce::jmax(512,(int)std::ceil(sr*.08));
 for(auto& line:tapeDelay)line.assign((size_t)tapeDelayLength,0.f);
 tapeWrite=0;transportTime=0;
 dryDelayLength=juce::jmax(1024,(int)std::ceil(sr*.12));
 for(auto& line:dryDelay)line.assign((size_t)dryDelayLength,0.f);
 for(auto& line:wetDelay)line.assign((size_t)dryDelayLength,0.f);
 dryWrite=0;
 for(int i=0;i<4;++i){
  oversamplers[(size_t)i]=std::make_unique<juce::dsp::Oversampling<float>>(2,(size_t)i,juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,true,false);
  oversamplers[(size_t)i]->initProcessing((size_t)maxBlock);
  oversamplers[(size_t)i]->reset();
 }
 maxOversamplingLatency=0;
 for(const auto& os:oversamplers)
  maxOversamplingLatency=juce::jmax(maxOversamplingLatency,(int)std::ceil(os->getLatencyInSamples()));
 transportLatency=(int)std::round(sr*.025);
 fixedLatency=maxOversamplingLatency+transportLatency;
 setLatencySamples(fixedLatency);
}
float SaturatorProcessor::stage(float x,int channel,int which,float drive,float character,float bias,float rate){
 auto& s=(which==3?transformer2States:states)[(size_t)channel];
 if(which==0||which==3){
  // Complete equipment-stage voicings: transformer, drive amplifier and loading.
  // Magnetic parameters are deliberately estimated; these are not measured clones.
  int model=cachedModels[which==3?3:0];if(model==0)return x;
  const float hp[]={0,22,12,24,9,28,14,18};
  const float lp[]={0,19000,17500,21000,22000,15500,18500,19500};
  const float asym[]={0,.035f,.085f,.075f,.045f,.02f,.095f,.045f};
  const float magnetDrive[]={0,.65f,1.1f,.8f,1.3f,.7f,1.35f,1.0f};
  const float amplifierDrive[]={0,.25f,.9f,.8f,1.1f,.2f,1.0f,1.15f};
  const float lowPole=cachedCoefficients[which==3?12:0],highPole=cachedCoefficients[which==3?13:1];
  s.coreLow=lowPole*s.coreLow+(1-lowPole)*x;
  s.coreHigh=highPole*s.coreHigh+(1-highPole)*x;
  const float ac=x-s.coreLow;
  // Core flux is driven by a leaky integral of voltage; low frequencies
  // generate greater flux for equal signal voltage, unlike static waveshaping.
  const float leak=cachedCoefficients[which==3?14:2];
  s.flux=leak*s.flux+(1-leak)*ac*(140.f/juce::jmax(10.f,hp[model]));
  const float core=std::tanh(s.flux*magnetDrive[model]*drive*.22f+bias*.12f);
  const float coreDist=(core-s.flux*magnetDrive[model]*drive*.22f)*(.04f+.15f*character);
  const float amp=x*drive*(.65f+amplifierDrive[model]*character);
  const float broken=juce::jlimit(0.f,1.f,(cachedBroken));
  const float supply=1.f-.92f*std::pow(broken,2.4f);
  const float starve=juce::jlimit(.02f,1.f,supply+.24f*std::abs(amp));
  float ampY=starve*(std::tanh(amp/juce::jmax(.12f,supply)+asym[model]+bias*.1f)-std::tanh(asym[model]+bias*.1f));
  ampY/=juce::jmax(.1f,drive*(.65f+amplifierDrive[model]*character));
  // Blend a predominantly clean signal with amplifier compression and
  // the nonlinear magnetic branch; load response changes with model.
  const float ampBlend=.14f+.36f*character;
  float y=(1-ampBlend)*x+ampBlend*ampY+coreDist;
  const float topLoss=(x-s.coreHigh)*(.015f+.065f*character);
  return y-topLoss;
 }
 if(which==1){
  int model=cachedModels[1];if(model==0)return x;
  // Record amp -> biased magnetic storage -> reproduce EQ/head response.
  // Figures are model estimates; nominal bandwidth targets come from manuals.
  const float head[]={0,32,48,25,36,90,65},top[]={0,17500,20500,21500,14500,7200,11200};
  // Approximate a correctly AC-biased record path: near-linear below
  // magnetic operating level, then a relatively narrow saturation knee.
  // These model-specific values are voicings, not measured calibrations.
  const float density[]={0,.90f,.75f,.65f,1.18f,2.3f,1.65f};
  const float knee[]={0,.82f,.88f,.74f,.68f,.43f,.62f};
  const float ceiling[]={0,1.20f,1.24f,1.15f,1.12f,.90f,1.03f};
  const float hfSensitivity[]={0,.20f,.13f,.28f,.34f,.60f,.43f};
  const float lowPole=cachedCoefficients[4],highPole=cachedCoefficients[3];
  s.tapeLow=lowPole*s.tapeLow+(1-lowPole)*x;
  s.tapeHigh=highPole*s.tapeHigh+(1-highPole)*x;
  const float hf=x-s.tapeHigh;
  // Frequency-dependent high-level HF compression (self-erasure proxy).
  // Envelope tracks high-frequency programme energy, not broadband drive.
  const float hfLevel=std::abs(hf);
  const float hfAttack=p(900.f,rate),hfRelease=p(85.f,rate);
  const float hfCoeff=hfLevel>s.tapeHFEnv?hfAttack:hfRelease;
  s.tapeHFEnv=hfCoeff*s.tapeHFEnv+(1-hfCoeff)*hfLevel;
  constexpr float nominalTapeHeadroom=.25f; // ~12 dB nominal calibration shift
  const float magneticDrive=juce::jmax(.001f,drive*density[model]*nominalTapeHeadroom);
  const float hfOverload=juce::jmax(0.f,magneticDrive*s.tapeHFEnv-.55f);
  const float hfGain=1.f/(1.f+hfSensitivity[model]*hfOverload);
  const float record=x-hf*(1.f-hfGain)+.025f*character*s.tapeLow;
  const float magneticBias=bias*.12f;
  const float broken=cachedBroken;
  const float dropout=1.f-.90f*std::pow(broken,3.4f);
  const float dropoutGate=juce::jlimit(.015f,1.f,dropout+.35f*std::abs(record));
  const float field=record*magneticDrive+magneticBias;
  // C1-continuous magnetic saturation: slope is exactly 1 below the
  // knee and remains continuous when approaching the saturation ceiling.
  // Unlike tanh this doesn't compress significantly below operating level.
  const auto magnetise=[&](float v){
   const float magnitude=std::abs(v);
   if(magnitude<=knee[model])return v;
   const float range=ceiling[model]-knee[model];
   const float saturated=ceiling[model]-range*std::exp(-(magnitude-knee[model])/range);
   return std::copysign(saturated,v);
  };
  const float equilibrium=magnetise(field);
  const float inertia=cachedCoefficients[5];
  s.magnet=inertia*s.magnet+(1.f-inertia)*equilibrium;
  // Magnetic history is deliberately modest: this is not a full
  // Jiles-Atherton hysteresis solver or explicit ultrasonic AC-bias model.
  const float magneticOutput=.94f*equilibrium+.06f*s.magnet;
  const float tapeOutput=(magneticOutput-magnetise(magneticBias))/magneticDrive;
  const float headBump=(model>=5?.16f:(model==4?.10f:.045f))*character*s.tapeLow;
  return (tapeOutput+headBump)*dropoutGate;
 }
 int model=cachedModels[2];if(model==0)return x;
 const float capHz[]={0,18,32,8,25,45};
 float capPole=cachedCoefficients[6];
 s.circuitCap=capPole*s.circuitCap+(1-capPole)*x;
 float signal=(x-s.circuitCap)*drive;
 const float broken=cachedBroken;
 const float supply=1.f-.93f*std::pow(broken,2.6f);
 const float starve=juce::jlimit(.015f,1.f,supply+.28f*std::abs(signal));
 signal/=juce::jmax(.12f,supply);
 float y=signal;
 switch(model){
  case 1: y=sat(signal+.18f+bias*.18f)-sat(.18f+bias*.18f);break;
  case 2: {
   // A correctly biased push-pull Class AB stage has no hard dead zone.
   // Smooth transfer between devices; residual crossover distortion
   // depends on the bias control and is small near the default bias.
   const float imbalance=juce::jlimit(0.f,1.f,std::abs(bias));
   const float crossover=.003f+.016f*character+.025f*imbalance;
   const float smoothWidth=.045f+.025f*(1.f-character);
   const float softSign=signal/std::sqrt(signal*signal+smoothWidth*smoothWidth);
   const float slope=1.f-crossover/smoothWidth;
   const float compensated=(signal-crossover*softSign)/juce::jmax(.65f,slope);
   y=std::tanh(compensated*.88f)/.88f;
   break;
  }
  case 3: y=juce::jlimit(-1.05f,1.05f,signal+.04f*signal*signal);break;
  case 4: y=sat(signal*.8f+.28f+bias*.25f)-sat(.28f+bias*.25f);break;
  case 5: y=sat(signal*1.35f+.45f+bias*.3f)-sat(.45f+bias*.3f);break;
 }
 float top=cachedCoefficients[7];
 s.circuitBias=top*s.circuitBias+(1-top)*y;
 return starve*(y-.09f*character*(y-s.circuitBias))/juce::jmax(.1f,drive);
}
void SaturatorProcessor::processChunk(juce::AudioBuffer<float>& buffer,int count,int channels,int osIndex){
 const float in=d(*parameters.getRawParameterValue("input")),out=d(*parameters.getRawParameterValue("output"));
 const float driveDb=*parameters.getRawParameterValue("drive"),drive=d(driveDb);
 const float stageDrive[3]={drive*d(*parameters.getRawParameterValue("transformerDrive")),
   drive*d(*parameters.getRawParameterValue("tapeDrive")),
   drive*d(*parameters.getRawParameterValue("circuitDrive"))};
 const float transformer2Drive=drive*d(*parameters.getRawParameterValue("transformer2Drive"));
 const float character=*parameters.getRawParameterValue("character")*.01f;
 const float broken=*parameters.getRawParameterValue("broken")*.01f;
 cachedBroken=broken;
 const float bias=*parameters.getRawParameterValue("bias")*.01f;
 // Increasing Broken removes operating bias from every active stage.
 const float brokenBias=juce::jlimit(-1.f,1.f,bias-.95f*broken);
 const float mix=*parameters.getRawParameterValue("mix")*.01f;
 const float preHP=*parameters.getRawParameterValue("preHP"),preLP=*parameters.getRawParameterValue("preLP");
 const float postHP=*parameters.getRawParameterValue("postHP"),postLP=*parameters.getRawParameterValue("postLP");
 const bool preOn=*parameters.getRawParameterValue("preOn")>.5f,postOn=*parameters.getRawParameterValue("postOn")>.5f;
 const int order=(int)*parameters.getRawParameterValue("order"),comp=(int)*parameters.getRawParameterValue("compensation");
 const int orders[6][3]={{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};
 auto* source=scratch.getArrayOfWritePointers();
 float inputEnergy=0,outputEnergy=0,inputPeak=0,outputPeak=0;
 for(int c=channels;c<2;++c) scratch.clear(c,0,count);
 for(int c=0;c<channels;++c){
  const auto* src=buffer.getReadPointer(c);
  for(int i=0;i<count;++i){source[c][i]=src[i]*in;inputEnergy+=source[c][i]*source[c][i];inputPeak=juce::jmax(inputPeak,std::abs(source[c][i]));}
 }
 auto& os=*oversamplers[(size_t)osIndex];
 juce::dsp::AudioBlock<float> original(scratch);
 auto inputBlock=original.getSubBlock(0,(size_t)count);
 juce::dsp::AudioBlock<const float> constInput(inputBlock);
 auto up=os.processSamplesUp(constInput);
 const float rate=(float)sr*(float)os.getOversamplingFactor();
 cachedModels[0]=juce::jlimit(0,7,(int)*parameters.getRawParameterValue("transformer"));
 cachedModels[3]=juce::jlimit(0,7,(int)*parameters.getRawParameterValue("transformer2"));
 cachedModels[1]=juce::jlimit(0,6,(int)*parameters.getRawParameterValue("tape"));
 cachedModels[2]=juce::jlimit(0,5,(int)*parameters.getRawParameterValue("circuit"));
 const float xfHP[]={0,22,12,24,9,28,14,18};
 const float xfLP[]={0,19000,17500,21000,22000,15500,18500,19500};
 const float tapeHP[]={0,32,48,25,36,90,65},tapeLP[]={0,17500,20500,21500,14500,7200,11200};
 const float capHP[]={0,18,32,8,25,45};
 cachedCoefficients[0]=p(xfHP[cachedModels[0]],rate);
 cachedCoefficients[1]=p(xfLP[cachedModels[0]],rate);
 cachedCoefficients[2]=p(xfHP[cachedModels[0]]*.55f,rate);
 cachedCoefficients[12]=p(xfHP[cachedModels[3]],rate);
 cachedCoefficients[13]=p(xfLP[cachedModels[3]],rate);
 cachedCoefficients[14]=p(xfHP[cachedModels[3]]*.55f,rate);
 cachedCoefficients[3]=p(tapeLP[cachedModels[1]],rate);
 cachedCoefficients[4]=p(tapeHP[cachedModels[1]],rate);
 cachedCoefficients[5]=p(cachedModels[1]>=5?360.f:(cachedModels[1]==4?500.f:2100.f),rate);
 cachedCoefficients[6]=p(capHP[cachedModels[2]],rate);
 cachedCoefficients[7]=p(cachedModels[2]==3?18500.f:12000.f,rate);
 cachedCoefficients[8]=p(preHP,rate);
 cachedCoefficients[9]=p(preLP,rate);
 cachedCoefficients[10]=p(postHP,rate);
 cachedCoefficients[11]=p(postLP,rate);
 for(int c=0;c<channels;++c){
  auto* data=up.getChannelPointer((size_t)c);
  for(size_t i=0;i<up.getNumSamples();++i){
   float x=data[i];
   if(preOn){auto& f=states[(size_t)c].pre;
    f.hp=cachedCoefficients[8]*f.hp+(1-cachedCoefficients[8])*x;
    const float high=x-f.hp;
    f.lp=cachedCoefficients[9]*f.lp+(1-cachedCoefficients[9])*high;
    x=f.lp;}
   for(int st=0;st<3;++st){const int type=orders[order][st];x=stage(x,c,type,stageDrive[type],character,brokenBias,rate);}
   // Fourth saturation stage: independent transformer after the selectable three-stage chain.
   x=stage(x,c,3,transformer2Drive,character,brokenBias,rate);
   if(postOn){auto& f=states[(size_t)c].post;
    f.hp=cachedCoefficients[10]*f.hp+(1-cachedCoefficients[10])*x;
    const float high=x-f.hp;
    f.lp=cachedCoefficients[11]*f.lp+(1-cachedCoefficients[11])*high;
    x=f.lp;}
   data[i]=x;
  }
 }
 os.processSamplesDown(inputBlock);
 for(int c=0;c<channels;++c){auto* wet=scratch.getReadPointer(c);for(int i=0;i<count;++i)outputEnergy+=wet[i]*wet[i];}
 // Model-specific tape transport. The published weighted wow/flutter figures
 // are NOT equivalent to sinusoidal peak modulation; depths are estimates.
 const int tapeModel=cachedModels[1];
 const bool transportOn=(*parameters.getRawParameterValue("wowFlutterOn")>.5f || tapeModel>=5 || cachedBroken>.001f) && tapeModel>0;
 const float wowHz[]={0,.48f,.55f,.62f,.42f,.73f,.68f};
 const float flutterHz[]={0,8.2f,9.5f,12.3f,6.4f,11.5f,10.2f};
 const float wowPct[]={0,.00012f,.00010f,.00017f,.00034f,.0032f,.0021f};
 const float flutterPct[]={0,.00009f,.00008f,.00011f,.00024f,.0018f,.0012f};
 const float delayBase=(float)(sr*.025);
 if(transportOn)for(int i=0;i<count;++i){
  const double t=transportTime;transportTime+=1.0/sr;
  const float wow=std::sin((float)(juce::MathConstants<double>::twoPi*wowHz[tapeModel]*t));
  const float drift=std::sin((float)(juce::MathConstants<double>::twoPi*.13*t+1.1));
  const float flutter=std::sin((float)(juce::MathConstants<double>::twoPi*flutterHz[tapeModel]*t))
    +.3f*std::sin((float)(juce::MathConstants<double>::twoPi*flutterHz[tapeModel]*1.79*t));
  const float wowSamples=(float)sr*wowPct[tapeModel]
      *(wow/juce::jmax(.1f,juce::MathConstants<float>::twoPi*wowHz[tapeModel])
       +.12f*drift/(juce::MathConstants<float>::twoPi*.13f));
  const float flutterSamples=(float)sr*flutterPct[tapeModel]
      *(flutter/(juce::MathConstants<float>::twoPi*flutterHz[tapeModel]));
  // Broken magnifies speed instability, adding irregular capstan drift.
  const float damage=1.f+18.f*cachedBroken*cachedBroken;
  const float rough=(float)sr*.00020f*cachedBroken*cachedBroken
    *(std::sin((float)(juce::MathConstants<double>::twoPi*3.7*t))
      +.38f*std::sin((float)(juce::MathConstants<double>::twoPi*17.3*t)));
  const float readDelay=juce::jlimit(1.f,(float)tapeDelayLength-3.f,delayBase+damage*(wowSamples+flutterSamples)+rough);
  const float readPos=(float)tapeWrite-readDelay;
  const int index0=((int)std::floor(readPos)%tapeDelayLength+tapeDelayLength)%tapeDelayLength;
  const int index1=(index0+1)%tapeDelayLength;
  const float frac=readPos-std::floor(readPos);
  for(int c=0;c<channels;++c){
   auto& line=tapeDelay[(size_t)c];
   const float x=scratch.getSample(c,i);
   line[(size_t)tapeWrite]=x;
   scratch.setSample(c,i,line[(size_t)index0]*(1-frac)+line[(size_t)index1]*frac);
  }
  tapeWrite=(tapeWrite+1)%tapeDelayLength;
 }
 const float rmsIn=std::sqrt(inputEnergy/juce::jmax(1,count*channels));
 const float rmsOut=std::sqrt(outputEnergy/juce::jmax(1,count*channels));
 const float staticTrim=d(-driveDb*.20f);
 const float autoTrim=juce::jlimit(.25f,4.f,rmsIn/juce::jmax(.0001f,rmsOut));
 const float trim=comp==0?1.f:(comp==1?staticTrim:autoTrim);
 const int wetPadding=juce::jlimit(0,dryDelayLength-1,
    fixedLatency-(int)std::round(oversamplers[(size_t)osIndex]->getLatencyInSamples())-
    (transportOn?transportLatency:0));
 const int totalDryDelay=fixedLatency;
 for(int c=0;c<channels;++c){auto* dst=buffer.getWritePointer(c);const auto* wet=scratch.getReadPointer(c);for(int i=0;i<count;++i){
  const float dryInput=dst[i]*in;
  const int dryLatency=totalDryDelay;
  auto& ring=dryDelay[(size_t)c];
  const int read=(dryWrite+i+dryDelayLength-dryLatency)%dryDelayLength;
  const float dry=dryLatency==0?dryInput:ring[(size_t)read];
  ring[(size_t)((dryWrite+i)%dryDelayLength)]=dryInput;
  auto& wetRing=wetDelay[(size_t)c];
  const int wetRead=(dryWrite+i+dryDelayLength-wetPadding)%dryDelayLength;
  const float alignedWet=wetPadding==0?wet[i]:wetRing[(size_t)wetRead];
  wetRing[(size_t)((dryWrite+i)%dryDelayLength)]=wet[i];
  float y=(dry*(1-mix)+alignedWet*trim*mix)*out;
  dst[i]=y;outputPeak=juce::jmax(outputPeak,std::abs(y));
 }}
 dryWrite=(dryWrite+count)%dryDelayLength;
 inputMeter.store(meter(inputPeak));outputMeter.store(meter(outputPeak));
 gainReduction.store(juce::Decibels::gainToDecibels(juce::jmax(.0001f,trim),-100.f));
}
void SaturatorProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&){
 juce::ScopedNoDenormals guard;
 int n=buffer.getNumSamples(),channels=juce::jmin(2,buffer.getNumChannels());
 if(n==0||channels==0)return;
 for(int c=channels;c<buffer.getNumChannels();++c)buffer.clear(c,0,n);
 const int idx=juce::jlimit(0,3,(int)*parameters.getRawParameterValue("oversampling"));
 // Host PDC is fixed in prepareToPlay; never notify the host from audio thread.
 // Wet signal is padded to this fixed latency in processChunk().
 for(int pos=0;pos<n;pos+=maxBlock){
  const int count=juce::jmin(maxBlock,n-pos);
  float* ptrs[2]={buffer.getWritePointer(0,pos),channels>1?buffer.getWritePointer(1,pos):nullptr};
  juce::AudioBuffer<float> chunk(ptrs,channels,count);
  processChunk(chunk,count,channels,idx);
 }
}
juce::AudioProcessorEditor* SaturatorProcessor::createEditor(){return new SaturatorEditor(*this);}
void SaturatorProcessor::getStateInformation(juce::MemoryBlock& b){auto xml=parameters.copyState().createXml();copyXmlToBinary(*xml,b);}
void SaturatorProcessor::setStateInformation(const void* data,int bytes){if(auto xml=getXmlFromBinary(data,bytes))if(xml->hasTagName(parameters.state.getType()))parameters.replaceState(juce::ValueTree::fromXml(*xml));}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new SaturatorProcessor();}
