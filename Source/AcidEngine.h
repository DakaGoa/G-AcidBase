#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>

struct Step
{
    int note = 0;
    bool gate = true, accent = false, slide = false;
    int probability = 100, ratchets = 1, timing = 0;
    bool locked = false;
};

constexpr size_t modulationSourceCount = 5; // velocity, pressure, wheel, LFO 1, LFO 2
struct ScheduledStep
{
    int64_t absolute = 0;
    int bank = 0, index = 0;
    Step step;
    double start = 0, duration = 1;
    bool plays = true;
};
uint64_t encodeStep(Step);
Step decodeStep(uint64_t);
float lfoWaveform(int shape, double phase);

struct AcidSettings
{
    float wave = 0, tune = 0, cutoff = 620, resonance = .68f, envMod = .68f;
    float decay = 240, accent = .7f, slideTime = 80, sweep = .4f, attack = 2;
    float accDecay = 180, accVolume = .5f, vibSpeed = 5, vibDepth = 0;
    float trim = 0, envCurve = .5f, accEnv = 1, accAmp = 1, filterBias = 0;
    float keyTrack = .15f, pulseWidth = .5f, clicks = .12f, noise = .015f;
    bool distortion = true, post = true;
    int model = 0;
    float dynamics = .4f, drive = 4, color = .55f;
    std::array<float, modulationSourceCount> modulationSources {};
    std::array<std::array<float, 4>, modulationSourceCount> modulationDepths {};
};

class AcidVoice
{
public:
    void prepare(double sampleRate);
    void reset();
    void noteOn(int note, float velocity, bool slide);
    void noteOff();
    void setPitchBend(float semitones) { bend = semitones; }
    void setModWheel(float amount) { modWheel = amount; }
    float render(const AcidSettings&);
    bool isActive() const { return amplitude > .00001f || gate; }
private:
    float distort(float x, const AcidSettings&);
    double rate = 192000, phase = 0, vibPhase = 0;
    float currentNote = 36, targetNote = 36, bend = 0, modWheel = 0;
    float envelope = 0, accentEnvelope = 0, amplitude = 0, velocity = 1;
    float clickEnvelope = 0, lastWave = 0, colorState = 0;
    float cutoffSmooth = 620, resonanceSmooth = .68f, driveSmooth = 4, tuneSmooth = 0;
    std::array<float, 4> poles {};
    bool gate = false, accented = false;
    juce::Random random { 303 };
};

struct FxSettings
{
    bool bypass = false, reverb = true, eq = true, delay = true, chorus = false, limiter = true;
    float predelay = 18, earlyLate = .55f, feedback = .52f, reverbMix = .13f;
    float delayTime = 3, delayFeedback = .35f, delayMix = .18f;
    float chorusRate = .4f, chorusDepth = .3f, chorusMix = .2f;
    float eqLow = 0, eqMid = 0, eqHigh = 0, volume = -9;
    std::array<std::array<float, 4>, modulationSourceCount> modulationDepths {};
};

class AcidEffects
{
public:
    void prepare(double sampleRate, int maxBlock);
    void reset();
    void process(juce::AudioBuffer<float>&, const FxSettings&, double bpm,
                 const std::array<const float*, modulationSourceCount>& modulationSources);
private:
    struct Biquad
    {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
        float tick(float x) { float y = b0*x + z1; z1 = b1*x - a1*y + z2; z2 = b2*x - a2*y; return y; }
        void peak(double rate, float frequency, float gain, float q);
    };
    double rate = 48000;
    std::array<std::vector<float>, 2> delayLines, chorusLines, predelayLines;
    std::array<std::array<Biquad, 3>, 2> equalizer;
    std::array<float, 2> dcIn {}, dcOut {}, delayTone {};
    int delayWrite = 0, chorusWrite = 0, predelayWrite = 0;
    double chorusPhase = 0;
    float gain = .35f, limiterGain = 1, delaySmooth = 18000;
    juce::Reverb reverb;
    juce::AudioBuffer<float> wetBuffer;
    static float read(const std::vector<float>&, int write, float delay);
};
