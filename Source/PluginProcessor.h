#pragma once
#include <JuceHeader.h>
#include "AcidEngine.h"
#include <map>

struct ParameterSpec
{
    const char* id;
    const char* name;
    float low, high, initial, interval;
    const char* unit;
    const char* choices;
    bool skew;
};
const std::vector<ParameterSpec>& parameterSpecs();

class GAcidBaseProcessor : public juce::AudioProcessor, private juce::Timer,
    private juce::AudioProcessorValueTreeState::Listener
{
public:
    GAcidBaseProcessor();
    ~GAcidBaseProcessor() override;
    void prepareToPlay(double, int) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "G-AcidBase"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 8; }
    int getNumPrograms() override { return 50; }
    int getCurrentProgram() override { return currentProgram.load(); }
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int,const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
    bool savePreset(const juce::File&);
    bool loadPreset(const juce::File&);
    void setParameter(const char*,float);
    float value(const char*) const;
    void audition(bool on);
    bool beginMidiLearn(int parameterIndex);
    void cancelMidiLearn();
    bool learnMidiController(int parameterIndex, int controller, int channel, int mode = 0);
    bool hasLearnedMidiController(int controller, int channel) const;
    void clearMidiLearnForParameter(int parameterIndex);
    int getLearnedMidiControllerCount() const { return mappedControllerCount.load(); }
    int getMidiLearnParameter() const { return midiLearnParameter.load(); }
    int getLastLearnedController() const { return lastLearnedController.load(); }
    juce::String getLastLearnedParameter() const;
    void setControllerMode(int parameterIndex, int mode);
    int getControllerMode(int parameterIndex) const;
    int getControllerNumber(int parameterIndex) const;
    bool assignMacro(int macro, int parameterIndex, float depth);
    float macroDepth(int macro, int parameterIndex) const;
    bool canAssignMacro(int index) const { return index>=0&&index<parameterCacheCapacity&&soundParameters[static_cast<size_t>(index)]&&learnableParameters[static_cast<size_t>(index)]&&!learnableParameters[static_cast<size_t>(index)]->isDiscrete(); }
    void clearMorph() { morphValid[0].store(false);morphValid[1].store(false); }
    void captureMorph(int slot);
    bool hasMorphSnapshot(int slot) const;
    void setStep(int, Step);
    Step getStep(int) const;
    void setBankStep(int bank, int index, Step);
    Step getBankStep(int bank, int index) const;
    void selectPattern(int bank) { editingPattern.store(juce::jlimit(0,7,bank)); }
    int selectedPattern() const { return editingPattern.load(); }
    void setChainEntry(int slot, int bank, int repeats);
    std::pair<int,int> getChainEntry(int slot) const;
    ScheduledStep scheduledStep(int64_t absolute) const;
    void generatePattern(int scale, float density, float variation);
    void resetPattern();
    bool exportMidi(const juce::File&, bool chain) const;
    void beginEdit(); // message-thread edit checkpoint, including slider gestures
    bool undoEdit();
    bool redoEdit();
    bool canUndo() const { return !undoStates.empty(); }
    bool canRedo() const { return !redoStates.empty(); }
    void captureComparison();
    bool switchComparison();
    bool comparisonIsB() const { return comparisonB; }
    juce::AudioProcessorValueTreeState parameters;
    juce::MidiKeyboardState keyboard;
    std::atomic<float> leftMeter {0}, rightMeter {0};
    std::atomic<int> activeStep {-1}, activePattern {0};
    std::atomic<double> hostTempo {145};
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    juce::ValueTree stateWithPattern();
    bool restoreState(const juce::ValueTree&);
    void handleMidi(const juce::MidiMessage&, int mode, AcidSettings&);
    void parameterChanged(const juce::String&, float) override;
    void applyLearnedMidi(const juce::MidiMessage&);
    void timerCallback() override;
    void updateMidiRoute(int controller, int channel, int slot);
    bool addLearnedMidiMapping(int parameterIndex, int controller, int channel, int mode = 0);
    AcidSettings readSynth() const;
    FxSettings readFx() const;
    void updateEffectiveParameters(bool initialise, int samples = 32);
    float soundValue(const char*) const;
    static constexpr int midiLearnCapacity = 512;
    static constexpr int midiControllerCount = 128;
    static constexpr int midiChannelCount = 16;
    static constexpr int midiRouteCount = (midiChannelCount + 1) * midiControllerCount;
    static constexpr int parameterCacheCapacity = 192;
    std::array<std::atomic<int>, midiLearnCapacity> learnedControllerParameters;
    std::array<std::atomic<int>, midiLearnCapacity> learnedControllerNumbers;
    std::array<std::atomic<int>, midiLearnCapacity> learnedControllerChannels;
    std::array<std::atomic<int>, midiLearnCapacity> learnedControllerModes;
    std::array<std::atomic<unsigned>, midiLearnCapacity> controllerRevisions;
    std::array<unsigned, midiLearnCapacity> seenControllerRevisions {};
    std::array<float, midiLearnCapacity> controllerPrevious {}, controllerExpected {};
    std::array<bool, midiLearnCapacity> controllerPickedUp {};
    std::array<std::array<std::atomic<uint64_t>, midiLearnCapacity / 64>, midiRouteCount> midiRouteMasks;
    std::array<juce::RangedAudioParameter*, parameterCacheCapacity> learnableParameters {};
    std::array<std::atomic<float>, parameterCacheCapacity> midiControllerValues;
    std::array<std::atomic<int>, parameterCacheCapacity> midiControllerNotificationIndices;
    std::atomic<int> midiLearnParameter {-1};
    std::atomic<int> pendingLearnMapping {-1};
    std::atomic<int> lastLearnedController {-1};
    std::atomic<int> lastLearnedParameter {-1};
    std::atomic<int> mappedControllerCount {0};
    std::atomic<float> currentMidiVelocity {0}, currentMidiPressure {0}, currentMidiWheel {0};
    std::array<std::array<std::atomic<float>, parameterCacheCapacity>,4> macroRoutes;
    std::array<std::array<std::atomic<float>, parameterCacheCapacity>,2> morphSnapshots;
    std::array<std::atomic<bool>,2> morphValid {{false,false}};
    std::array<float,parameterCacheCapacity> effectiveValues {};
    std::array<bool,parameterCacheCapacity> soundParameters {};
    std::map<std::string,int,std::less<>> parameterIndices;
    bool effectiveInitialised = false;
    std::array<double,2> lfoPhases {};
    bool lfoRetriggerPending = false;
    juce::AudioBuffer<float> modulationBuffer;
    AcidVoice voice;
    AcidEffects effects;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    juce::AudioBuffer<float> synthBuffer;
    std::map<std::string,std::atomic<float>*,std::less<>> values;
    std::array<std::array<std::atomic<uint64_t>,16>,8> patterns;
    std::array<std::atomic<int>,8> chainEntries;
    std::atomic<int> editingPattern {0};
    std::array<int,128> heldNotes {};
    std::array<float,128> velocities {};
    int heldCount=0, root=36, lastMode=-1, sequenceNote=36;
    int64_t previousStep=std::numeric_limits<int64_t>::min();
    int previousRatchet=-1;
    double sampleRate=48000, internalPpq=0;
    bool sequenceGate=false, wasPlaying=false, previousSlide=false;
    std::atomic<int> currentProgram {0};
    std::vector<juce::ValueTree> undoStates, redoStates;
    std::array<juce::ValueTree,2> comparison;
    bool restoring = false, comparisonB = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GAcidBaseProcessor)
};
