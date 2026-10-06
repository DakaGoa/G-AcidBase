#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

const std::vector<ParameterSpec>& parameterSpecs()
{
    static const std::vector<ParameterSpec> specs {
        {"wave","Wave",0,1,0,1,"","Saw|Square",false},
        {"tune","Tuning",-24,24,0,.01f,"st","",false},
        {"cutoff","Cutoff",30,16000,620,.01f,"Hz","",true},
        {"resonance","Resonance",0,.99f,.68f,.001f,"","",false},
        {"envMod","Env mod",0,1,.68f,.001f,"","",false},
        {"decay","Decay",25,1800,240,.1f,"ms","",true},
        {"accent","Accent",0,1,.7f,.001f,"","",false},
        {"slideTime","Slide time",1,500,80,.1f,"ms","",true},
        {"sweep","Sweep amt",0,1,.4f,.001f,"","",false},
        {"attack","Env attack",.5f,150,2,.1f,"ms","",true},
        {"accDecay","Acc decay",20,900,180,.1f,"ms","",true},
        {"accVolume","Acc volume",0,1,.5f,.001f,"","",false},
        {"vibSpeed","Vib speed",.1f,20,5,.01f,"Hz","",true},
        {"vibDepth","Vib depth",0,1,0,.001f,"st","",false},
        {"trim","TM3 trim",-50,50,0,.1f,"ct","",false},
        {"envCurve","C21/22 env",0,1,.5f,.001f,"","",false},
        {"accEnv","ACCENV",0,2,1,.001f,"","",false},
        {"accAmp","ACCAMP",0,2,1,.001f,"","",false},
        {"filterBias","C13 bias",-1,1,0,.001f,"","",false},
        {"keyTrack","TM5 track",0,1,.15f,.001f,"","",false},
        {"pulseWidth","SQR PW",.1f,.9f,.5f,.001f,"","",false},
        {"clicks","BA662 clicks",0,1,.12f,.001f,"","",false},
        {"noise","BA662 noise",0,1,.015f,.001f,"","",false},
        {"mode","Play mode",0,2,0,1,"","MIDI|SEQ|ARP",false},
        {"distortion","Distortion",0,1,1,1,"","Off|On",false},
        {"model","Dist model",0,3,0,1,"","Diode clean|Asymmetric|Tube|Fold",false},
        {"post","Dist order",0,1,1,1,"","Pre filter|Post filter",false},
        {"dynamics","Dynamics",0,1,.4f,.001f,"","",false},
        {"drive","Drive",0,30,4,.01f,"dB","",false},
        {"color","Color",0,1,.55f,.001f,"","",false},
        {"reverb","Reverb",0,1,1,1,"","Off|On",false},
        {"predelay","Predelay",1,250,18,.1f,"ms","",false},
        {"earlyLate","Early / late",0,1,.55f,.001f,"","",false},
        {"feedback","Reverb decay",0,.95f,.52f,.001f,"","",false},
        {"reverbMix","Reverb mix",0,.7f,.13f,.001f,"","",false},
        {"eq","EQ",0,1,1,1,"","Off|On",false},
        {"eqLow","Low",-12,12,0,.1f,"dB","",false},
        {"eqMid","Mid",-12,12,0,.1f,"dB","",false},
        {"eqHigh","High",-12,12,0,.1f,"dB","",false},
        {"delay","Delay",0,1,1,1,"","Off|On",false},
        {"delayTime","Delay sync",0,5,2,1,"","1/16|1/8|1/8 dotted|1/4|1/4 dotted|1/2",false},
        {"delayFeedback","Delay feedback",0,.88f,.35f,.001f,"","",false},
        {"delayMix","Delay mix",0,.7f,.18f,.001f,"","",false},
        {"chorus","Chorus",0,1,0,1,"","Off|On",false},
        {"chorusRate","Chorus rate",.05f,5,.4f,.01f,"Hz","",true},
        {"chorusDepth","Chorus depth",0,1,.3f,.001f,"","",false},
        {"chorusMix","Chorus mix",0,1,.2f,.001f,"","",false},
        {"limiter","Limiter",0,1,1,1,"","Off|On",false},
        {"bypass","Bypass FX",0,1,0,1,"","Off|On",false},
        {"volume","Volume",-60,12,-9,.1f,"dB","",false},
        {"tempo","Tempo",60,220,145,.1f,"BPM","",false},
        {"run","Run",0,1,0,1,"","Stop|Run",false},
        {"seqRoot","Root",24,60,36,1,"MIDI","",false},
        {"gate","Gate",.1f,.95f,.62f,.001f,"","",false},
        {"swing","Swing",0,.45f,0,.001f,"","",false},
        {"rate","Rate",0,2,1,1,"","1/8|1/16|1/32",false},
        {"arpOctaves","Arp octaves",1,3,1,1,"","",false},
        {"arpDirection","Arp direction",0,2,0,1,"","Up|Down|Up-down",false},
        {"modVelFilter","Velocity to filter",-48,48,0,.01f,"semitones","",false},
        {"modVelAccent","Velocity to accent",-1,1,0,.001f,"","",false},
        {"modVelReverb","Velocity to reverb",-.7f,.7f,0,.001f,"","",false},
        {"modVelEffects","Velocity to effects",-1,1,0,.001f,"","",false},
        {"modPressureFilter","Aftertouch to filter",-48,48,0,.01f,"semitones","",false},
        {"modPressureAccent","Aftertouch to accent",-1,1,0,.001f,"","",false},
        {"modPressureReverb","Aftertouch to reverb",-.7f,.7f,0,.001f,"","",false},
        {"modPressureEffects","Aftertouch to effects",-1,1,0,.001f,"","",false},
        {"modWheelFilter","Mod wheel to filter",-48,48,0,.01f,"semitones","",false},
        {"modWheelAccent","Mod wheel to accent",-1,1,0,.001f,"","",false},
        {"modWheelReverb","Mod wheel to reverb",-.7f,.7f,0,.001f,"","",false},
        {"modWheelEffects","Mod wheel to effects",-1,1,0,.001f,"","",false},
        {"midiChannel","MIDI channel",0,16,0,1,"","Omni|Ch 1|Ch 2|Ch 3|Ch 4|Ch 5|Ch 6|Ch 7|Ch 8|Ch 9|Ch 10|Ch 11|Ch 12|Ch 13|Ch 14|Ch 15|Ch 16",false},
        {"lfo1Sync","LFO 1 clock",0,1,1,1,"","Free|Sync",false},
        {"lfo1Rate","LFO 1 rate",.05f,20,1,.01f,"Hz","",true},
        {"lfo1Division","LFO 1 sync",0,6,3,1,"","1/16|1/8|1/4|1/2|1 bar|2 bars|4 bars",false},
        {"lfo1Shape","LFO 1 shape",0,3,0,1,"","Sine|Triangle|Saw|Square",false},
        {"lfo1Retrigger","LFO 1 retrigger",0,1,0,1,"","Off|On",false},
        {"lfo1Filter","LFO 1 to filter",-48,48,0,.01f,"st","",false},
        {"lfo1Accent","LFO 1 to accent",-1,1,0,.001f,"","",false},
        {"lfo1Reverb","LFO 1 to reverb",-.7f,.7f,0,.001f,"","",false},
        {"lfo1Effects","LFO 1 to FX",-1,1,0,.001f,"","",false},
        {"lfo2Sync","LFO 2 clock",0,1,1,1,"","Free|Sync",false},
        {"lfo2Rate","LFO 2 rate",.05f,20,.5f,.01f,"Hz","",true},
        {"lfo2Division","LFO 2 sync",0,6,4,1,"","1/16|1/8|1/4|1/2|1 bar|2 bars|4 bars",false},
        {"lfo2Shape","LFO 2 shape",0,3,1,1,"","Sine|Triangle|Saw|Square",false},
        {"lfo2Retrigger","LFO 2 retrigger",0,1,0,1,"","Off|On",false},
        {"lfo2Filter","LFO 2 to filter",-48,48,0,.01f,"st","",false},
        {"lfo2Accent","LFO 2 to accent",-1,1,0,.001f,"","",false},
        {"lfo2Reverb","LFO 2 to reverb",-.7f,.7f,0,.001f,"","",false},
        {"lfo2Effects","LFO 2 to FX",-1,1,0,.001f,"","",false},
        {"macro1","Macro 1",0,1,0,.001f,"","",false},
        {"macro2","Macro 2",0,1,0,.001f,"","",false},
        {"macro3","Macro 3",0,1,0,.001f,"","",false},
        {"macro4","Macro 4",0,1,0,.001f,"","",false},
        {"morph","Sound morph",0,1,0,.001f,"","",false},
        {"chainEnabled","Pattern chain",0,1,0,1,"","Off|On",false},
        {"chainLength","Chain length",1,8,1,1,"","",false},
        {"seqSeed","Variation seed",0,65535,303,1,"","",false},
        {"controllerMode","New CC mode",0,3,0,1,"","Jump|Pickup|Relative signed|Relative offset",false}
    };
    return specs;
}

juce::AudioProcessorValueTreeState::ParameterLayout GAcidBaseProcessor::layout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout out;
    for (const auto& s:parameterSpecs())
    {
        if (juce::String(s.choices).isNotEmpty())
            out.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{s.id,1},s.name,juce::StringArray::fromTokens(s.choices,"|",""),static_cast<int>(s.initial)));
        else
        {
            juce::NormalisableRange<float> range(s.low,s.high,s.interval);
            if(s.skew) range.setSkewForCentre(std::sqrt(s.low*s.high));
            out.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{s.id,1},s.name,range,s.initial,juce::AudioParameterFloatAttributes().withLabel(s.unit)));
        }
    }
    return out;
}
GAcidBaseProcessor::GAcidBaseProcessor(const juce::String& feedUrl)
    : AudioProcessor(BusesProperties().withOutput("Output",juce::AudioChannelSet::stereo(),true)),
      sessionUpdates(feedUrl), parameters(*this,nullptr,"GAcidBase",layout())
{
    int specIndex=0;
    for(const auto& s:parameterSpecs())
    {
        values.emplace(s.id,parameters.getRawParameterValue(s.id));
        parameterIndices.emplace(s.id,specIndex);
        soundParameters[static_cast<size_t>(specIndex)]=specIndex<50&&juce::String(s.id)!="mode";
        ++specIndex;
    }
    for(auto& routes:macroRoutes)for(auto& depth:routes)depth.store(0);
    for(auto& snapshot:morphSnapshots)for(auto& v:snapshot)v.store(0);
    for(auto& bank:patterns)for(auto& step:bank)step.store(encodeStep(Step{}));
    for(int i=0;i<8;++i)chainEntries[static_cast<size_t>(i)].store(i|8);
    controllerPrevious.fill(-1); controllerExpected.fill(-1);
    const auto raw=getParameters();
    jassert(raw.size()<=parameterCacheCapacity);
    for(int i=0;i<parameterCacheCapacity;++i)
    {
        learnableParameters[static_cast<size_t>(i)]=i<raw.size()?dynamic_cast<juce::RangedAudioParameter*>(raw.getUnchecked(i)):nullptr;
        midiControllerValues[static_cast<size_t>(i)].store(0.f);
        midiControllerNotificationIndices[static_cast<size_t>(i)].store(-1);
    }
    for(auto& route:midiRouteMasks)
        for(auto& word:route)
            word.store(0);
    for(int i=0;i<midiLearnCapacity;++i)
    {
        learnedControllerParameters[static_cast<size_t>(i)].store(-1);
        learnedControllerNumbers[static_cast<size_t>(i)].store(-1);
        learnedControllerChannels[static_cast<size_t>(i)].store(0);
        learnedControllerModes[static_cast<size_t>(i)].store(0);
        controllerRevisions[static_cast<size_t>(i)].store(1);
    }
    setCurrentProgram(0);
    undoStates.clear();
    parameters.addParameterListener("run", this);
    parameters.addParameterListener("mode", this);
    startTimerHz(30);
}
GAcidBaseProcessor::~GAcidBaseProcessor()
{
    parameters.removeParameterListener("run", this);
    parameters.removeParameterListener("mode", this);
    stopTimer();
}
float GAcidBaseProcessor::value(const char* id) const { auto it=values.find(id); return it!=values.end()?it->second->load():0; }
void GAcidBaseProcessor::setParameter(const char* id,float v)
{
    if(auto* p=parameters.getParameter(id)) { p->beginChangeGesture(); p->setValueNotifyingHost(p->convertTo0to1(v)); p->endChangeGesture(); }
}
// The RUN latch is the sequencer's play button and must never glow while the
// sequence stands still: engaging it from plain MIDI mode selects SEQ, and
// selecting MIDI mode releases it. Each rule only ever flips the other
// parameter once, so these nested notifications cannot loop.
void GAcidBaseProcessor::parameterChanged(const juce::String& id,float newValue)
{
    if(id=="run"&&newValue>.5f&&static_cast<int>(value("mode"))==0)setParameter("mode",1);
    else if(id=="mode"&&static_cast<int>(newValue)==0&&value("run")>.5f)setParameter("run",0);
}

static const char* presetNames[] {
    "01 / Anjuna Sunrise", "02 / Goa Transmission", "03 / Emerald Serpent", "04 / Shiva Circuit", "05 / Liquid Mandala",
    "06 / Astral Runner", "07 / Psychedelic Roots", "08 / Temple of Acid", "09 / Magnetic Lotus", "10 / Full Moon Ritual",
    "11 / Bamboo Pulse", "12 / Cosmic Cobra", "13 / Neon Monsoon", "14 / Golden Triangle", "15 / Spiral Pilgrim",
    "16 / Deep Jungle 303", "17 / Silver Squelch", "18 / Solar Flare", "19 / Mantra Machine", "20 / Green Vortex",
    "21 / Sandstorm Drive", "22 / Goa Afterburner", "23 / Diode Dharma", "24 / Chrome Chakra", "25 / Acid Avalanche",
    "26 / Quantum Ritual", "27 / Furnace Bass", "28 / Electric Sadhana", "29 / Blacklight Coil", "30 / Resonant Beast",
    "31 / Midnight Dub", "32 / Dotted Mirage", "33 / Orbital Echo", "34 / Crystal Cave", "35 / Dimension Drift",
    "36 / Hypnotic Tide", "37 / Starlight Slide", "38 / Third Eye Delay", "39 / Moonlit Arp", "40 / Fractal Stream",
    "41 / Subterranean", "42 / Warm Earth", "43 / Minimal Mantra", "44 / Velvet Square", "45 / Low Orbit",
    "46 / Vintage Voltage", "47 / Pure 303", "48 / Acid Staccato", "49 / Goa Lead Bass", "50 / Final Ascension"
};
const juce::String GAcidBaseProcessor::getProgramName(int i) { return presetNames[juce::jlimit(0,49,i)]; }
void GAcidBaseProcessor::setCurrentProgram(int i)
{
    beginEdit();
    i=juce::jlimit(0,49,i);
    editingPattern.store(0);
    for(auto& routes:macroRoutes)for(auto& depth:routes)depth.store(0);
    for(auto& valid:morphValid)valid.store(false);
    // Each bank has a deliberately different musical role, with bounded variations inside the bank.
    const int bank=i/10, v=i%10;
    // Play mode and the RUN latch are transport, not sound: a preset keeps both,
    // so a running sequence continues while sounds are browsed. Resetting mode
    // here while the latch stayed lit is what made play appear dead.
    for(const auto& s:parameterSpecs()) if(juce::String(s.id)!="run"&&juce::String(s.id)!="mode") setParameter(s.id,s.initial);
    setParameter("cutoff",bank==4?90.f+v*36.f:180.f+v*84.f+bank*100.f);
    setParameter("resonance",bank==4?.3f+v*.055f:.62f+v*.034f);
    setParameter("envMod",bank==4?.3f+v*.035f:.5f+v*.038f);
    setParameter("decay",95.f+v*29.f+(bank==3?200.f:0));
    setParameter("accent",.48f+v*.05f); setParameter("slideTime",35.f+v*13.f);
    setParameter("sweep",.2f+v*.065f); setParameter("accDecay",95.f+v*22.f);
    setParameter("wave",(v==3||v==6||v==8)?1.f:0.f);
    setParameter("pulseWidth",.36f+v*.025f);
    setParameter("drive",bank==2?12.f+v*1.6f:bank==4?1.f+v*.6f:3.f+v*.9f);
    setParameter("model",bank==2?static_cast<float>(v%4):0.f);
    setParameter("post",v%3==0?0.f:1.f); setParameter("color",.36f+v*.05f);
    setParameter("delayMix",bank==3?.28f+v*.025f:bank==4?.06f:.1f+v*.014f);
    setParameter("delayFeedback",bank==3?.4f+v*.025f:.22f+v*.025f);
    setParameter("delayTime",static_cast<float>(v%4));
    setParameter("reverbMix",bank==3?.18f+v*.012f:bank==4?.05f:.08f+v*.009f);
    setParameter("chorus",(bank==3&&v%3==0)?1.f:0.f);
    setParameter("eqLow",bank==4?2.f:0.f); setParameter("eqMid",bank==2?1.5f:-.5f);
    setParameter("gate",.42f+v*.045f); setParameter("swing",v%4==0?.08f:0.f);
    setParameter("envCurve",.28f+v*.06f); setParameter("accVolume",.35f+v*.04f);
    setParameter("seqRoot",bank==4?28.f:36.f);
    static constexpr int motifs[10][8] {
        {0,0,7,0,12,0,3,7},{0,3,0,7,0,10,7,3},{0,0,12,7,0,3,7,10},{0,7,0,1,0,7,12,1},
        {0,2,3,7,0,10,12,7},{0,0,0,7,3,0,12,10},{0,5,7,0,12,5,3,0},{0,1,7,8,0,12,7,3},
        {0,7,10,12,0,3,7,15},{0,3,7,10,12,7,3,0}
    };
    for(int n=0;n<16;++n)
        setStep(n,{motifs[v][n%8]+(n>=8&&v%3==1?12:0), !((n+v)%7==5), (n+v)%4==0||(n==11), (n+v)%5==3});
    for(int bankIndex=1;bankIndex<8;++bankIndex)
        for(int n=0;n<16;++n)setBankStep(bankIndex,n,getBankStep(0,n));
    for(int slot=0;slot<8;++slot)chainEntries[static_cast<size_t>(slot)].store(slot|8);
    currentProgram=i;
}
void GAcidBaseProcessor::setStep(int i,Step s) { setBankStep(editingPattern.load(),i,s); }
Step GAcidBaseProcessor::getStep(int i) const { return getBankStep(editingPattern.load(),i); }
AcidSettings GAcidBaseProcessor::readSynth() const
{
    AcidSettings s;
    s.modulationDepths[0][0]=value("modVelAccent");s.modulationDepths[0][1]=value("modVelFilter");
    s.modulationDepths[1][0]=value("modPressureAccent");s.modulationDepths[1][1]=value("modPressureFilter");
    s.modulationDepths[2][0]=value("modWheelAccent");s.modulationDepths[2][1]=value("modWheelFilter");
#define GET(n) s.n=soundValue(#n)
    GET(wave); GET(tune); GET(cutoff); GET(resonance); GET(envMod); GET(decay); GET(accent);
    GET(slideTime); GET(sweep); GET(attack); GET(accDecay); GET(accVolume); GET(vibSpeed); GET(vibDepth);
    GET(trim); GET(envCurve); GET(accEnv); GET(accAmp); GET(filterBias); GET(keyTrack); GET(pulseWidth); GET(clicks); GET(noise);
    GET(dynamics); GET(drive); GET(color);
#undef GET
    s.modulationDepths[3]={value("lfo1Accent"),value("lfo1Filter"),value("lfo1Reverb"),value("lfo1Effects")};
    s.modulationDepths[4]={value("lfo2Accent"),value("lfo2Filter"),value("lfo2Reverb"),value("lfo2Effects")};
    s.distortion=soundValue("distortion")>.5f; s.post=soundValue("post")>.5f; s.model=static_cast<int>(soundValue("model"));
    return s;
}
FxSettings GAcidBaseProcessor::readFx() const
{
    FxSettings s;
#define GET(n) s.n=soundValue(#n)
    GET(predelay); GET(earlyLate); GET(feedback); GET(reverbMix); GET(delayTime); GET(delayFeedback); GET(delayMix);
    GET(chorusRate); GET(chorusDepth); GET(chorusMix); GET(eqLow); GET(eqMid); GET(eqHigh); GET(volume);
#undef GET
    s.modulationDepths[0]={value("modVelAccent"),value("modVelFilter"),value("modVelReverb"),value("modVelEffects")};
    s.modulationDepths[1]={value("modPressureAccent"),value("modPressureFilter"),value("modPressureReverb"),value("modPressureEffects")};
    s.modulationDepths[2]={value("modWheelAccent"),value("modWheelFilter"),value("modWheelReverb"),value("modWheelEffects")};
    s.modulationDepths[3]={value("lfo1Accent"),value("lfo1Filter"),value("lfo1Reverb"),value("lfo1Effects")};
    s.modulationDepths[4]={value("lfo2Accent"),value("lfo2Filter"),value("lfo2Reverb"),value("lfo2Effects")};
    s.bypass=soundValue("bypass")>.5f; s.reverb=soundValue("reverb")>.5f; s.eq=soundValue("eq")>.5f;
    s.delay=soundValue("delay")>.5f; s.chorus=soundValue("chorus")>.5f; s.limiter=soundValue("limiter")>.5f;
    return s;
}
bool GAcidBaseProcessor::isBusesLayoutSupported(const BusesLayout& b) const { return b.getMainOutputChannelSet()==juce::AudioChannelSet::stereo(); }
void GAcidBaseProcessor::prepareToPlay(double sr,int block)
{
    sampleRate=sr; const int capacity=juce::jmax(32,block);
    modulationBuffer.setSize(static_cast<int>(modulationSourceCount),capacity,false,false,true); modulationBuffer.clear();
    oversampler=std::make_unique<juce::dsp::Oversampling<float>>(1,2,juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,true,true);
    oversampler->initProcessing(static_cast<size_t>(capacity)); oversampler->reset();
    setLatencySamples(static_cast<int>(oversampler->getLatencyInSamples()));
    voice.prepare(sr*4); effects.prepare(sr,capacity); synthBuffer.setSize(1,capacity);
    heldCount=0; previousStep=std::numeric_limits<int64_t>::min(); previousRatchet=-1; lastMode=-1; internalPpq=0; wasPlaying=false; sequenceGate=false; previousSlide=false;
    lfoPhases.fill(0); lfoRetriggerPending=false;
    updateEffectiveParameters(true);
    keyboard.reset();currentMidiVelocity.store(0);currentMidiPressure.store(0);currentMidiWheel.store(0); activeStep=-1;
}
void GAcidBaseProcessor::releaseResources() { voice.reset(); effects.reset(); keyboard.reset(); heldCount=0;currentMidiVelocity.store(0);currentMidiPressure.store(0);currentMidiWheel.store(0);activeStep=-1; }
void GAcidBaseProcessor::audition(bool on) { if(on)keyboard.noteOn(1,36,.95f); else keyboard.noteOff(1,36,0); }
bool GAcidBaseProcessor::hasLearnedMidiController(int controller,int channel) const
{
    for(int i=0;i<midiLearnCapacity;++i)
        if(learnedControllerParameters[static_cast<size_t>(i)].load()>=0
           &&learnedControllerNumbers[static_cast<size_t>(i)].load()==controller
           &&learnedControllerChannels[static_cast<size_t>(i)].load()==channel)return true;
    return false;
}
bool GAcidBaseProcessor::beginMidiLearn(int parameterIndex)
{
    if(parameterIndex<0||parameterIndex>=parameterCacheCapacity||learnableParameters[static_cast<size_t>(parameterIndex)]==nullptr
       ||learnableParameters[static_cast<size_t>(parameterIndex)]->isDiscrete()||learnableParameters[static_cast<size_t>(parameterIndex)]->isBoolean())return false;
    cancelMidiLearn();
    midiLearnParameter.store(parameterIndex);
    return true;
}
void GAcidBaseProcessor::clearMidiLearnForParameter(int parameterIndex)
{
    if(parameterIndex<0)
    {
        pendingLearnMapping.store(-1);
        for(auto& pending:midiControllerNotificationIndices)pending.store(-1,std::memory_order_release);
        for(int i=0;i<midiLearnCapacity;++i)
        {
            const int previous=learnedControllerParameters[static_cast<size_t>(i)].exchange(-1);
            if(previous>=0)
            {
                updateMidiRoute(learnedControllerNumbers[static_cast<size_t>(i)].load(),
                                learnedControllerChannels[static_cast<size_t>(i)].load(),-(i+1));
                mappedControllerCount.fetch_sub(1);
            }
            learnedControllerNumbers[static_cast<size_t>(i)]=-1;learnedControllerChannels[static_cast<size_t>(i)]=0;
        }
        midiLearnParameter=-1;pendingLearnMapping=-1;lastLearnedController=-1;lastLearnedParameter=-1;return;
    }
    int pendingLearn=midiLearnParameter.load();
    if(pendingLearn==parameterIndex)midiLearnParameter.store(-1);
    if(parameterIndex<parameterCacheCapacity)
        midiControllerNotificationIndices[static_cast<size_t>(parameterIndex)].store(-1,std::memory_order_release);
    for(int i=0;i<midiLearnCapacity;++i)
        if(learnedControllerParameters[static_cast<size_t>(i)].load()==parameterIndex)
        {
            const int controller=learnedControllerNumbers[static_cast<size_t>(i)].load();
            const int channel=learnedControllerChannels[static_cast<size_t>(i)].load();
            updateMidiRoute(controller,channel,-(i+1));
            learnedControllerParameters[static_cast<size_t>(i)].store(-1);
            learnedControllerNumbers[static_cast<size_t>(i)].store(-1);
            learnedControllerChannels[static_cast<size_t>(i)].store(0);
            mappedControllerCount.fetch_sub(1);
        }
    int pendingMapping=pendingLearnMapping.load();
    while(pendingMapping>=0&&pendingMapping/524288==parameterIndex
          &&!pendingLearnMapping.compare_exchange_weak(pendingMapping,-1)){}
    lastLearnedController=-1;
    if(lastLearnedParameter.load()==parameterIndex)lastLearnedParameter=-1;
}
juce::String GAcidBaseProcessor::getLastLearnedParameter() const
{
    const int index=lastLearnedParameter.load();
    return index<0||index>=parameterCacheCapacity||learnableParameters[static_cast<size_t>(index)]==nullptr
        ?juce::String():learnableParameters[static_cast<size_t>(index)]->getName(48);
}
void GAcidBaseProcessor::updateMidiRoute(int controller,int channel,int slot)
{
    if(controller<0||controller>=midiControllerCount||channel<0||channel>midiChannelCount)return;
    const auto route=static_cast<size_t>(channel*midiControllerCount+controller);
    if(slot>=0) midiRouteMasks[route][static_cast<size_t>(slot/64)].fetch_or(uint64_t{1}<<(slot%64));
    else
    {
        const int removed=-slot-1;
        midiRouteMasks[route][static_cast<size_t>(removed/64)].fetch_and(~(uint64_t{1}<<(removed%64)));
    }
}
bool GAcidBaseProcessor::addLearnedMidiMapping(int parameterIndex,int controller,int channel,int mode)
{
    if(parameterIndex<0||parameterIndex>=parameterCacheCapacity||learnableParameters[static_cast<size_t>(parameterIndex)]==nullptr
       ||learnableParameters[static_cast<size_t>(parameterIndex)]->isDiscrete()||learnableParameters[static_cast<size_t>(parameterIndex)]->isBoolean()
       ||controller<0||controller>=midiControllerCount||channel<0||channel>midiChannelCount)return false;
    clearMidiLearnForParameter(parameterIndex);
    int slot=-1;
    for(int i=0;i<midiLearnCapacity;++i)
        if(learnedControllerParameters[static_cast<size_t>(i)].load()<0){slot=i;break;}
    if(slot<0)return false;
    learnedControllerNumbers[static_cast<size_t>(slot)].store(controller);
    learnedControllerChannels[static_cast<size_t>(slot)].store(channel);
    learnedControllerModes[static_cast<size_t>(slot)].store(juce::jlimit(0,3,mode));
    controllerRevisions[static_cast<size_t>(slot)].fetch_add(1);
    learnedControllerParameters[static_cast<size_t>(slot)].store(parameterIndex);
    updateMidiRoute(controller,channel,slot);
    lastLearnedController.store(controller);lastLearnedParameter.store(parameterIndex);mappedControllerCount.fetch_add(1);
    return true;
}
void GAcidBaseProcessor::timerCallback()
{
    const int pendingMapping=pendingLearnMapping.exchange(-1);
    if(pendingMapping>=0)
    {
        const int parameterIndex=pendingMapping/524288;
        const int remainder=pendingMapping%524288;
        const int channel=remainder/16384;
        const int controller=(remainder/128)%128;
        const int controllerValue=remainder%128;
        beginEdit();
        if(addLearnedMidiMapping(parameterIndex,controller,channel,static_cast<int>(value("controllerMode"))))
        {
            if(auto* parameter=learnableParameters[static_cast<size_t>(parameterIndex)])
            {
                if(static_cast<int>(value("controllerMode"))==0)
                {
                    const float normalized=static_cast<float>(controllerValue)/127.f;
                    parameter->setValueNotifyingHost(parameter->convertTo0to1(parameter->convertFrom0to1(normalized)));
                }
            }
            updateHostDisplay();
        }
    }
    bool changed=false;
    for(int i=0;i<parameterCacheCapacity;++i)
    {
        const auto index=static_cast<size_t>(i);
        const int pendingIndex=midiControllerNotificationIndices[index].exchange(-1,std::memory_order_acquire);
        if(pendingIndex>=0&&pendingIndex<parameterCacheCapacity)
        {
            if(auto* parameter=learnableParameters[static_cast<size_t>(pendingIndex)])
            {
                parameter->setValueNotifyingHost(parameter->convertTo0to1(parameter->convertFrom0to1(midiControllerValues[index].load(std::memory_order_relaxed))));
                changed=true;
            }
        }
    }
    if(changed)updateHostDisplay();
}
bool GAcidBaseProcessor::learnMidiController(int parameterIndex,int controller,int channel,int mode)
{
    if(!addLearnedMidiMapping(parameterIndex,controller,channel,mode))return false;
    updateHostDisplay();
    return true;
}
void GAcidBaseProcessor::applyLearnedMidi(const juce::MidiMessage& message)
{
    if(message.isChannelPressure())
    {
        const float pressure=static_cast<float>(message.getChannelPressureValue())/127.f;
        currentMidiPressure.store(pressure);
    }
    else if(message.isAftertouch())
    {
        const float pressure=static_cast<float>(message.getAfterTouchValue())/127.f;
        currentMidiPressure.store(pressure);
    }

    if(!message.isController())return;
    const int controller=message.getControllerNumber();
    const int channel=message.getChannel();
    const float normalized=static_cast<float>(juce::jlimit(0,127,message.getControllerValue()))/127.f;
    if(controller==1)
    {
        currentMidiWheel.store(normalized);
        voice.setModWheel(normalized);
    }
    if(controller!=0&&controller!=32&&midiLearnParameter.load()>=0)
    {
        const int parameterIndex=midiLearnParameter.exchange(-1);
        if(parameterIndex>=0)
            pendingLearnMapping.store(parameterIndex*524288+channel*16384+controller*128+message.getControllerValue());
    }

    const auto applyRoute=[&](int assignedChannel)
    {
        const auto route=static_cast<size_t>(assignedChannel*midiControllerCount+controller);
        for(size_t word=0;word<midiRouteMasks[route].size();++word)
        {
            uint64_t mapped=midiRouteMasks[route][word].load();
            while(mapped!=0)
            {
                const uint64_t lowestBit=mapped&(~mapped+1);
                int bit=0;
                while((lowestBit>>bit)!=1)++bit;
                const int slot=static_cast<int>(word*64)+bit;
                mapped&=mapped-1;
                if(slot>=midiLearnCapacity)continue;
                const int parameterIndex=learnedControllerParameters[static_cast<size_t>(slot)].load();
                if(parameterIndex<0||parameterIndex>=parameterCacheCapacity)continue;
                // A mask read can overlap remapping; validate the slot identity again.
                if(learnedControllerNumbers[static_cast<size_t>(slot)].load()!=controller
                   ||learnedControllerChannels[static_cast<size_t>(slot)].load()!=assignedChannel)continue;
                if(auto* parameter=learnableParameters[static_cast<size_t>(parameterIndex)])
                {
                    const auto index=static_cast<size_t>(slot);
                    const unsigned revision=controllerRevisions[index].load();
                    if(seenControllerRevisions[index]!=revision)
                    {
                        seenControllerRevisions[index]=revision;controllerPrevious[index]=-1;controllerExpected[index]=-1;controllerPickedUp[index]=false;
                    }
                    const int mappingMode=learnedControllerModes[index].load();
                    const float current=parameter->getValue();
                    float target=normalized;
                    if(mappingMode==1)
                    {
                        if(std::abs(current-controllerExpected[index])>.02f)controllerPickedUp[index]=false;
                        const float previous=controllerPrevious[index];
                        const bool crossed=previous>=0&&(previous-current)*(normalized-current)<=0;
                        controllerPrevious[index]=normalized;
                        if(!controllerPickedUp[index]&&(std::abs(normalized-current)<.015f||crossed))controllerPickedUp[index]=true;
                        controllerExpected[index]=current;
                        if(!controllerPickedUp[index])continue;
                    }
                    else if(mappingMode>=2)
                    {
                        const int rawValue=message.getControllerValue();
                        const int delta=mappingMode==2?(rawValue==64?0:rawValue<64?rawValue:rawValue-128):rawValue-64;
                        if(delta==0)continue;
                        const bool pending=midiControllerNotificationIndices[static_cast<size_t>(parameterIndex)].load()>=0;
                        const float base=pending?midiControllerValues[static_cast<size_t>(parameterIndex)].load():current;
                        target=juce::jlimit(0.f,1.f,base+static_cast<float>(delta)/127.f);
                    }
                    controllerExpected[index]=target;
                    midiControllerValues[static_cast<size_t>(parameterIndex)].store(target,std::memory_order_relaxed);
                    midiControllerNotificationIndices[static_cast<size_t>(parameterIndex)].store(parameterIndex,std::memory_order_release);
                }
            }
        }
    };
    applyRoute(channel);
    applyRoute(0);
}
void GAcidBaseProcessor::handleMidi(const juce::MidiMessage& m,int mode,AcidSettings& s)
{
    const int channelFilter=static_cast<int>(value("midiChannel"));
    if(channelFilter!=0&&m.getChannel()!=channelFilter)return;
    if(m.isNoteOn())
    {
        lfoRetriggerPending=true;
        const int note=m.getNoteNumber();
        for(int i=0;i<heldCount;++i)if(heldNotes[static_cast<size_t>(i)]==note) { for(int j=i;j<heldCount-1;++j)heldNotes[static_cast<size_t>(j)]=heldNotes[static_cast<size_t>(j+1)]; --heldCount; break; }
        const bool legato=heldCount>0;
        if(heldCount<128)heldNotes[static_cast<size_t>(heldCount++)]=note;
        velocities[static_cast<size_t>(note)]=m.getFloatVelocity(); root=note;currentMidiVelocity.store(m.getFloatVelocity());
        if(mode==0)
        {
            const float noteVelocity=m.getFloatVelocity();
            s.modulationSources[0]=noteVelocity;
            voice.noteOn(note,noteVelocity,legato&&s.slideTime>1);
        }
    }
    else if(m.isNoteOff())
    {
        const int note=m.getNoteNumber();
        for(int i=0;i<heldCount;++i)if(heldNotes[static_cast<size_t>(i)]==note) { for(int j=i;j<heldCount-1;++j)heldNotes[static_cast<size_t>(j)]=heldNotes[static_cast<size_t>(j+1)]; --heldCount; break; }
        if(heldCount==0){voice.noteOff();currentMidiVelocity.store(0);}
        else { root=heldNotes[static_cast<size_t>(heldCount-1)];currentMidiVelocity.store(velocities[static_cast<size_t>(root)]);if(mode==0)voice.noteOn(root,velocities[static_cast<size_t>(root)],true); }
    }
    else if(m.isAllNotesOff()||m.isAllSoundOff()) { heldCount=0;currentMidiVelocity.store(0);voice.noteOff(); }
    else if(m.isPitchWheel())voice.setPitchBend((m.getPitchWheelValue()-8192)/8192.f*2);
}
void GAcidBaseProcessor::processBlock(juce::AudioBuffer<float>& out,juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    out.clear(); if(!oversampler||out.getNumSamples()==0)return;
    // Indirect events are the on-screen keyboard and AUDITION, not duplicate host notes.
    keyboard.processNextMidiBuffer(midi,0,out.getNumSamples(),true);
    auto event=midi.cbegin(); const auto eventEnd=midi.cend();
    const int mode=static_cast<int>(value("mode"));
    double bpm=value("tempo"),ppq=internalPpq; bool hostPlaying=false;
    if(auto* ph=getPlayHead())if(auto pos=ph->getPosition())
    {
        if(auto tempo=pos->getBpm();tempo&&std::isfinite(*tempo))bpm=juce::jlimit(30.,300.,*tempo);
        hostPlaying=pos->getIsPlaying();
        if(hostPlaying)if(auto p=pos->getPpqPosition();p&&std::isfinite(*p))ppq=*p;
    }
    hostTempo=bpm;
    const double increment=bpm/(60*sampleRate);
    const double stepsPerBeat=value("rate")==0?2.:value("rate")==1?4.:8.;
    const float gateAmount=value("gate");
    const int arpOctaves=static_cast<int>(value("arpOctaves")),direction=static_cast<int>(value("arpDirection"));
    const auto blockFirst=scheduledStep(static_cast<int64_t>(std::floor(ppq*stepsPerBeat))-1);
    auto cachedSchedule=blockFirst;
    auto cachedNext=scheduledStep(blockFirst.absolute+1);
    static constexpr double lfoBeats[]{.25,.5,1,2,4,8,16};
    const std::array<bool,2> sync{value("lfo1Sync")>.5f,value("lfo2Sync")>.5f};
    const std::array<bool,2> retrigger{value("lfo1Retrigger")>.5f,value("lfo2Retrigger")>.5f};
    const std::array<int,2> shapes{static_cast<int>(value("lfo1Shape")),static_cast<int>(value("lfo2Shape"))};
    const std::array<double,2> periods{lfoBeats[juce::jlimit(0,6,static_cast<int>(value("lfo1Division")))],lfoBeats[juce::jlimit(0,6,static_cast<int>(value("lfo2Division")))]};
    const std::array<double,2> rates{value("lfo1Rate"),value("lfo2Rate")};
    if(mode!=lastMode) {voice.noteOff(); previousStep=std::numeric_limits<int64_t>::min(); previousRatchet=-1; sequenceGate=false; lastMode=mode;}
    // Bound work to prepared chunks and update automation smoothing every 32 samples.
    for(int offset=0;offset<out.getNumSamples();)
    {
        const int count=juce::jmin(32,juce::jmin(synthBuffer.getNumSamples(),out.getNumSamples()-offset));
        updateEffectiveParameters(false,count);
        auto synth=readSynth(); const auto fx=readFx();
        std::array<float*,modulationSourceCount> sources {};
        std::array<const float*,modulationSourceCount> sourceReads {};
        for(size_t source=0;source<modulationSourceCount;++source)
            sourceReads[source]=sources[source]=modulationBuffer.getWritePointer(static_cast<int>(source));
        juce::dsp::AudioBlock<float> low(synthBuffer); low=low.getSubBlock(0,static_cast<size_t>(count)); low.clear();
        auto high=oversampler->processSamplesUp(low); auto* samples=high.getChannelPointer(0);
        for(int n=0;n<count;++n)
        {
            while(event!=eventEnd&&(*event).samplePosition<=offset+n)
            {
                const auto message=(*event).getMessage();
                const int selectedChannel=static_cast<int>(value("midiChannel"));
                if(selectedChannel==0||message.getChannel()==selectedChannel)
                {
                    applyLearnedMidi(message); handleMidi(message,mode,synth);
                }
                ++event;
            }
            const bool playing=mode!=0&&(value("run")>.5f||heldCount>0);
            if(playing)
            {
                if(!wasPlaying)
                {
                    previousStep=std::numeric_limits<int64_t>::min(); previousRatchet=-1;
                    if(!hostPlaying)ppq=0;
                }
                const double position=ppq*stepsPerBeat;
                const auto base=static_cast<int64_t>(std::floor(position));
                if(cachedSchedule.start>position||cachedSchedule.absolute<base-2)
                {cachedSchedule=scheduledStep(base-1);cachedNext=scheduledStep(base);}
                while(cachedNext.start<=position)
                {cachedSchedule=cachedNext;cachedNext=scheduledStep(cachedSchedule.absolute+1);}
                const auto& scheduled=cachedSchedule;
                const auto& st=scheduled.step;
                const int pulse=juce::jlimit(0,st.ratchets-1,static_cast<int>((position-scheduled.start)*st.ratchets/scheduled.duration));
                if(scheduled.absolute!=previousStep||pulse!=previousRatchet)
                {
                    const bool newStep=scheduled.absolute!=previousStep;
                    const bool slide=newStep&&sequenceGate&&previousSlide&&pulse==0;
                    if(st.gate&&scheduled.plays&&(hostPlaying||scheduled.absolute>=0))
                    {
                        int note=(heldCount>0?root:static_cast<int>(value("seqRoot")))+st.note;
                        if(mode==2)
                        {
                            auto sorted=heldNotes;
                            std::sort(sorted.begin(),sorted.begin()+heldCount);
                            const int total=juce::jmax(1,heldCount)*arpOctaves;
                            int index=static_cast<int>((scheduled.absolute%total+total)%total);
                            if(direction==1)index=total-1-index;
                            if(direction==2&&total>1)
                            {
                                const int cycle=2*total-2; index=static_cast<int>((scheduled.absolute%cycle+cycle)%cycle);
                                if(index>=total)index=cycle-index;
                            }
                            note=(heldCount>0?sorted[static_cast<size_t>(index%heldCount)]:static_cast<int>(value("seqRoot")))+12*(index/juce::jmax(1,heldCount));
                        }
                        sequenceNote=juce::jlimit(0,127,note);
                        currentMidiVelocity.store(st.accent?1.f:.65f);
                        voice.noteOn(sequenceNote,st.accent?1.f:.65f,slide);
                        sequenceGate=true; lfoRetriggerPending=true;
                    }
                    else {voice.noteOff();currentMidiVelocity.store(0);sequenceGate=false;}
                    previousStep=scheduled.absolute; previousRatchet=pulse;
                    previousSlide=st.slide; activeStep=scheduled.index; activePattern=scheduled.bank;
                }
                const double pulsePhase=(position-scheduled.start)*st.ratchets/scheduled.duration-pulse;
                const bool tie=st.slide&&pulse==st.ratchets-1;
                if(pulsePhase>=gateAmount&&!tie&&sequenceGate)
                {voice.noteOff();currentMidiVelocity.store(0);sequenceGate=false;}
            }
            else if(wasPlaying)
            {voice.noteOff();currentMidiVelocity.store(0);previousStep=std::numeric_limits<int64_t>::min();activeStep=-1;sequenceGate=false;}
            wasPlaying=playing;
            for(size_t lfo=0;lfo<2;++lfo)
            {
                if(retrigger[lfo]&&lfoRetriggerPending)lfoPhases[lfo]=0;
                const double phase=sync[lfo]&&hostPlaying&&!retrigger[lfo]?ppq/periods[lfo]:lfoPhases[lfo];
                sources[lfo+3][n]=lfoWaveform(shapes[lfo],phase);
                lfoPhases[lfo]+=sync[lfo]?increment/periods[lfo]:rates[lfo]/sampleRate;
                lfoPhases[lfo]-=std::floor(lfoPhases[lfo]);
            }
            lfoRetriggerPending=false;
            sources[0][n]=currentMidiVelocity.load();sources[1][n]=currentMidiPressure.load();sources[2][n]=currentMidiWheel.load();
            for(size_t source=0;source<modulationSourceCount;++source)synth.modulationSources[source]=sources[source][n];
            for(int h=0;h<4;++h)samples[n*4+h]=voice.render(synth);
            ppq+=increment;
        }
        oversampler->processSamplesDown(low);
        out.copyFrom(0,offset,synthBuffer,0,0,count);out.copyFrom(1,offset,synthBuffer,0,0,count);
        float* pointers[]{out.getWritePointer(0,offset),out.getWritePointer(1,offset)};
        juce::AudioBuffer<float> chunk(pointers,2,count);
        effects.process(chunk,fx,bpm,sourceReads);offset+=count;
    }
    internalPpq=ppq;
    leftMeter=out.getMagnitude(0,0,out.getNumSamples());rightMeter=out.getMagnitude(1,0,out.getNumSamples());
    midi.clear();
}
juce::ValueTree GAcidBaseProcessor::stateWithPattern()
{
    auto state=parameters.copyState(); state.setProperty("program",currentProgram.load(),nullptr);
    for(int i=state.getNumChildren();--i>=0;)if(state.getChild(i).hasType("MidiLearn"))state.removeChild(i,nullptr);
    state.setProperty("midiLearnCount",mappedControllerCount.load(),nullptr);
    const auto rawParameters=getParameters();
    for(int i=0;i<midiLearnCapacity;++i)
    {
        const int parameterIndex=learnedControllerParameters[static_cast<size_t>(i)].load();
        if(parameterIndex<0||parameterIndex>=rawParameters.size())continue;
        auto* parameter=rawParameters.getUnchecked(parameterIndex);
        if(auto* withId=dynamic_cast<juce::AudioProcessorParameterWithID*>(parameter))
        {
            juce::ValueTree mapping("MidiLearn");mapping.setProperty("parameter",withId->paramID,nullptr);
            mapping.setProperty("controller",learnedControllerNumbers[static_cast<size_t>(i)].load(),nullptr);
            mapping.setProperty("channel",learnedControllerChannels[static_cast<size_t>(i)].load(),nullptr);
            mapping.setProperty("mode",learnedControllerModes[static_cast<size_t>(i)].load(),nullptr);
            state.addChild(mapping,-1,nullptr);
        }
    }
    for(const char* name:{"Pattern","Patterns","Chain","Macros","Morph"})
    {auto old=state.getChildWithName(name);if(old.isValid())state.removeChild(old,nullptr);}
    state.setProperty("featureVersion",2,nullptr);state.setProperty("editingPattern",editingPattern.load(),nullptr);
    juce::ValueTree banks("Patterns");
    for(int bank=0;bank<8;++bank)
    {
        juce::ValueTree steps("Pattern");
        for(int i=0;i<16;++i)
        {
            juce::ValueTree st("Step");st.setProperty("bits",static_cast<juce::int64>(patterns[static_cast<size_t>(bank)][static_cast<size_t>(i)].load()),nullptr);steps.addChild(st,-1,nullptr);
        }
        banks.addChild(steps,-1,nullptr);
    }
    // Retain the legacy primary pattern for older readers.
    state.addChild(banks.getChild(editingPattern.load()).createCopy(),-1,nullptr);state.addChild(banks,-1,nullptr);
    juce::ValueTree chain("Chain");
    for(int i=0;i<8;++i){juce::ValueTree entry("Entry");entry.setProperty("bits",chainEntries[static_cast<size_t>(i)].load(),nullptr);chain.addChild(entry,-1,nullptr);}
    state.addChild(chain,-1,nullptr);
    juce::ValueTree macros("Macros"),morph("Morph");
    for(int p=0;p<rawParameters.size();++p)
    {
        if(auto* parameter=dynamic_cast<juce::AudioProcessorParameterWithID*>(rawParameters[p]))
        {
            for(int m=0;m<4;++m)
            {
                const float depth=macroRoutes[static_cast<size_t>(m)][static_cast<size_t>(p)].load();
                if(depth==0)continue;
                juce::ValueTree route("Route");route.setProperty("macro",m,nullptr);route.setProperty("parameter",parameter->paramID,nullptr);route.setProperty("depth",depth,nullptr);macros.addChild(route,-1,nullptr);
            }
            if(soundParameters[static_cast<size_t>(p)])
                for(int slot=0;slot<2;++slot)if(morphValid[static_cast<size_t>(slot)].load())
                {
                    juce::ValueTree v("Value");v.setProperty("slot",slot,nullptr);v.setProperty("parameter",parameter->paramID,nullptr);v.setProperty("value",morphSnapshots[static_cast<size_t>(slot)][static_cast<size_t>(p)].load(),nullptr);morph.addChild(v,-1,nullptr);
                }
        }
    }
    morph.setProperty("a",morphValid[0].load(),nullptr);morph.setProperty("b",morphValid[1].load(),nullptr);
    state.addChild(macros,-1,nullptr);state.addChild(morph,-1,nullptr);return state;
}
bool GAcidBaseProcessor::restoreState(const juce::ValueTree& state)
{
    if(!state.isValid()||!state.hasType("GAcidBase"))return false;
    const auto banks=state.getChildWithName("Patterns");
    const bool modern=static_cast<int>(state.getProperty("featureVersion",0))>=2;
    for(int bank=0;bank<8;++bank)
    {
        const auto steps=banks.isValid()?banks.getChild(bank):state.getChildWithName("Pattern");
        for(int i=0;i<16;++i)
        {
            const auto bits=static_cast<uint64_t>(static_cast<juce::int64>(steps.getChild(i).getProperty("bits",76)));
            Step step=modern?decodeStep(bits):Step{static_cast<int>(bits&63)-12,(bits&64)!=0,(bits&128)!=0,(bits&256)!=0};
            setBankStep(bank,i,step);
        }
    }
    editingPattern.store(juce::jlimit(0,7,static_cast<int>(state.getProperty("editingPattern",0))));
    const auto chain=state.getChildWithName("Chain");
    for(int i=0;i<8;++i)
    {
        const int bits=static_cast<int>(chain.getChild(i).getProperty("bits",i|8));
        setChainEntry(i,bits&7,juce::jlimit(1,16,bits>>3));
    }
    for(auto& routes:macroRoutes)for(auto& depth:routes)depth.store(0);
    const auto macros=state.getChildWithName("Macros");
    for(int i=0;i<macros.getNumChildren();++i)
    {
        const auto route=macros.getChild(i);auto* parameter=parameters.getParameter(route.getProperty("parameter").toString());
        if(parameter)assignMacro(static_cast<int>(route.getProperty("macro",-1)),parameter->getParameterIndex(),static_cast<float>(route.getProperty("depth",0)));
    }
    for(auto& valid:morphValid)valid.store(false);
    const auto morph=state.getChildWithName("Morph");
    for(int i=0;i<morph.getNumChildren();++i)
    {
        const auto v=morph.getChild(i);const int slot=static_cast<int>(v.getProperty("slot",-1));
        if(auto* parameter=parameters.getParameter(v.getProperty("parameter").toString());parameter&&slot>=0&&slot<2)
        {
            const float normal=static_cast<float>(v.getProperty("value",0));
            if(std::isfinite(normal))morphSnapshots[static_cast<size_t>(slot)][static_cast<size_t>(parameter->getParameterIndex())].store(juce::jlimit(0.f,1.f,normal));
        }
    }
    morphValid[0].store(static_cast<bool>(morph.getProperty("a",false)));morphValid[1].store(static_cast<bool>(morph.getProperty("b",false)));
    clearMidiLearnForParameter(-1);
    for(auto& route:midiRouteMasks)for(auto& word:route)word.store(0);
    mappedControllerCount=0;lastLearnedController=-1;lastLearnedParameter=-1;
    const auto raw=getParameters();
    for(int i=0;i<state.getNumChildren();++i)
    {
        const auto mapping=state.getChild(i);
        if(!mapping.hasType("MidiLearn"))continue;
        const auto id=mapping.getProperty("parameter").toString();
        int parameterIndex=-1;
        for(int p=0;p<raw.size();++p)
            if(auto* withId=dynamic_cast<juce::AudioProcessorParameterWithID*>(raw.getUnchecked(p));withId&&withId->paramID==id){parameterIndex=p;break;}
        const int controller=static_cast<int>(mapping.getProperty("controller",-1));
        const int channel=static_cast<int>(mapping.getProperty("channel",0));
        if(parameterIndex>=0&&controller>=0&&controller<midiControllerCount&&channel>=0&&channel<=midiChannelCount)
            addLearnedMidiMapping(parameterIndex,controller,channel,static_cast<int>(mapping.getProperty("mode",0)));
    }
    currentProgram=juce::jlimit(0,49,static_cast<int>(state.getProperty("program",0)));
    auto merged=state.createCopy();
    // Old .gacid files must reset missing new parameters, not retain the previous sound's values.
    for(const auto& s:parameterSpecs())
    {
        bool found=false;
        for(int i=0;i<merged.getNumChildren();++i)if(merged.getChild(i).getProperty("id").toString()==s.id){found=true;break;}
        if(!found){juce::ValueTree p("PARAM");p.setProperty("id",s.id,nullptr);p.setProperty("value",s.initial,nullptr);merged.addChild(p,-1,nullptr);}
    }
    parameters.replaceState(merged);return true;
}
void GAcidBaseProcessor::getStateInformation(juce::MemoryBlock& data) { if(auto xml=stateWithPattern().createXml())copyXmlToBinary(*xml,data); }
void GAcidBaseProcessor::setStateInformation(const void* data,int bytes) { if(auto xml=getXmlFromBinary(data,bytes))restoreState(juce::ValueTree::fromXml(*xml)); }
bool GAcidBaseProcessor::savePreset(const juce::File& file) { if(auto xml=stateWithPattern().createXml())return xml->writeTo(file); return false; }
bool GAcidBaseProcessor::loadPreset(const juce::File& file) { if(auto xml=juce::XmlDocument::parse(file))return restoreState(juce::ValueTree::fromXml(*xml)); return false; }
juce::AudioProcessorEditor* GAcidBaseProcessor::createEditor() { return new GAcidBaseEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new GAcidBaseProcessor(); }
