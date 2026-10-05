#include "PluginProcessor.h"
#include <cmath>

namespace
{
int positiveModulo(int64_t n,int divisor) { return static_cast<int>((n%divisor+divisor)%divisor); }
float probabilityDraw(int64_t step,int seed)
{
    // Stateless: block size, export and host seeks produce the same musical decisions.
    uint64_t x=static_cast<uint64_t>(step)^static_cast<uint64_t>(seed+1)*0x9e3779b97f4a7c15ull;
    x=(x^(x>>30))*0xbf58476d1ce4e5b9ull;x=(x^(x>>27))*0x94d049bb133111ebull;
    return static_cast<float>((x^(x>>31))>>40)/16777216.f;
}
}
void GAcidBaseProcessor::cancelMidiLearn() { midiLearnParameter.store(-1);pendingLearnMapping.store(-1); }
void GAcidBaseProcessor::setBankStep(int bank,int index,Step step)
{
    if(bank>=0&&bank<8&&index>=0&&index<16)patterns[static_cast<size_t>(bank)][static_cast<size_t>(index)].store(encodeStep(step));
}
Step GAcidBaseProcessor::getBankStep(int bank,int index) const
{
    return decodeStep(patterns[static_cast<size_t>(juce::jlimit(0,7,bank))][static_cast<size_t>(juce::jlimit(0,15,index))].load());
}
void GAcidBaseProcessor::setChainEntry(int slot,int bank,int repeats)
{
    if(slot>=0&&slot<8)chainEntries[static_cast<size_t>(slot)].store(juce::jlimit(0,7,bank)|(juce::jlimit(1,16,repeats)<<3));
}
std::pair<int,int> GAcidBaseProcessor::getChainEntry(int slot) const
{
    const int bits=chainEntries[static_cast<size_t>(juce::jlimit(0,7,slot))].load();return {bits&7,juce::jlimit(1,16,bits>>3)};
}
ScheduledStep GAcidBaseProcessor::scheduledStep(int64_t absolute) const
{
    const auto bankFor=[this](int64_t n)
    {
        if(value("chainEnabled")<.5f)return editingPattern.load();
        const int length=juce::jlimit(1,8,static_cast<int>(value("chainLength")));
        int bars=0;for(int i=0;i<length;++i)bars+=getChainEntry(i).second;
        int bar=positiveModulo(static_cast<int64_t>(std::floor(static_cast<double>(n)/16)),bars);
        for(int i=0;i<length;++i)
        {
            const auto entry=getChainEntry(i);if(bar<entry.second)return entry.first;bar-=entry.second;
        }
        return 0;
    };
    const auto startFor=[&](int64_t n)
    {
        const auto step=getBankStep(bankFor(n),positiveModulo(n,16));
        return static_cast<double>(n)+(positiveModulo(n,2)?value("swing"):0.f)+step.timing*.01;
    };
    ScheduledStep result;
    result.absolute=absolute;result.bank=bankFor(absolute);result.index=positiveModulo(absolute,16);
    result.step=getBankStep(result.bank,result.index);result.start=startFor(absolute);
    result.duration=juce::jmax(.05,startFor(absolute+1)-result.start);
    result.plays=result.step.probability>=100||probabilityDraw(absolute,static_cast<int>(value("seqSeed")))*100<result.step.probability;
    return result;
}
void GAcidBaseProcessor::generatePattern(int scale,float density,float variation)
{
    beginEdit();
    static constexpr int scales[4][7]{{0,1,3,5,7,8,10},{0,2,3,5,7,8,10},{0,2,4,5,7,9,11},{0,2,3,5,7,10,12}};
    juce::Random random(static_cast<juce::int64>(value("seqSeed")));
    for(int i=0;i<16;++i)
    {
        auto step=getStep(i);
        const float change=random.nextFloat();
        if(step.locked||change>juce::jlimit(0.f,1.f,variation))continue;
        step.note=scales[juce::jlimit(0,3,scale)][random.nextInt(7)]+(random.nextFloat()>.82f?12:0);
        step.gate=random.nextFloat()<juce::jlimit(0.f,1.f,density);
        step.accent=random.nextFloat()<.3f;step.slide=random.nextFloat()<.22f;
        setStep(i,step);
    }
    setParameter("seqSeed",static_cast<float>((static_cast<int>(value("seqSeed"))+1)&65535));
}
void GAcidBaseProcessor::resetPattern()
{
    beginEdit();for(int i=0;i<16;++i)if(!getStep(i).locked)setStep(i,{0,true,i%4==0,false});
}
bool GAcidBaseProcessor::exportMidi(const juce::File& file,bool chain) const
{
    juce::MidiMessageSequence sequence;
    sequence.addEvent(juce::MidiMessage::tempoMetaEvent(static_cast<int>(60000000./juce::jlimit(30.f,300.f,value("tempo")))));
    sequence.addEvent(juce::MidiMessage::timeSignatureMetaEvent(4,4));
    const double stepsPerBeat=value("rate")==0?2.:value("rate")==1?4.:8.;
    int stepCount=16;
    if(chain&&value("chainEnabled")>.5f)
    {
        stepCount=0;for(int i=0;i<static_cast<int>(value("chainLength"));++i)stepCount+=getChainEntry(i).second*16;
    }
    const int channel=juce::jmax(1,static_cast<int>(value("midiChannel")));
    for(int n=0;n<stepCount;++n)
    {
        auto scheduled=scheduledStep(n);
        if(!chain) // Export the selected bank even when the chain is enabled.
        {
            scheduled.step=getBankStep(selectedPattern(),n%16);
            scheduled.start=n+(n%2?value("swing"):0.f)+scheduled.step.timing*.01;
            const auto next=getBankStep(selectedPattern(),(n+1)%16);
            const double nextStart=n+1+((n+1)%2?value("swing"):0.f)+next.timing*.01;
            scheduled.duration=juce::jmax(.05,nextStart-scheduled.start);
            scheduled.plays=scheduled.step.probability>=100||probabilityDraw(n,static_cast<int>(value("seqSeed")))*100<scheduled.step.probability;
        }
        const auto step=scheduled.step;
        if(!step.gate||!scheduled.plays)continue;
        const int note=juce::jlimit(0,127,static_cast<int>(value("seqRoot"))+step.note);
        for(int pulse=0;pulse<step.ratchets;++pulse)
        {
            const double start=juce::jmax(0.,scheduled.start+pulse*scheduled.duration/step.ratchets);
            if(start>=stepCount)continue; // A late final ratchet cannot start beyond this clip.
            const double length=scheduled.duration/step.ratchets*((step.slide&&pulse==step.ratchets-1)?1.01:value("gate"));
            auto on=juce::MidiMessage::noteOn(channel,note,static_cast<juce::uint8>(step.accent?127:83));on.setTimeStamp(start/stepsPerBeat*960);sequence.addEvent(on);
            auto off=juce::MidiMessage::noteOff(channel,note);off.setTimeStamp(juce::jmin(static_cast<double>(stepCount),start+length)/stepsPerBeat*960);sequence.addEvent(off);
        }
    }
    auto end=juce::MidiMessage::endOfTrack();end.setTimeStamp(stepCount/stepsPerBeat*960);sequence.addEvent(end);
    sequence.sort();sequence.updateMatchedPairs();
    juce::MidiFile midi;midi.setTicksPerQuarterNote(960);midi.addTrack(sequence);
    auto stream=file.createOutputStream();if(!stream)return false;
    stream->setPosition(0);if(stream->truncate().failed())return false;
    return midi.writeTo(*stream,0);
}
bool GAcidBaseProcessor::assignMacro(int macro,int parameterIndex,float depth)
{
    if(macro<0||macro>=4||parameterIndex<0||parameterIndex>=parameterCacheCapacity
       ||!soundParameters[static_cast<size_t>(parameterIndex)]||!std::isfinite(depth)
       ||!learnableParameters[static_cast<size_t>(parameterIndex)]||learnableParameters[static_cast<size_t>(parameterIndex)]->isDiscrete())return false;
    macroRoutes[static_cast<size_t>(macro)][static_cast<size_t>(parameterIndex)].store(juce::jlimit(-1.f,1.f,depth));return true;
}
float GAcidBaseProcessor::macroDepth(int macro,int parameterIndex) const
{
    if(macro<0||macro>=4||parameterIndex<0||parameterIndex>=parameterCacheCapacity)return 0;
    return macroRoutes[static_cast<size_t>(macro)][static_cast<size_t>(parameterIndex)].load();
}
void GAcidBaseProcessor::captureMorph(int slot)
{
    if(slot<0||slot>=2)return;
    beginEdit();morphValid[static_cast<size_t>(slot)].store(false);
    for(size_t p=0;p<learnableParameters.size();++p)
        if(soundParameters[p]&&learnableParameters[p])morphSnapshots[static_cast<size_t>(slot)][p].store(learnableParameters[p]->getValue());
    morphValid[static_cast<size_t>(slot)].store(true);
}
bool GAcidBaseProcessor::hasMorphSnapshot(int slot) const {return slot>=0&&slot<2&&morphValid[static_cast<size_t>(slot)].load();}
void GAcidBaseProcessor::updateEffectiveParameters(bool initialise,int samples)
{
    const std::array<float,4> macros{value("macro1"),value("macro2"),value("macro3"),value("macro4")};
    const float morph=value("morph");
    const bool morphing=morphValid[0].load()&&morphValid[1].load();
    const float smoothing=static_cast<float>(1-std::exp(-samples/(.015*sampleRate)));
    for(size_t p=0;p<learnableParameters.size();++p)
    {
        auto* parameter=learnableParameters[p];if(!parameter||!soundParameters[p])continue;
        float normal=parameter->getValue();
        if(morphing)normal=morphSnapshots[0][p].load()*(1-morph)+morphSnapshots[1][p].load()*morph;
        for(size_t m=0;m<4;++m)normal+=macros[m]*macroRoutes[m][p].load();
        const float target=parameter->convertFrom0to1(juce::jlimit(0.f,1.f,normal));
        if(initialise||!effectiveInitialised||parameter->isDiscrete())effectiveValues[p]=target;
        else effectiveValues[p]+=smoothing*(target-effectiveValues[p]);
    }
    effectiveInitialised=true;
}
float GAcidBaseProcessor::soundValue(const char* id) const
{
    const auto it=parameterIndices.find(id);
    if(it==parameterIndices.end()||!effectiveInitialised||!soundParameters[static_cast<size_t>(it->second)])return value(id);
    return effectiveValues[static_cast<size_t>(it->second)];
}
void GAcidBaseProcessor::setControllerMode(int parameterIndex,int mode)
{
    for(int slot=0;slot<midiLearnCapacity;++slot)
        if(learnedControllerParameters[static_cast<size_t>(slot)].load()==parameterIndex)
        {
            learnedControllerModes[static_cast<size_t>(slot)].store(juce::jlimit(0,3,mode));
            controllerRevisions[static_cast<size_t>(slot)].fetch_add(1);
        }
}
int GAcidBaseProcessor::getControllerMode(int parameterIndex) const
{
    for(int slot=0;slot<midiLearnCapacity;++slot)
        if(learnedControllerParameters[static_cast<size_t>(slot)].load()==parameterIndex)return learnedControllerModes[static_cast<size_t>(slot)].load();
    return -1;
}
int GAcidBaseProcessor::getControllerNumber(int parameterIndex) const
{
    for(int slot=0;slot<midiLearnCapacity;++slot)
        if(learnedControllerParameters[static_cast<size_t>(slot)].load()==parameterIndex)return learnedControllerNumbers[static_cast<size_t>(slot)].load();
    return -1;
}
void GAcidBaseProcessor::beginEdit()
{
    if(restoring)return;
    auto state=stateWithPattern();
    if(!undoStates.empty()&&undoStates.back().isEquivalentTo(state))return;
    if(undoStates.size()>=64)undoStates.erase(undoStates.begin());
    undoStates.push_back(state);redoStates.clear();
}
bool GAcidBaseProcessor::undoEdit()
{
    if(undoStates.empty())return false;
    redoStates.push_back(stateWithPattern());auto state=undoStates.back();undoStates.pop_back();
    restoring=true;const bool ok=restoreState(state);restoring=false;return ok;
}
bool GAcidBaseProcessor::redoEdit()
{
    if(redoStates.empty())return false;
    undoStates.push_back(stateWithPattern());auto state=redoStates.back();redoStates.pop_back();
    restoring=true;const bool ok=restoreState(state);restoring=false;return ok;
}
void GAcidBaseProcessor::captureComparison()
{
    comparison[0]=stateWithPattern();comparison[1]=comparison[0].createCopy();comparisonB=false;
}
bool GAcidBaseProcessor::switchComparison()
{
    if(!comparison[0].isValid())captureComparison();
    beginEdit();comparison[comparisonB?1:0]=stateWithPattern();comparisonB=!comparisonB;
    restoring=true;const bool ok=restoreState(comparison[comparisonB?1:0]);restoring=false;return ok;
}
