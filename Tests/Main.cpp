#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <iostream>
#include <set>
#include <chrono>

namespace
{
int failures=0;
void check(bool good,const std::string& message)
{
    std::cout<<(good?"PASS ":"FAIL ")<<message<<std::endl; if(!good)++failures;
}
struct Stats { double square=0, sum=0; float peak=0; size_t samples=0; bool finite=true; };
void measure(const juce::AudioBuffer<float>& b,Stats& s)
{
    for(int c=0;c<b.getNumChannels();++c)for(int i=0;i<b.getNumSamples();++i)
    {
        const float x=b.getSample(c,i); s.finite &= std::isfinite(x); s.peak=juce::jmax(s.peak,std::abs(x)); s.square+=x*x; s.sum+=x; ++s.samples;
    }
}
void writeWav(const juce::File& file,const juce::AudioBuffer<float>& audio,double sr)
{
    juce::WavAudioFormat format;
    auto stream=file.createOutputStream();
    if(!stream){check(false,"open WAV "+file.getFileName().toStdString());return;}
    stream->setPosition(0);
    if(stream->truncate().failed()){check(false,"truncate previous WAV render");return;}
    std::unique_ptr<juce::AudioFormatWriter> writer(format.createWriterFor(stream.get(),sr,static_cast<unsigned int>(audio.getNumChannels()),24,{},0));
    if(!writer){check(false,"create WAV writer");return;}
    stream.release(); check(writer->writeFromAudioSampleBuffer(audio,0,audio.getNumSamples()),"write "+file.getFileName().toStdString());
}
}
int main(int argc,char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const auto directory=juce::File::getCurrentWorkingDirectory().getChildFile("artifacts"); directory.createDirectory();
    const auto presets=directory.getChildFile("Presets"); presets.createDirectory();
    GAcidBaseProcessor processor;
    check(processor.getNumPrograms()==50,"exactly 50 host-visible factory programs");
    check(processor.getParameters().size()==static_cast<int>(parameterSpecs().size()),"every control exposed to host automation");
    constexpr int block=256; constexpr double sr=48000;
    const int frames=static_cast<int>(sr*1.5);
    juce::AudioBuffer<float> montage(2,frames*50); montage.clear();
    std::set<juce::String> names,states;
    double maxRms=0,minRms=1,totalCpu=0;
    juce::String csv="preset,peak,rms,dc,cpu_ms\n";
    for(int preset=0;preset<50;++preset)
    {
        processor.setCurrentProgram(preset); processor.setParameter("run",0);
        auto name=processor.getProgramName(preset); names.insert(name);
        juce::MemoryBlock data; processor.getStateInformation(data); states.insert(data.toBase64Encoding());
        const auto filename=name.replace(" / "," - ").replaceCharacter('/','-')+".gacid";
        check(processor.savePreset(presets.getChildFile(filename)),"export "+name.toStdString());
        processor.setParameter("mode",1); processor.setParameter("run",1);
        processor.prepareToPlay(sr,block);
        juce::AudioBuffer<float> audio(2,block); juce::MidiBuffer midi;
        Stats stats; const auto start=std::chrono::steady_clock::now();
        for(int offset=0;offset<frames;offset+=block)
        {
            const int count=juce::jmin(block,frames-offset); audio.setSize(2,count,false,false,true);
            processor.processBlock(audio,midi); measure(audio,stats);
            for(int c=0;c<2;++c)montage.copyFrom(c,preset*frames+offset,audio,c,0,count);
        }
        const auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count(); totalCpu+=ms;
        const double rms=std::sqrt(stats.square/static_cast<double>(stats.samples)),dc=stats.sum/static_cast<double>(stats.samples);
        maxRms=juce::jmax(maxRms,rms); minRms=juce::jmin(minRms,rms);
        check(stats.finite&&stats.peak<=.941f&&rms>.002&&std::abs(dc)<.025,"audio finite / audible / limited / DC controlled: "+name.toStdString());
        csv+=name.replaceCharacter(',',' ')+","+juce::String(stats.peak,6)+","+juce::String(rms,6)+","+juce::String(dc,6)+","+juce::String(ms,2)+"\n";
        processor.releaseResources();
    }
    check(names.size()==50&&states.size()==50,"all 50 factory programs have unique names and states");
    writeWav(directory.getChildFile("G-AcidBase-50-Preset-Demo.wav"),montage,sr);
    directory.getChildFile("preset-audio-measurements.csv").replaceWithText(csv);
    processor.setCurrentProgram(4); processor.setParameter("cutoff",777); processor.setStep(3,{-7,false,true,true});
    juce::MemoryBlock state; processor.getStateInformation(state); processor.setCurrentProgram(0); processor.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
    const auto step=processor.getStep(3);
    check(std::abs(processor.value("cutoff")-777)<.1f&&processor.getCurrentProgram()==4&&step.note==-7&&!step.gate&&step.accent&&step.slide,"host state restores parameters, program and complete pattern");
    check(!processor.loadPreset(directory.getChildFile("missing.gacid")),"missing preset rejected");
    for(double rate:{44100.,48000.,96000.})for(int size:{64,511,2048})
    {
        processor.setCurrentProgram(29); processor.setParameter("mode",0); processor.setParameter("run",0); processor.setParameter("delay",0); processor.setParameter("reverb",0);
        processor.prepareToPlay(rate,size); juce::AudioBuffer<float> audio(2,size); juce::MidiBuffer midi; Stats stats;
        for(int n=0;n<20;++n)
        {
            if(n==0)midi.addEvent(juce::MidiMessage::noteOn(1,36,.65f),0);
            if(n==3)midi.addEvent(juce::MidiMessage::noteOn(1,43,1.f),size/2);
            if(n==4)midi.addEvent(juce::MidiMessage::pitchWheel(1,16383),0);
            if(n==6)midi.addEvent(juce::MidiMessage::noteOff(1,43),0);
            if(n==8)midi.addEvent(juce::MidiMessage::allNotesOff(1),0);
            processor.processBlock(audio,midi); measure(audio,stats);
        }
        check(stats.finite&&stats.peak<=.941f&&stats.peak>.005f,"MIDI note/accent/legato/pitch/panic at "+std::to_string(static_cast<int>(rate))+" Hz / "+std::to_string(size));
        processor.releaseResources();
    }
    // Sample-accurate MIDI onset and note release, with effects disabled.
    processor.setCurrentProgram(46); processor.setParameter("run",0); processor.setParameter("mode",0); processor.setParameter("bypass",1);
    processor.prepareToPlay(sr,block); juce::AudioBuffer<float> audio(2,block); juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1,36,1.f),128); processor.processBlock(audio,midi);
    check(audio.getMagnitude(0,0,128)<.000001f&&audio.getMagnitude(0,160,96)>.001f,"MIDI onset respects sample offset");
    midi.addEvent(juce::MidiMessage::allNotesOff(1),0); for(int n=0;n<150;++n)processor.processBlock(audio,midi);
    check(audio.getMagnitude(0,0,block)<.00001f,"panic releases dry voice to silence");
    processor.releaseResources();
    // ARP uses held notes; sequence gate, slide and pattern are exercised above.
    processor.setCurrentProgram(38); processor.setParameter("mode",2); processor.setParameter("run",0); processor.prepareToPlay(sr,block);
    midi.addEvent(juce::MidiMessage::noteOn(1,36,.7f),0); midi.addEvent(juce::MidiMessage::noteOn(1,43,1.f),30);
    Stats arp; for(int i=0;i<200;++i){processor.processBlock(audio,midi);measure(audio,arp);} check(arp.finite&&arp.peak>.01f&&processor.activeStep>=0,"arpeggiator runs from held MIDI notes");
    midi.addEvent(juce::MidiMessage::allNotesOff(1),0); processor.processBlock(audio,midi); check(processor.activeStep==-1,"arpeggiator stops on panic"); processor.releaseResources();
    processor.setCurrentProgram(0); processor.setParameter("run",0);
    {
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        auto* acid=dynamic_cast<GAcidBaseEditor*>(editor.get()); check(acid!=nullptr,"native editor creates");
        if(acid)
        {
            acid->saveUiSnapshot(directory.getChildFile("G-AcidBase-Interface.png"));
            std::vector<juce::TextButton*> buttons;
            std::vector<juce::ComboBox*> boxes;
            std::function<void(juce::Component&)> collect=[&](juce::Component& component)
            {
                if(auto* button=dynamic_cast<juce::TextButton*>(&component))buttons.push_back(button);
                if(auto* box=dynamic_cast<juce::ComboBox*>(&component))boxes.push_back(box);
                for(int i=0;i<component.getNumChildComponents();++i)collect(*component.getChildComponent(i));
            };
            collect(*editor);
            auto click=[&](const juce::String& title)
            {
                for(auto* b:buttons)if(b->getButtonText()==title&&b->isVisible())
                {
                    if(b->getClickingTogglesState())b->setToggleState(!b->getToggleState(),juce::dontSendNotification);
                    if(b->onClick)b->onClick();
                    return true;
                }
                return false;
            };
            check(click("DELAY")&&click("EQ")&&click("CHORUS")&&click("REVERB"),"native effect page buttons respond");
            check(click("CIRCUIT TRIMS"),"native circuit trims page opens");
            acid->saveUiSnapshot(directory.getChildFile("G-AcidBase-Circuit.png"));
            check(click("CIRCUIT TRIMS"),"native synthesis page returns");
            bool selected=false;
            for(auto* box:boxes)if(box->getNumItems()==50){box->setSelectedId(25,juce::sendNotificationSync);selected=true;break;}
            check(selected&&processor.getCurrentProgram()==24,"native preset menu changes factory program");
            check(click("RESET")&&processor.getStep(0).note==0&&processor.getStep(0).accent,"native pattern reset responds");
            check(click("MUTATE"),"native pattern mutation responds");
            processor.setCurrentProgram(0);
        }
        editor->setSize(960,705); check(editor->getWidth()==960,"editor resizes to supported minimum");
    }
    if(argc>1)
    {
        juce::AudioPluginFormatManager manager; manager.addFormat(new juce::VST3PluginFormat());
        juce::OwnedArray<juce::PluginDescription> descriptions;
        manager.getFormat(0)->findAllTypesForFile(descriptions,juce::String(argv[1]));
        check(descriptions.size()==1,"actual VST3 bundle scans as one instrument");
        if(!descriptions.isEmpty())
        {
            juce::String error; auto instance=manager.createPluginInstance(*descriptions[0],sr,block,error);
            check(instance!=nullptr,"actual VST3 instantiates: "+error.toStdString());
            if(instance)
            {
                check(instance->getNumPrograms()==50,"VST3 wrapper exposes 50 programs");
                instance->prepareToPlay(sr,block); audio.clear(); midi.clear(); midi.addEvent(juce::MidiMessage::noteOn(1,36,1.f),0);
                Stats vst; for(int n=0;n<100;++n){instance->processBlock(audio,midi);measure(audio,vst);}
                check(vst.finite&&vst.peak>.01f,"actual VST3 outputs audible finite MIDI-triggered audio");
                std::unique_ptr<juce::AudioProcessorEditor> editor(instance->createEditorIfNeeded()); check(editor!=nullptr,"actual VST3 native editor opens"); editor.reset(); instance->releaseResources();
            }
        }
    }
    else check(false,"VST3 path argument required for delivery verification");
    juce::String summary="G-AcidBase automated verification\nFailures: "+juce::String(failures)+"\n50 presets: 48 kHz stereo, 1.5 s each\nRMS range: "+juce::String(minRms,6)+" to "+juce::String(maxRms,6)+"\nTotal preset rendering CPU: "+juce::String(totalCpu,1)+" ms for 75 seconds of audio\nNo claim of circuit-identical 303 emulation or subjective best sound quality.\n";
    directory.getChildFile("verification.txt").replaceWithText(summary); std::cout<<summary<<std::endl;
    return failures==0?0:1;
}
