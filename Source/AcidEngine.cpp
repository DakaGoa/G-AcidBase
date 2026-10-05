#include "AcidEngine.h"
#include <cmath>

namespace
{
constexpr float pi = juce::MathConstants<float>::pi;
float blep(double t, double dt)
{
    if (t < dt) { auto x = static_cast<float>(t / dt); return x + x - x*x - 1; }
    if (t > 1 - dt) { auto x = static_cast<float>((t - 1) / dt); return x*x + x + x + 1; }
    return 0;
}
float saturate(float x) { return std::tanh(x); }
}

uint64_t encodeStep(Step s)
{
    return static_cast<uint64_t>(juce::jlimit(-12,24,s.note)+12)
        | (s.gate?64ull:0) | (s.accent?128ull:0) | (s.slide?256ull:0)
        | (static_cast<uint64_t>(juce::jlimit(0,100,s.probability))<<9)
        | (static_cast<uint64_t>(juce::jlimit(1,4,s.ratchets)-1)<<16)
        | (static_cast<uint64_t>(juce::jlimit(-25,25,s.timing)+25)<<18)
        | (s.locked?(1ull<<24):0);
}
Step decodeStep(uint64_t bits)
{
    return {static_cast<int>(bits&63)-12,(bits&64)!=0,(bits&128)!=0,(bits&256)!=0,
        static_cast<int>((bits>>9)&127),static_cast<int>((bits>>16)&3)+1,
        static_cast<int>((bits>>18)&63)-25,(bits&(1ull<<24))!=0};
}
float lfoWaveform(int shape,double phase)
{
    phase-=std::floor(phase);
    switch(shape)
    {
        case 1:return static_cast<float>(1-4*std::abs(phase-.5));
        case 2:return static_cast<float>(2*phase-1);
        case 3:return phase<.5?1.f:-1.f;
        default:return std::sin(static_cast<float>(phase)*2*juce::MathConstants<float>::pi);
    }
}

void AcidVoice::prepare(double sampleRate) { rate = sampleRate; reset(); }
void AcidVoice::reset()
{
    phase = vibPhase = 0;
    envelope = accentEnvelope = amplitude = clickEnvelope = lastWave = colorState = 0;
    poles.fill(0); gate = accented = false; bend = modWheel = 0;
    currentNote = targetNote = 36; cutoffSmooth = 620; resonanceSmooth = .68f; driveSmooth = 4; tuneSmooth = 0;
}
void AcidVoice::noteOn(int note, float v, bool slide)
{
    const bool legato = slide && gate;
    targetNote = static_cast<float>(note);
    if (!legato) { currentNote = targetNote; envelope = 1; clickEnvelope = 1; }
    velocity = juce::jlimit(.05f, 1.f, v);
    accented = v >= .8f;
    if (accented) accentEnvelope = 1;
    gate = true;
}
void AcidVoice::noteOff() { gate = false; }

float AcidVoice::distort(float x, const AcidSettings& s)
{
    const float drive = juce::Decibels::decibelsToGain(driveSmooth);
    const float dynamics = 1 + s.dynamics * envelope * 1.5f;
    x *= drive * dynamics;
    const float tone = 1 - std::exp(-2*pi*(1000 + s.color*15000) / static_cast<float>(rate));
    colorState += tone * (x - colorState);
    x = colorState;
    float y = 0;
    switch (s.model)
    {
        case 1: y = saturate(x + .15f) - saturate(.15f); break;
        case 2: y = std::atan(x * 1.6f) / 1.3f; break;
        case 3: y = std::sin(juce::jlimit(-pi, pi, x)); break;
        default: y = saturate(x * (x >= 0 ? 1.3f : .9f)); break;
    }
    return y / std::sqrt(drive * dynamics);
}

float AcidVoice::render(const AcidSettings& s)
{
    const float sr = static_cast<float>(rate);
    float filterMod=0,accentMod=0;
    for(size_t source=0;source<modulationSourceCount;++source)
    {
        filterMod+=s.modulationSources[source]*s.modulationDepths[source][1];
        accentMod+=s.modulationSources[source]*s.modulationDepths[source][0];
    }
    const float cutoffMod=juce::jlimit(-48.f,48.f,filterMod);
    const float smoothing = 1 - std::exp(-1 / (.008f*sr));
    cutoffSmooth += smoothing*(juce::jlimit(30.f,16000.f,s.cutoff*std::pow(2.f,cutoffMod/12.f))-cutoffSmooth);
    resonanceSmooth += smoothing*(s.resonance-resonanceSmooth);
    driveSmooth += smoothing*(s.drive-driveSmooth);
    tuneSmooth += smoothing*(s.tune+s.trim*.01f-tuneSmooth);
    currentNote += (1 - std::exp(-1/(juce::jmax(1.f, s.slideTime)*.001f*sr))) * (targetNote-currentNote);
    vibPhase += s.vibSpeed/rate; if (vibPhase >= 1) vibPhase -= 1;
    const float vibrato = std::sin(static_cast<float>(vibPhase)*2*pi)*(s.vibDepth + modWheel*.5f);
    const double freq = 440 * std::pow(2., (currentNote+tuneSmooth+bend+vibrato-69)/12.);
    const double dt = juce::jlimit(.000001, .2, freq/rate);
    phase += dt; if (phase >= 1) phase -= 1;
    float saw = static_cast<float>(2*phase-1) - blep(phase, dt);
    const double pw = juce::jlimit(.1, .9, static_cast<double>(s.pulseWidth));
    float pulse = (phase < pw ? 1.f : -1.f) + blep(phase, dt);
    double p2 = phase-pw; if (p2 < 0) p2 += 1;
    pulse -= blep(p2, dt);
    float x = (s.wave < .5f ? saw : pulse) * .55f;
    x += (random.nextFloat()*2-1)*s.noise*.035f;
    x += clickEnvelope*s.clicks*.08f;
    clickEnvelope *= std::exp(-1/(.0015f*sr));
    if (s.distortion && !s.post) x = distort(x, s);

    envelope *= std::exp(-1/(juce::jmax(25.f, s.decay)*.001f*sr));
    accentEnvelope *= std::exp(-1/(juce::jmax(20.f, s.accDecay)*.001f*sr));
    const float shape = std::pow(envelope, .5f + s.envCurve*1.3f);
    const float envOctaves = s.envMod*6.5f*shape
                           +juce::jlimit(0.f,2.f,s.accent+accentMod)*s.accEnv*accentEnvelope*(1+s.sweep)*2.1f;
    const float tracking = (currentNote-36)*s.keyTrack/12;
    const float fc = juce::jlimit(25.f, juce::jmin(18000.f, sr*.18f), cutoffSmooth*std::pow(2.f, envOctaves+tracking));
    const float g = std::tan(pi*fc/sr), G = g/(1+g);
    const float G2 = G*G, G3 = G2*G, G4 = G2*G2;
    const float k = resonanceSmooth*3.95f;
    const float feedbackState = (1-G)*(G3*poles[0]+G2*poles[1]+G*poles[2]+poles[3]);
    float u = (x-k*feedbackState)/(1+k*G4);
    u = saturate(u*(1.15f+s.filterBias*.35f));
    for (auto& state : poles)
    {
        const float v = (u-state)*G;
        const float y = v+state;
        state = saturate(y+v);
        u = y;
    }
    // Resonance compensation retains fundamental energy without removing the characteristic squelch.
    float out = u*(1+resonanceSmooth*.65f);
    if (s.distortion && s.post) out = distort(out, s);
    const float ampTarget = gate ? (.72f + velocity*.28f) : 0;
    const float ampTime = gate ? juce::jmax(.5f,s.attack)*.001f : .009f;
    amplitude += (1-std::exp(-1/(ampTime*sr)))*(ampTarget-amplitude);
    const float accentAmount=juce::jlimit(0.f,2.f,s.accent+accentMod);
    out *= amplitude*(1+accentAmount*s.accVolume*s.accAmp*accentEnvelope*.8f);
    return out;
}

void AcidEffects::Biquad::peak(double sr, float f, float db, float q)
{
    const float A = std::pow(10.f,db/40), w = 2*pi*f/static_cast<float>(sr);
    const float alpha = std::sin(w)/(2*q), c = std::cos(w), a0 = 1+alpha/A;
    b0=(1+alpha*A)/a0; b1=-2*c/a0; b2=(1-alpha*A)/a0;
    a1=-2*c/a0; a2=(1-alpha/A)/a0;
}
void AcidEffects::prepare(double sr, int maxBlock)
{
    rate = sr;
    for (int c=0;c<2;++c)
    {
        delayLines[c].assign(static_cast<size_t>(sr*4.1)+4,0);
        chorusLines[c].assign(static_cast<size_t>(sr*.07)+4,0);
        predelayLines[c].assign(static_cast<size_t>(sr*.251)+4,0);
    }
    wetBuffer.setSize(2,maxBlock);
    reverb.setSampleRate(sr); reset();
}
void AcidEffects::reset()
{
    for (auto* lines : { &delayLines, &chorusLines, &predelayLines })
        for (auto& line : *lines) std::fill(line.begin(),line.end(),0.f);
    for (auto& channel : equalizer) for (auto& eq : channel) eq.z1=eq.z2=0;
    dcIn.fill(0); dcOut.fill(0); delayTone.fill(0);
    delayWrite=chorusWrite=predelayWrite=0; chorusPhase=0; gain=.35f; limiterGain=1;
    delaySmooth=static_cast<float>(rate*.375); reverb.reset();
}
float AcidEffects::read(const std::vector<float>& line, int write, float delay)
{
    float index = static_cast<float>(write)-juce::jlimit(1.f,static_cast<float>(line.size()-2),delay);
    if (index < 0) index += static_cast<float>(line.size());
    const int a = static_cast<int>(index), b = (a+1)%static_cast<int>(line.size());
    return line[static_cast<size_t>(a)] + (index-a)*(line[static_cast<size_t>(b)]-line[static_cast<size_t>(a)]);
}
void AcidEffects::process(juce::AudioBuffer<float>& buffer, const FxSettings& s, double bpm,
                          const std::array<const float*, modulationSourceCount>& modulationSources)
{
    const int count = buffer.getNumSamples();
    auto* l=buffer.getWritePointer(0); auto* r=buffer.getWritePointer(1);
    float* wetL=wetBuffer.getWritePointer(0); float* wetR=wetBuffer.getWritePointer(1);
    const float sr=static_cast<float>(rate);
    static constexpr float beats[] { .25f, .5f, .75f, 1.f, 1.5f, 2.f };
    const float delayTarget=juce::jlimit(1.f,static_cast<float>(delayLines[0].size()-2), sr*60/static_cast<float>(juce::jlimit(30.,300.,bpm))*beats[juce::jlimit(0,5,static_cast<int>(s.delayTime))]);
    const bool rv=s.reverb&&!s.bypass, dl=s.delay&&!s.bypass, ch=s.chorus&&!s.bypass;
    for (auto& channel : equalizer)
    {
        channel[0].peak(rate,100,s.eqLow,.65f); channel[1].peak(rate,900,s.eqMid,.7f); channel[2].peak(rate,6000,s.eqHigh,.65f);
    }
    for (int i=0;i<count;++i)
    {
        float values[] { l[i],r[i] };
        delaySmooth += .0005f*(delayTarget-delaySmooth);
        const float delayed[] { read(delayLines[0],delayWrite,delaySmooth),read(delayLines[1],delayWrite,delaySmooth) };
        float fxMod=0;
        for(size_t source=0;source<modulationSourceCount;++source)
            if(modulationSources[source])fxMod+=modulationSources[source][i]*s.modulationDepths[source][3];
        const float delayWet=juce::jlimit(0.f,.7f,s.delayMix+fxMod*.35f);
        for (int c=0;c<2;++c)
        {
            float x=values[c];
            if (s.eq&&!s.bypass) for (auto& eq : equalizer[c]) x=eq.tick(x);
            const float cdelay=sr*(.012f+.006f*s.chorusDepth*std::sin(static_cast<float>(chorusPhase)*2*pi+c*pi*.5f));
            const float cv=read(chorusLines[c],chorusWrite,cdelay);
            chorusLines[c][static_cast<size_t>(chorusWrite)]=x;
            if (ch) x=x*(1-s.chorusMix*.5f)+cv*s.chorusMix*.5f;
            delayTone[c]+=.15f*(delayed[1-c]-delayTone[c]);
            delayLines[c][static_cast<size_t>(delayWrite)]=dl ? saturate(x+delayTone[c]*s.delayFeedback) : 0;
            if (dl) x += delayed[c]*delayWet;
            predelayLines[c][static_cast<size_t>(predelayWrite)] = rv ? x : 0;
            (c==0?wetL:wetR)[i]=read(predelayLines[c],predelayWrite,s.predelay*.001f*sr);
            (c==0?l:r)[i]=x;
        }
        delayWrite=(delayWrite+1)%static_cast<int>(delayLines[0].size());
        chorusWrite=(chorusWrite+1)%static_cast<int>(chorusLines[0].size());
        predelayWrite=(predelayWrite+1)%static_cast<int>(predelayLines[0].size());
        chorusPhase+=s.chorusRate/rate; if(chorusPhase>=1)chorusPhase-=1;
    }
    juce::Reverb::Parameters rp;
    rp.roomSize=s.feedback; rp.damping=s.earlyLate; rp.width=1; rp.wetLevel=1; rp.dryLevel=0;
    reverb.setParameters(rp); reverb.processStereo(wetL,wetR,count);
    const float smooth=1-std::exp(-1/(.02f*sr));
    for(int i=0;i<count;++i)
    {
        float reverbDepth=0,fxMod=0;
        for(size_t source=0;source<modulationSourceCount;++source)
            if(modulationSources[source])
            {
                reverbDepth+=modulationSources[source][i]*s.modulationDepths[source][2];
                fxMod+=modulationSources[source][i]*s.modulationDepths[source][3];
            }
        const float reverbMod=juce::jlimit(0.f,.7f,s.reverbMix+reverbDepth);
        const float volumeMod=juce::jlimit(-60.f,12.f,s.volume+fxMod*12.f);
        const float modulatedGain=juce::Decibels::decibelsToGain(volumeMod);
        gain+=smooth*(modulatedGain-gain);
        float values[2] { (l[i]+(rv?wetL[i]*reverbMod:0))*gain,
                           (r[i]+(rv?wetR[i]*reverbMod:0))*gain };
        for(int c=0;c<2;++c)
        {
            const float x=values[c]; values[c]=x-dcIn[c]+.995f*dcOut[c]; dcIn[c]=x; dcOut[c]=values[c];
        }
        const float peak=juce::jmax(std::abs(values[0]),std::abs(values[1]));
        const float desired=peak>.94f ? .94f/peak : 1;
        if(desired<limiterGain)limiterGain=desired; else limiterGain+=(1-std::exp(-1/(.08f*sr)))*(desired-limiterGain);
        const float limit=(s.limiter&&!s.bypass)?limiterGain:1;
        l[i]=values[0]*limit; r[i]=values[1]*limit;
    }
}
