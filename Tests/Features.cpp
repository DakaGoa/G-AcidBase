#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include "../Source/Logo.h"
#include <iostream>
#include <stdexcept>
#include <windows.h>

namespace
{
void require(bool condition,const juce::String& message)
{
    if(!condition)throw std::runtime_error(message.toStdString());
}
int parameterIndex(GAcidBaseProcessor& processor,const juce::String& id)
{
    for(int i=0;i<processor.getParameters().size();++i)
        if(auto* parameter=dynamic_cast<juce::AudioProcessorParameterWithID*>(processor.getParameters().getUnchecked(i));parameter&&parameter->paramID==id)return i;
    return -1;
}
void pumpMessagesFor(int milliseconds)
{
    const auto deadline=GetTickCount64()+static_cast<ULONGLONG>(milliseconds);
    MSG message {};
    while(GetTickCount64()<deadline)
    {
        while(PeekMessage(&message,nullptr,0,0,PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
        Sleep(2);
    }
}
}

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;
        juce::File::getCurrentWorkingDirectory().getChildFile("artifacts/feature-verification.txt").replaceWithText("G-AcidBase expanded feature verification\nFailures: pending\n");
        auto processorOwner=std::make_unique<GAcidBaseProcessor>();auto& processor=*processorOwner;
        const int cutoff=parameterIndex(processor,"cutoff");
        const int wave=parameterIndex(processor,"wave");
        require(cutoff>=0&&processor.beginMidiLearn(cutoff),"continuous parameter enters MIDI learn");
        require(processor.getMidiLearnParameter()==cutoff,"learn request waits for a controller");
        require(!processor.beginMidiLearn(wave),"choice parameter is excluded from MIDI learn");
        processor.setParameter("midiChannel",1.f);
        processor.prepareToPlay(48000,64);
        const auto sendCC=[](GAcidBaseProcessor& target,int controller,int value,int channel)
        {
            juce::AudioBuffer<float> audio(2,64);
            juce::MidiBuffer midi;
            midi.addEvent(juce::MidiMessage::controllerEvent(channel,controller,value),0);
            target.processBlock(audio,midi);
            pumpMessagesFor(120);
        };
        sendCC(processor,74,96,1);
        require(processor.getMidiLearnParameter()==-1,"first incoming CC completes learn");
        require(processor.getLearnedMidiControllerCount()==1&&processor.hasLearnedMidiController(74,1),"learn stores the controller and channel");
        processor.setParameter("midiChannel",0.f);
        const auto firstLearnedValue=processor.value("cutoff");
        require(firstLearnedValue>1000.f,"learned controller applies its first position to parameter");
        sendCC(processor,74,20,2);
        require(processor.value("cutoff")>1000.f,"channel-specific mapping ignores other channels");
        sendCC(processor,74,20,1);
        require(processor.value("cutoff")<firstLearnedValue,"learned controller changes its parameter");
        juce::MemoryBlock state;
        processor.getStateInformation(state);
        auto restoredOwner=std::make_unique<GAcidBaseProcessor>();auto& restored=*restoredOwner;
        restored.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
        require(restored.getLearnedMidiControllerCount()==1&&restored.hasLearnedMidiController(74,1),"learned mappings survive host state restore");
        restored.prepareToPlay(48000,64);
        sendCC(restored,74,20,1);
        require(restored.value("cutoff")<4000.f,"restored mapping controls its parameter");
        restored.clearMidiLearnForParameter(-1);
        require(restored.getLearnedMidiControllerCount()==0&&!restored.hasLearnedMidiController(74,1),"clear learn removes routing");

        processor.releaseResources();
        auto renderRoute=[](const char* depthID,float depth,bool pressureEvent,bool wheelEvent)
        {
            auto synthOwner=std::make_unique<GAcidBaseProcessor>();auto& synth=*synthOwner;
            synth.setCurrentProgram(46);
            synth.setParameter("mode",0);synth.setParameter("bypass",1);
            synth.setParameter("reverb",0);synth.setParameter("delay",0);
            synth.setParameter(depthID,depth);
            synth.prepareToPlay(48000,8192);
            juce::AudioBuffer<float> output(2,8192);
            juce::MidiBuffer events;
            events.addEvent(juce::MidiMessage::noteOn(1,36,1.f),0);
            if(pressureEvent)events.addEvent(juce::MidiMessage::channelPressureChange(1,127),4096);
            if(wheelEvent)events.addEvent(juce::MidiMessage::controllerEvent(1,1,127),4096);
            synth.processBlock(output,events);
            return output.getRMSLevel(0,0,output.getNumSamples());
        };
        const auto velocityFilter=renderRoute("modVelFilter",24.f,false,false);
        const auto dryFilter=renderRoute("modVelFilter",0.f,false,false);
        require(std::abs(velocityFilter-dryFilter)>.00001f,"velocity-to-filter route changes rendered audio");
        const auto aftertouchFilter=renderRoute("modPressureFilter",24.f,true,false);
        const auto dryAftertouch=renderRoute("modPressureFilter",0.f,true,false);
        require(std::abs(aftertouchFilter-dryAftertouch)>.00001f,"aftertouch-to-filter route changes rendered audio");
        const auto wheelFilter=renderRoute("modWheelFilter",24.f,false,true);
        const auto dryWheel=renderRoute("modWheelFilter",0.f,false,true);
        require(std::abs(wheelFilter-dryWheel)>.00001f,"mod-wheel-to-filter route changes rendered audio");
        const auto velocityAccent=renderRoute("modVelAccent",1.f,false,false);
        const auto dryAccent=renderRoute("modVelAccent",0.f,false,false);
        require(std::abs(velocityAccent-dryAccent)>.00001f,"velocity-to-accent route changes rendered audio");
        const auto wheelAccent=renderRoute("modWheelAccent",1.f,false,true);
        const auto dryWheelAccent=renderRoute("modWheelAccent",0.f,false,true);
        require(std::abs(wheelAccent-dryWheelAccent)>.00001f,"mod-wheel-to-accent route changes rendered audio");
        const auto velocityGain=renderRoute("modVelEffects",1.f,false,false);
        const auto dryGain=renderRoute("modVelEffects",0.f,false,false);
        require(velocityGain>dryGain*1.5f,"velocity-to-effects route modulates output gain");

        // Persistent bank scheduling, generation locks, deterministic chance, and real MIDI file parsing.
        auto musicalOwner=std::make_unique<GAcidBaseProcessor>();auto& musical=*musicalOwner;
        musical.setBankStep(0,0,{0,true,true,false,100,4,-10,true});
        musical.setBankStep(1,0,{7,true,false,true,100,2,15,false});
        musical.setChainEntry(0,0,2);musical.setChainEntry(1,1,1);
        musical.setParameter("chainEnabled",1);musical.setParameter("chainLength",2);
        require(musical.scheduledStep(31).bank==0&&musical.scheduledStep(32).bank==1&&musical.scheduledStep(48).bank==0,"chain repeats and wraps correctly");
        require(std::abs(musical.scheduledStep(0).start+.1)<.0001,"negative microtiming advances steps");
        require(musical.getBankStep(0,0).ratchets==4,"ratchets survive packed step storage");
        musical.generatePattern(2,1,1);
        require(musical.getStep(0).locked&&musical.getStep(0).ratchets==4&&musical.getStep(0).note==0,"generator preserves locked steps");
        const int major[]{0,2,4,5,7,9,11};
        for(int i=1;i<16;++i)
        {
            bool inScale=false;for(int degree:major)inScale|=musical.getStep(i).note%12==degree;
            require(inScale&&musical.getStep(i).gate,"full-density generator obeys chosen scale");
        }
        musical.setBankStep(0,1,{0,true,false,false,0,1,0,false});
        require(!musical.scheduledStep(1).plays,"zero probability always rests");
        musical.setBankStep(0,2,{0,true,false,false,50,1,0,false});
        require(musical.scheduledStep(2).plays==musical.scheduledStep(2).plays,"probability is deterministic across queries");
        const auto midiFile=juce::File::getCurrentWorkingDirectory().getChildFile("artifacts/feature-pattern.mid");
        require(musical.exportMidi(midiFile,true),"chain MIDI export writes requested file");
        auto input=midiFile.createInputStream();juce::MidiFile imported;
        require(input&&imported.readFrom(*input)&&imported.getNumTracks()==1&&imported.getTimeFormat()==960,"export is readable format-0 960-PPQ MIDI");
        int noteOns=0;const auto* track=imported.getTrack(0);
        for(int i=0;i<track->getNumEvents();++i)if(track->getEventPointer(i)->message.isNoteOn())++noteOns;
        require(noteOns>48&&std::abs(track->getEndTime()-48*240)<.001,"ratchets and repeated chain are present in exported MIDI");
        musical.setBankStep(0,15,{0,true,false,true,100,4,25,false});musical.setBankStep(0,0,{0,true,true,false,100,1,25,false});musical.setParameter("swing",.45f);
        input.reset();require(musical.exportMidi(midiFile,false),"late final-ratchet MIDI exports");input=midiFile.createInputStream();juce::MidiFile lateExport;
        require(input&&lateExport.readFrom(*input),"late final-ratchet MIDI parses");
        const auto* lateTrack=lateExport.getTrack(0);require(std::abs(lateTrack->getEndTime()-16*240)<.001,"late final ratchets do not extend clip length");
        for(int i=0;i<lateTrack->getNumEvents();++i)
        {
            const auto* event=lateTrack->getEventPointer(i);
            require(event->message.getTimeStamp()<=16*240,"all exported events stay inside clip bounds");
            if(event->message.isNoteOn())require(event->noteOffObject&&event->noteOffObject->message.getTimeStamp()>event->message.getTimeStamp(),"every late ratchet has a later note-off");
        }
        input.reset();musical.setParameter("swing",0);
        const int musicalCutoff=parameterIndex(musical,"cutoff");
        require(musical.assignMacro(0,musicalCutoff,.5f),"macro can target cutoff");
        require(!musical.assignMacro(0,parameterIndex(musical,"macro1"),.5f),"macro cycles are excluded");
        musical.captureMorph(0);musical.setParameter("cutoff",4000);musical.captureMorph(1);
        juce::MemoryBlock musicState;musical.getStateInformation(musicState);
        auto recalledOwner=std::make_unique<GAcidBaseProcessor>();auto& recalled=*recalledOwner;recalled.setStateInformation(musicState.getData(),static_cast<int>(musicState.getSize()));
        require(recalled.hasMorphSnapshot(0)&&recalled.hasMorphSnapshot(1)&&recalled.macroDepth(0,musicalCutoff)==.5f,"macro and both morph snapshots recall");
        require(recalled.getBankStep(1,0).timing==15&&recalled.getChainEntry(0).second==2,"all pattern banks and chain repeats recall");
        musical.beginEdit();musical.setParameter("cutoff",888);musical.setStep(3,{12,false,true,true,25,3,20,true});
        require(musical.undoEdit()&&std::abs(musical.value("cutoff")-4000)<.1f,"undo restores complete pre-edit state");
        require(musical.redoEdit()&&std::abs(musical.value("cutoff")-888)<.1f&&musical.getStep(3).ratchets==3,"redo restores parameter and extended step");
        musical.captureComparison();musical.setParameter("cutoff",999);require(musical.switchComparison(),"A/B comparison switches");
        require(std::abs(musical.value("cutoff")-888)<.1f,"B initially recalls captured comparison");
        musical.setParameter("cutoff",777);musical.switchComparison();require(std::abs(musical.value("cutoff")-999)<.1f,"A/B retains independent edits");

        auto controllerOwner=std::make_unique<GAcidBaseProcessor>();auto& controller=*controllerOwner;
        controller.prepareToPlay(48000,64);const int drive=parameterIndex(controller,"drive");
        controller.setParameter("drive",15);require(controller.learnMidiController(drive,71,1,1),"pickup mapping installed");
        sendCC(controller,71,0,1);require(std::abs(controller.value("drive")-15)<.02f,"pickup ignores mismatched first position");
        sendCC(controller,71,90,1);require(controller.value("drive")>20,"pickup follows controller after crossing parameter");
        controller.setParameter("drive",6);sendCC(controller,71,100,1);require(std::abs(controller.value("drive")-6)<.02f,"pickup rearms after manual or host edit");
        sendCC(controller,71,10,1);require(controller.value("drive")<3,"pickup reacquires on crossing");
        controller.setControllerMode(drive,2);controller.setParameter("drive",15);sendCC(controller,71,1,1);require(controller.value("drive")>15,"relative signed increments");
        sendCC(controller,71,127,1);require(std::abs(controller.value("drive")-15)<.03f,"relative signed decrements");
        controller.setControllerMode(drive,3);sendCC(controller,71,65,1);require(controller.value("drive")>15,"relative offset increments");
        controller.beginMidiLearn(drive);controller.cancelMidiLearn();require(controller.getMidiLearnParameter()<0&&controller.hasLearnedMidiController(71,1),"cancel relearn preserves old mapping");

        const auto renderFeature=[](const char* id,float depth,bool wheel,bool keyboardInput)
        {
            auto pOwner=std::make_unique<GAcidBaseProcessor>();auto& p=*pOwner;p.setCurrentProgram(46);p.setParameter("bypass",1);p.setParameter("noise",0);p.setParameter("envMod",0);p.setParameter(id,depth);
            p.prepareToPlay(48000,64);juce::AudioBuffer<float> audio(2,4096);juce::MidiBuffer events;
            if(keyboardInput)p.audition(true);else events.addEvent(juce::MidiMessage::noteOn(1,36,.95f),0);
            if(wheel)events.addEvent(juce::MidiMessage::controllerEvent(1,1,127),0);
            p.processBlock(audio,events);return audio;
        };
        const auto plain=renderFeature("lfo1Filter",0,false,false),lfo=renderFeature("lfo1Filter",36,false,false);
        float difference=0;for(int i=0;i<4096;++i)difference+=std::abs(plain.getSample(0,i)-lfo.getSample(0,i));
        require(difference>.01f,"tempo-synced LFO changes rendered sound");
        const auto wheel=renderFeature("lfo1Filter",0,true,false);difference=0;
        for(int i=0;i<4096;++i)difference+=std::abs(plain.getSample(0,i)-wheel.getSample(0,i));
        require(difference>.01f,"legacy CC1 vibrato works with zero matrix depths");
        require(renderFeature("lfo1Filter",0,false,true).getMagnitude(0,0,4096)>.001f,"AUDITION/on-screen indirect MIDI reaches the engine");
        require(std::abs(lfoWaveform(1,0)+1)<.001f&&std::abs(lfoWaveform(2,.5))<.001f,"LFO waveform shapes have expected values");

        // Old preset compatibility: missing new parameters reset; legacy steps gain safe defaults.
        auto legacy=juce::ValueTree("GAcidBase");juce::ValueTree oldPattern("Pattern"),oldStep("Step");
        oldStep.setProperty("bits",76,nullptr);oldPattern.addChild(oldStep,-1,nullptr);legacy.addChild(oldPattern,-1,nullptr);
        auto legacyFile=juce::File::getCurrentWorkingDirectory().getChildFile("artifacts/legacy-compatibility.gacid");
        require(legacy.createXml()->writeTo(legacyFile),"legacy preset fixture written");
        recalled.setParameter("lfo1Filter",48);require(recalled.loadPreset(legacyFile),"old preset loads");
        require(recalled.value("lfo1Filter")==0&&recalled.getStep(0).probability==100&&recalled.getStep(0).ratchets==1&&recalled.getStep(0).timing==0,"old presets reset new controls and preserve original step behavior");
        legacyFile.deleteFile();
        const auto renderMacroMorph=[](bool macro,bool morph)
        {
            auto owner=std::make_unique<GAcidBaseProcessor>();auto& p=*owner;p.setCurrentProgram(46);p.setParameter("noise",0);p.setParameter("envMod",0);p.setParameter("bypass",1);
            if(macro){p.assignMacro(0,parameterIndex(p,"cutoff"),.6f);p.setParameter("macro1",1);}
            if(morph){p.captureMorph(0);p.setParameter("cutoff",10000);p.captureMorph(1);p.setParameter("cutoff",200);p.setParameter("morph",1);}
            p.prepareToPlay(48000,64);juce::AudioBuffer<float> audio(2,8192);juce::MidiBuffer events;events.addEvent(juce::MidiMessage::noteOn(1,36,.7f),0);p.processBlock(audio,events);return audio.getRMSLevel(0,0,8192);
        };
        const float baseSound=renderMacroMorph(false,false);
        require(std::abs(renderMacroMorph(true,false)-baseSound)>.0001f,"assigned macro changes audible DSP");
        require(std::abs(renderMacroMorph(false,true)-baseSound)>.0001f,"sound morph endpoint changes audible DSP");
        // Probability and ratchet scheduling must affect the actual engine, not just MIDI export.
        const auto renderSequence=[](int chance,int repeats,int block)
        {
            auto owner=std::make_unique<GAcidBaseProcessor>();auto& p=*owner;p.setCurrentProgram(46);p.setParameter("mode",1);p.setParameter("run",1);p.setParameter("bypass",1);p.setParameter("noise",0);
            for(int n=0;n<16;++n)p.setStep(n,{0,true,false,false,chance,repeats,0,false});
            p.prepareToPlay(48000,block);juce::AudioBuffer<float> result(2,12000),chunk(2,block);juce::MidiBuffer midi;
            for(int offset=0;offset<12000;offset+=block)
            {
                const int count=juce::jmin(block,12000-offset);chunk.setSize(2,count,false,false,true);p.processBlock(chunk,midi);
                for(int c=0;c<2;++c)result.copyFrom(c,offset,chunk,c,0,count);
            }
            return result;
        };
        require(renderSequence(0,1,64).getMagnitude(0,0,12000)<.000001f,"zero-chance sequence produces silence");
        auto timingOwner=std::make_unique<GAcidBaseProcessor>();auto& timing=*timingOwner;timing.setParameter("mode",1);timing.setParameter("run",1);timing.setParameter("swing",0);timing.setParameter("bypass",1);
        for(int i=0;i<16;++i)timing.setStep(i,{0,true,false,false,100,1,25,false});
        timing.prepareToPlay(48000,64);juce::AudioBuffer<float> timingAudio(2,1024);juce::MidiBuffer timingMidi;timing.processBlock(timingAudio,timingMidi);
        require(timingAudio.getMagnitude(0,0,900)<.000001f,"delayed first step does not trigger phantom prior-bar note");
        const auto single=renderSequence(100,1,64),ratcheted=renderSequence(100,4,64),oddBlock=renderSequence(100,4,511);
        difference=0;float blockDifference=0;
        for(int n=0;n<12000;++n){difference+=std::abs(single.getSample(0,n)-ratcheted.getSample(0,n));blockDifference=juce::jmax(blockDifference,std::abs(ratcheted.getSample(0,n)-oddBlock.getSample(0,n)));}
        require(difference>.1f,"ratchets retrigger rendered sequence");        require(blockDifference<.0001f,"sequence output is independent of host block partition");
        struct TestPlayHead : juce::AudioPlayHead
        {
            double bpm=120,ppq=0;bool playing=true;
            juce::Optional<PositionInfo> getPosition() const override
            {PositionInfo info;info.setBpm(bpm);info.setPpqPosition(ppq);info.setIsPlaying(playing);return info;}
        };
        auto hostOwner=std::make_unique<GAcidBaseProcessor>();auto& host=*hostOwner;TestPlayHead transport;
        host.setPlayHead(&transport);host.setParameter("mode",1);host.setParameter("run",1);host.setParameter("swing",0);host.prepareToPlay(48000,64);
        juce::AudioBuffer<float> hostAudio(2,64);juce::MidiBuffer hostMidi;
        transport.ppq=8;host.processBlock(hostAudio,hostMidi);require(host.activeStep==0&&host.hostTempo==120,"sequence uses host tempo and PPQ");
        transport.ppq=1;host.processBlock(hostAudio,hostMidi);require(host.activeStep==4,"backward host seek resynchronizes sequence");
        transport.ppq=-.25;host.processBlock(hostAudio,hostMidi);require(host.activeStep==15,"negative host PPQ remains valid");
        host.setPlayHead(nullptr);
        // The RUN latch is the sequencer's play button. The shipped bug: it lit
        // up while nothing played - RUN in plain MIDI mode did nothing at all,
        // and choosing a preset reset Play mode to MIDI while the latch stayed
        // lit, stalling a sequence that had been running.
        {
            auto owner=std::make_unique<GAcidBaseProcessor>();auto& p=*owner;
            p.setCurrentProgram(0);p.setParameter("bypass",1);
            require(static_cast<int>(p.value("mode"))==0,"factory presets load in plain MIDI mode");
            p.setParameter("mode",2);p.setParameter("run",1);
            require(static_cast<int>(p.value("mode"))==2,"RUN engaged in ARP mode stays in ARP mode");
            p.setParameter("mode",0);
            require(p.value("run")<.5f,"choosing MIDI play mode releases the RUN latch");
            p.setParameter("run",1);
            require(static_cast<int>(p.value("mode"))==1,"engaging RUN from MIDI mode selects the sequencer");
            p.prepareToPlay(48000,64);
            juce::AudioBuffer<float> audio(2,4096);juce::MidiBuffer midi;
            int changes=0,previous=p.activeStep.load();float heard=0;
            for(int block=0;block<12;++block)
            {
                p.processBlock(audio,midi);
                const int now=p.activeStep.load();if(now!=previous){++changes;previous=now;}
                heard=juce::jmax(heard,audio.getMagnitude(0,0,4096));
            }
            require(changes>=3&&heard>.001f,"RUN pressed in MIDI mode advances steps and makes sound");
            p.setCurrentProgram(23);
            require(p.value("run")>.5f&&static_cast<int>(p.value("mode"))==1,"choosing a preset keeps the play mode and the RUN latch");
            changes=0;previous=p.activeStep.load();heard=0;
            for(int block=0;block<12;++block)
            {
                p.processBlock(audio,midi);
                const int now=p.activeStep.load();if(now!=previous){++changes;previous=now;}
                heard=juce::jmax(heard,audio.getMagnitude(0,0,4096));
            }
            require(changes>=3&&heard>.001f,"the sequence keeps playing across preset changes");
        }
        // Dense host automation remains finite over representative sample rates and odd blocks.
        for(double sr:{44100.,48000.,96000.})
        {
            host.setParameter("mode",0);host.setParameter("run",0);host.setParameter("midiChannel",2);host.prepareToPlay(sr,511);
            hostAudio.setSize(2,511);hostMidi.addEvent(juce::MidiMessage::noteOn(1,36,1.f),0);host.processBlock(hostAudio,hostMidi);
            require(hostAudio.getMagnitude(0,0,511)<.000001f,"selected MIDI channel rejects wrong-channel note");
            hostMidi.addEvent(juce::MidiMessage::noteOn(2,36,1.f),0);
            for(int block=0;block<20;++block)
            {
                host.setParameter("cutoff",block%2?16000.f:30.f);host.setParameter("drive",block%2?30.f:0.f);host.setParameter("reverbMix",block%2?.7f:0.f);host.setParameter("macro1",block%2?1.f:0.f);
                host.processBlock(hostAudio,hostMidi);
                for(int c=0;c<2;++c)for(int n=0;n<511;++n)
                {
                    const float sample=hostAudio.getSample(c,n);const bool safe=std::isfinite(sample)&&std::abs(sample)<=.941f;
                    if(!safe)std::cerr<<"Automation sr="<<sr<<" block="<<block<<" sample="<<sample<<" limiter="<<host.value("limiter")<<" bypass="<<host.value("bypass")<<std::endl;
                    require(safe,"dense automation stays finite and limited");
                }
            }
        }
        // Logo: the vector mark in the header must match the shipped raster, and the
        // header lockup must not collide with the buttons that follow it.
        constexpr int logoSize = 512;
        juce::Image vectorMark(juce::Image::ARGB, logoSize, logoSize, true);
        {
            juce::Graphics g(vectorMark);
            g.setColour(juce::Colours::transparentBlack);
            GAcidLogo::paint(g,{0.f,0.f,static_cast<float>(logoSize),static_cast<float>(logoSize)});
        }
        if(auto vectorStream=juce::File::getCurrentWorkingDirectory().getChildFile("artifacts/G-AcidBase-Logo-Vector-512.png").createOutputStream())
            juce::PNGImageFormat().writeImageToStream(vectorMark,*vectorStream);
        const auto rasterFile=juce::File::getCurrentWorkingDirectory().getChildFile("assets/G-AcidBase-Logo-512.png");
        require(rasterFile.existsAsFile(),"generated 512 px logo raster is present");
        juce::Image raster;
        auto rasterStream=std::unique_ptr<juce::InputStream>(rasterFile.createInputStream());
        juce::PNGImageFormat pngFormat;
        if(rasterStream)raster=pngFormat.decodeImage(*rasterStream);
        require(raster.isValid(),"logo raster decodes as a PNG");
        require(raster.getWidth()==logoSize&&raster.getHeight()==logoSize&&raster.hasAlphaChannel(),"logo raster is 512x512 with transparency");
        require(raster.getPixelAt(2,2).getAlpha()==0&&raster.getPixelAt(logoSize/2,logoSize/2).getAlpha()==255,"logo raster keeps rounded corners transparent and the badge opaque");
        const auto classify=[&](juce::Colour colour)
        {
            int best=-1;float bestDistance=1e9f;
            const auto palette=GAcidLogo::palette();
            for(int index=0;index<5;++index)
            {
                const auto candidate=juce::Colour(palette[static_cast<size_t>(index)]);
                const float distance=std::abs(colour.getRed()-candidate.getRed())+std::abs(colour.getGreen()-candidate.getGreen())
                                    +std::abs(colour.getBlue()-candidate.getBlue())+std::abs(colour.getAlpha()-candidate.getAlpha());
                if(distance<bestDistance){bestDistance=distance;best=index;}
            }
            return std::pair<int,float> { best, bestDistance };
        };
        // Two independent rasterisers (JUCE and the generator) must produce the same mark:
        // near-identical pixels plus every brand colour present in both images.
        int matching=0,vectorPixels[5]{},rasterPixels[5]{};
        int worstDelta=0;
        for(int y=0;y<logoSize;++y)for(int x=0;x<logoSize;++x)
        {
            const auto vectorPixel=vectorMark.getPixelAt(x,y),rasterPixel=raster.getPixelAt(x,y);
            const auto delta=[&](juce::Colour a,juce::Colour b)
            {
                return std::abs((int)a.getRed()-(int)b.getRed())+std::abs((int)a.getGreen()-(int)b.getGreen())
                     + std::abs((int)a.getBlue()-(int)b.getBlue())+std::abs((int)a.getAlpha()-(int)b.getAlpha());
            };
            const int difference=delta(vectorPixel,rasterPixel);
            worstDelta=juce::jmax(worstDelta,difference);
            if(difference<=90)++matching;
            const auto [vectorClass,vectorError]=classify(vectorPixel);
            const auto [rasterClass,rasterError]=classify(rasterPixel);
            if(vectorError<=30)++vectorPixels[vectorClass];
            if(rasterError<=30)++rasterPixels[rasterClass];
        }
        const auto pixels=logoSize*logoSize;
        const float agreement=static_cast<float>(matching)/static_cast<float>(pixels);
        std::cout<<"Logo raster agreement: "<<juce::String(agreement*100.f,2)<<"% of "<<pixels<<" pixels (worst channel delta "<<worstDelta<<")\n";
        require(agreement>.99f,"JUCE vector logo matches the packaged raster geometry");
        for(int index=0;index<5;++index)
        {
            std::cout<<"  logo class "<<index<<": vector "<<vectorPixels[index]<<" raster "<<rasterPixels[index]<<" pixels\n";
            require(vectorPixels[index]>1000&&rasterPixels[index]>1000,
                    juce::String("logo palette class ")+juce::String(index)+" is present in both renders");
        }
        auto modulationOwner=std::make_unique<GAcidBaseProcessor>("http://127.0.0.1:1/nowhere");auto& modulation=*modulationOwner;
        std::unique_ptr<juce::AudioProcessorEditor> editor(modulation.createEditor());
        auto* acid=dynamic_cast<GAcidBaseEditor*>(editor.get());
        require(acid!=nullptr,"editor creates for DPI checks");
        juce::MidiKeyboardComponent* piano=nullptr;
        for(int i=0;i<editor->getChildComponent(0)->getNumChildComponents();++i)
            if(auto* keyboard=dynamic_cast<juce::MidiKeyboardComponent*>(editor->getChildComponent(0)->getChildComponent(i)))piano=keyboard;
        require(piano!=nullptr,"native piano keyboard exists");
        require(piano->getRangeStart()==24&&piano->getRangeEnd()==84,"keyboard cleanup preserves playable note range");
        require(GAcidLogo::badgeInset==.02f&&GAcidLogo::badgeRadius==.22f&&GAcidLogo::badgeEdgeWidth==.035f
                &&GAcidLogo::waveWidth==.05f&&GAcidLogo::waveBaseline==.44f&&GAcidLogo::wavePeak==.19f
                &&GAcidLogo::waveLeft==.13f&&GAcidLogo::waveRight==.87f&&GAcidLogo::waveCycles==3
                &&GAcidLogo::slopeWidth==.038f&&GAcidLogo::slopeDotRadius==.055f
                &&GAcidLogo::knobRadius==.088f&&GAcidLogo::knobRingWidth==.036f&&GAcidLogo::knobNotchWidth==.042f
                &&GAcidLogo::slopeStart==juce::Point<float>(.13f,.54f)&&GAcidLogo::slopeEnd==juce::Point<float>(.87f,.72f)
                &&GAcidLogo::knobCentre==juce::Point<float>(.22f,.79f)&&GAcidLogo::knobNotch==juce::Point<float>(.282f,.728f),
                "logo geometry constants stay in step with scripts/make_logo.py");
        require(GAcidLogo::palette()[0]==0xff10161f&&GAcidLogo::palette()[1]==0xff33425a&&GAcidLogo::palette()[2]==0xffb8ff38
                &&GAcidLogo::palette()[3]==0xff42e8da&&GAcidLogo::palette()[4]==0xffb290ff,
                "logo palette matches the generated assets");
        // Animated header mark: the host tempo drives one cycle per beat and the resonance
        // parameter opens up the filter slope. The shipped geometry must survive the animation,
        // so motion is confined to the travelling saw and to the filter slope.
        const auto renderMark=[](const GAcidLogo::AnimationState& state)
        {
            juce::Image image(juce::Image::ARGB,128,128,true);
            { juce::Graphics graphics(image); GAcidLogo::paint(graphics,{0.f,0.f,128.f,128.f},state); }
            return image;
        };
        juce::Image staticMark(juce::Image::ARGB,128,128,true);
        { juce::Graphics graphics(staticMark); GAcidLogo::paint(graphics,{0.f,0.f,128.f,128.f}); }
        const auto movedPixels=[](const juce::Image& a,const juce::Image& b,juce::Rectangle<int> only={})
        {
            int moved=0;
            for(int y=0;y<a.getHeight();++y)for(int x=0;x<a.getWidth();++x)
            {
                if(!only.isEmpty()&&!only.contains(x,y))continue;
                const auto first=a.getPixelAt(x,y),second=b.getPixelAt(x,y);
                if(std::abs((int)first.getRed()-(int)second.getRed())+std::abs((int)first.getGreen()-(int)second.getGreen())
                  +std::abs((int)first.getBlue()-(int)second.getBlue())+std::abs((int)first.getAlpha()-(int)second.getAlpha())>12)++moved;
            }
            return moved;
        };
        const juce::Rectangle<int> sawBand(0,static_cast<int>(GAcidLogo::wavePeak*128.f)-3,128,
                                            static_cast<int>(GAcidLogo::waveBaseline*128.f)-static_cast<int>(GAcidLogo::wavePeak*128.f)+7);
        const juce::Rectangle<int> filterBand(0,static_cast<int>(GAcidLogo::slopeStart.y*128.f)-5,128,128);
        require(movedPixels(renderMark(GAcidLogo::AnimationState{.5f,0.f}),renderMark(GAcidLogo::AnimationState{.5f,0.f}))==0,
                "animated logo is deterministic for the same state");
        require(movedPixels(renderMark(GAcidLogo::AnimationState{.5f,0.f}),staticMark,sawBand)==movedPixels(renderMark(GAcidLogo::AnimationState{.5f,0.f}),staticMark),
                "between beats the animated logo keeps the shipped geometry outside the travelling saw");
        require(movedPixels(renderMark(GAcidLogo::AnimationState{0.f,0.f}),renderMark(GAcidLogo::AnimationState{.5f,0.f}))>400,
                "the logo pulses on the beat");
        require(movedPixels(renderMark(GAcidLogo::AnimationState{.25f,0.f}),renderMark(GAcidLogo::AnimationState{.5f,0.f}),sawBand)>250,
                "the saw echo travels once per beat");
        require(movedPixels(renderMark(GAcidLogo::AnimationState{.5f,1.f}),renderMark(GAcidLogo::AnimationState{.5f,0.f}))>200,
                "resonance opens up the filter slope in the logo");
        require(movedPixels(renderMark(GAcidLogo::AnimationState{.5f,1.f}),renderMark(GAcidLogo::AnimationState{.5f,0.f}),filterBand)
                  ==movedPixels(renderMark(GAcidLogo::AnimationState{.5f,1.f}),renderMark(GAcidLogo::AnimationState{.5f,0.f})),
                "resonance moves the filter slope only, leaving the rest of the mark alone");
        for(const auto state:{GAcidLogo::AnimationState{.25f,0.f},GAcidLogo::AnimationState{0.f,0.f},GAcidLogo::AnimationState{.75f,1.f}})
        {
            const auto mark=renderMark(state);
            const auto reach=64.f*(1.f+GAcidLogo::beatGrowth)+2.f;
            int outside=0;
            for(int y=0;y<128;++y)for(int x=0;x<128;++x)
                if(mark.getPixelAt(x,y).getAlpha()>0&&(std::abs(x-63.5f)>reach||std::abs(y-63.5f)>reach))++outside;
            require(outside==0,"animated logo never spills outside the badge");
        }
        {   // Contact sheet of the motion: one beat across the top, the same beat with the filter
            // wide open underneath, so the animation can be reviewed without a running plugin.
            const float phases[] { 0.f, .125f, .25f, .375f, .5f };
            juce::Image sheet(juce::Image::ARGB,128*5,128*2,true);
            for(size_t index=0;index<5;++index)for(int row=0;row<2;++row)
            {
                const auto mark=renderMark(GAcidLogo::AnimationState{phases[index],row==0?0.f:1.f});
                for(int y=0;y<128;++y)for(int x=0;x<128;++x)
                    if(mark.getPixelAt(x,y).getAlpha()>0)
                        sheet.setPixelAt(static_cast<int>(index)*128+x,row*128+y,mark.getPixelAt(x,y));
            }
            if(auto stream=juce::File::getCurrentWorkingDirectory().getChildFile("artifacts/G-AcidBase-Logo-Animation.png").createOutputStream())
                juce::PNGImageFormat().writeImageToStream(sheet,*stream);
        }
        // Phase maths: one cycle per host tempo beat, wrapped, and independent of the display refresh.
        require(std::abs(GAcidLogo::advance(GAcidLogo::AnimationState{0.f,0.f},120,.5).phase)<1e-6f,
                "120 BPM advances one full beat in half a second");
        require(std::abs(GAcidLogo::advance(GAcidLogo::AnimationState{0.f,0.f},60,.5).phase-.5f)<1e-6f,
                "60 BPM advances half a beat in half a second");
        require(std::abs(GAcidLogo::advance(GAcidLogo::AnimationState{.25f,0.f},120,.25).phase-.75f)<1e-6f,
                "the phase keeps its position within the beat");
        require(GAcidLogo::advance(GAcidLogo::AnimationState{.9f,0.f},120,1.).phase>.89f,
                "the phase stays wrapped after many beats");
        require(GAcidLogo::advance(GAcidLogo::AnimationState{0.f,0.f},0,1.).phase==0.f,
                "a stopped transport holds the logo still");
        require(GAcidLogo::resonanceAmount(0.f)==0.f&&std::abs(GAcidLogo::resonanceAmount(.99f)-1.f)<1e-6f,
                "resonance maps the parameter range to 0..1");
        require(GAcidLogo::resonanceAmount(-1.f)==0.f&&GAcidLogo::resonanceAmount(2.f)==1.f,
                "resonance amount is clamped");
        // The editor must drive that state from the live host clock and the resonance parameter.
        const auto resting=acid->getLogoAnimation();
        pumpMessagesFor(140);
        require(std::abs(acid->getLogoAnimation().phase-resting.phase)>1e-6f,
                "the header logo advances with the host clock");
        modulation.setParameter("resonance",.99f);pumpMessagesFor(140);
        require(acid->getLogoAnimation().resonance>.9f,
                "the header logo follows the resonance parameter");
        modulation.setParameter("resonance",.68f);
        // Update check: the version comparison and every word of the result
        // dialog are decided as pure functions, so this needs no network and no
        // window - the fetch thread only ever calls in to them.
        require(gacid::versionString()=="1.1.1","the built version is the one CMake's project() names");
        require(gacid::compareVersions("1.1.0","1.1.0")==0,"equal versions compare equal");
        require(gacid::compareVersions("1.1.0","1.2.0")<0&&gacid::compareVersions("1.2.0","1.1.0")>0,"a minor release is offered");
        require(gacid::compareVersions("1.1.0","1.1.1")<0,"a patch release is offered too");
        require(gacid::compareVersions("1.1.0","v1.2")>0,"a malformed feed version cannot outrank the build");
        require(gacid::installerUrl("1.2.0")=="https://github.com/DakaGoa/G-AcidBase/releases/download/v1.2.0/G-AcidBase-Setup-1.2.0.exe",
                "the download link points at that release's installer, named for its version");
        require(gacid::installerAssetName("1.2.0")=="G-AcidBase-Setup-1.2.0.exe",
                "the installer asset name carries the version it installs");
        require(gacid::releaseNotesUrl("1.2.0")=="https://dakagoa.github.io/GoaSynth/gacidbase/#v1.2.0",
                "the notes link deep-links to the site's matching release heading");
        require(gacid::changeLogUrl=="https://dakagoa.github.io/GoaSynth/gacidbase/#changelog",
                "Change Log opens the full site history");
        const auto feed=[](const juce::String& json){return juce::JSON::parse(json);};
        const auto full=gacid::readFeed(feed("{\"latest\":\"9.9.9\",\"released\":\"5 October 2026\",\"installer_size\":\"4.5 MB\",\"notes\":[\"first\",\"\",\"  \",\"second\",\"third\",\"fourth\",\"fifth\"]}"),gacid::versionString());
        require(full.reachable&&full.newer,"a newer feed is reachable and offers an update");
        require(full.notes.size()==4&&full.notes[0]=="first"&&full.notes[1]=="second"&&full.notes[3]=="fourth",
                "the notes shown are the first four non-empty lines");
        const auto message=gacid::describe(full,gacid::versionString());
        require(message.title=="UPDATE AVAILABLE"&&message.message.contains("G-AcidBase 9.9.9 is available"),
                "the update dialog names both versions");
        require(message.message.contains("Released 5 October 2026.")&&message.message.contains("What's new:")
                &&message.message.contains("4.5 MB")&&message.message.contains("first"),
                "the update dialog carries the date, the notes and the download size");
        require(message.downloadUrl==gacid::installerUrl("9.9.9")&&message.downloadLabel==gacid::installerAssetName("9.9.9")
                &&message.notesUrl==gacid::releaseNotesUrl("9.9.9"),
                "the update dialog links the installer and the release notes");
        const auto bare=gacid::describe(gacid::readFeed(feed("{\"latest\":\"9.9.9\"}"),gacid::versionString()),gacid::versionString());
        require(!bare.message.contains("Released ")&&!bare.message.contains("What's new:"),
                "a feed with no date or notes leaves those lines out");
        require(bare.message.contains("Download the new installer:"),"the download row is offered even without a size");
        require(!gacid::readFeed(feed("not json"),gacid::versionString()).reachable,"an error page is not a feed");
        const auto dead=gacid::describe(gacid::readFeed(feed("{\"latest\":\"\"}"),gacid::versionString()),gacid::versionString());
        require(dead.title=="G-ACIDBASE"&&dead.message.contains("Could not reach the update feed")&&dead.downloadUrl==gacid::siteUrl
                &&dead.notesUrl.isEmpty(),"an unreachable feed says so and points at the site");
        const auto current=gacid::describe(gacid::readFeed(feed("{\"latest\":\"1.1.0\"}"),gacid::versionString()),gacid::versionString());
        require(current.message.contains("You are running the latest version")&&current.downloadUrl.isEmpty()
                &&current.notesUrl==gacid::releaseNotesUrl(gacid::versionString()),
                "an up-to-date build links its own site notes and has nothing to download");
        // The dialog itself: the rows sit inside the card, the download row
        // names its file, and the card hides when there is nothing to offer.
        gacid::UpdateResultOverlay overlay;
        overlay.setBounds(0,0,1280,940);
        overlay.configure("TITLE","body text","https://example.invalid/file.zip","https://example.invalid/notes","file.zip");
        require(overlay.downloadLink.isVisible()&&overlay.notesLink.isVisible(),"both link rows show when both are offered");
        require(overlay.cardBounds().contains(overlay.titleLabel.getBounds())&&overlay.cardBounds().contains(overlay.messageLabel.getBounds())
                &&overlay.cardBounds().contains(overlay.downloadLink.getBounds())&&overlay.cardBounds().contains(overlay.notesLink.getBounds())
                &&overlay.cardBounds().contains(overlay.okButton.getBounds())&&overlay.cardBounds().contains(overlay.closeButton.getBounds()),
                "every row of the update dialog sits inside its card");
        require(overlay.downloadLink.getButtonText()=="file.zip"&&overlay.downloadLink.getURL().toString(true)=="https://example.invalid/file.zip",
                "the download row names the file and links the address");
        overlay.configure("TITLE","body text",{},gacid::releaseNotesUrl("1.1.0"));
        require(!overlay.downloadLink.isVisible()&&overlay.notesLink.isVisible(),"the download row hides when there is nothing to download");
        overlay.configure("TITLE","body text",{},{});
        require(!overlay.downloadLink.isVisible()&&!overlay.notesLink.isVisible(),"an unreachable-feed dialog offers no second door");
        overlay.setVisible(true);overlay.okButton.triggerClick();pumpMessagesFor(40);   // Button::triggerClick posts a command message
        require(!overlay.isVisible(),"the update dialog closes from its OK button");
        overlay.setVisible(true);
        overlay.mouseDown(juce::MouseEvent(*juce::Desktop::getInstance().getMouseSource(0),juce::Point<float>(4.f,4.f),juce::ModifierKeys(),0.f,0.f,0.f,0.f,0.f,&overlay,&overlay,juce::Time::getCurrentTime(),juce::Point<float>(4.f,4.f),juce::Time::getCurrentTime(),1,false));
        require(!overlay.isVisible(),"clicking the backdrop dismisses the update dialog");
        juce::Image dialogShot(juce::Image::ARGB,640,470,true);
        { juce::Graphics g(dialogShot); overlay.setBounds(0,0,640,470); overlay.configure("G-ACIDBASE","body text",{},{}); overlay.paint(g); }
        require(dialogShot.getPixelAt(2,2).getRed()<20&&dialogShot.getPixelAt(320,235).getRed()>15,
                "the update dialog darkens the backdrop and draws its card");
        // The address the plugin polls is the one the site publishes.
        require(juce::String(gacid::updateFeedUrl)==juce::String(gacid::siteUrl)+"version.json",
                "the update feed sits beside the product page");
        // The fetch itself, end to end: a local server answers the very GET the
        // button makes, and the outcome arrives through the same callback the
        // editor receives. No network and no DAW - just the whole path from
        // "user clicked" to "outcome parsed".
        {
            struct FeedServer : juce::Thread
            {
                FeedServer(const juce::String& body,int portNumber)
                    : juce::Thread("feed server"),payload(body),port(portNumber) {}
                void run() override
                {
                    juce::StreamingSocket listener;
                    if(!listener.createListener(port,"127.0.0.1")){failed=true;return;}
                    std::unique_ptr<juce::StreamingSocket> client(listener.waitForNextConnection());
                    if(client==nullptr){failed=true;listener.close();return;}
                    char request[2048];
                    // StreamingSocket::read's third argument is shouldBlock, not a timeout:
                    // wait for the request to land, then take whatever is there.
                    client->waitUntilReady(true,5000);
                    client->read(request,static_cast<int>(sizeof(request)),false);
                    const juce::String header="HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: "
                                             +juce::String(payload.getNumBytesAsUTF8())+"\r\nConnection: close\r\n\r\n";
                    client->write(header.toRawUTF8(),static_cast<int>(header.getNumBytesAsUTF8()));
                    client->write(payload.toRawUTF8(),static_cast<int>(payload.getNumBytesAsUTF8()));
                    client->close();listener.close();
                }
                juce::String payload;int port;bool failed=false;
            };
            constexpr int port=48917;
            FeedServer server("{\"latest\":\"9.9.9\",\"released\":\"tomorrow\",\"installer_size\":\"1 MB\",\"notes\":[\"one\"]}",port);
            server.startThread();
            std::atomic<bool> answered { false };
            gacid::UpdateOutcome fetched;
            gacid::UpdateCheckThread check("http://127.0.0.1:"+juce::String(port)+"/version.json",
                [&](const gacid::UpdateOutcome& outcome)
                {
                    fetched=outcome;   // written before the flag, read after it
                    answered.store(true);
                });
            check.startThread();
            const auto deadline=GetTickCount64()+8000;
            while(!answered.load()&&GetTickCount64()<deadline)pumpMessagesFor(50);
            check.stopThread(4000);server.stopThread(4000);
            require(!server.failed,"the local feed server accepts the plugin's request");
            require(answered.load(),"the update check always reports back");
            require(fetched.reachable&&fetched.newer&&fetched.latestVersion=="9.9.9"
                    &&fetched.released=="tomorrow"&&fetched.packageSize=="1 MB"&&fetched.notes.size()==1&&fetched.notes[0]=="one",
                    "the real fetch path delivers the feed to the caller"
                    " (reachable="+juce::String(fetched.reachable?1:0)+" newer="+juce::String(fetched.newer?1:0)
                    +" version="+fetched.latestVersion+" released="+fetched.released+" size="+fetched.packageSize
                    +" notes="+juce::String(fetched.notes.size())+" serverFailed="+juce::String(server.failed?1:0)+")");
            // A server that answers 404 is "cannot check", never "up to date".
            std::atomic<bool> answeredAgain { false };
            gacid::UpdateOutcome missing;
            gacid::UpdateCheckThread denied("http://127.0.0.1:1/nowhere",
                [&](const gacid::UpdateOutcome& outcome)
                {
                    missing=outcome;answeredAgain.store(true);
                });
            denied.startThread();
            const auto retry=GetTickCount64()+8000;
            while(!answeredAgain.load()&&GetTickCount64()<retry)pumpMessagesFor(50);
            denied.stopThread(4000);
            require(answeredAgain.load()&&!missing.reachable,
                    "a feed that cannot be fetched reports unreachable");
        }
        // Automatic checks run once per processor session, survive a closed
        // editor, and only show a card for newer versions. Drive actual editor
        // timers/buttons against a loopback feed; no live internet dependency.
        {
            struct SessionFeedServer : juce::Thread
            {
                explicit SessionFeedServer(juce::String body)
                    : juce::Thread("session feed server"),payload(std::move(body))
                {
                    require(listener.createListener(0,"127.0.0.1"),"session feed listener starts");
                    port=listener.getBoundPort();
                    startThread();
                }
                ~SessionFeedServer() override { signalThreadShouldExit();stopThread(2000); }
                void run() override
                {
                    while(!threadShouldExit())
                    {
                        if(listener.waitUntilReady(true,100)<=0)continue;
                        std::unique_ptr<juce::StreamingSocket> client(listener.waitForNextConnection());
                        if(client==nullptr)continue;
                        if(client->waitUntilReady(true,1000)<=0)continue;
                        char request[2048];client->read(request,static_cast<int>(sizeof(request)),false);
                        const juce::String response="HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: "
                            +juce::String(payload.getNumBytesAsUTF8())+"\r\nConnection: close\r\n\r\n"+payload;
                        client->write(response.toRawUTF8(),static_cast<int>(response.getNumBytesAsUTF8()));
                        requests.fetch_add(1);
                    }
                }
                juce::String url() const { return "http://127.0.0.1:"+juce::String(port)+"/version.json"; }
                juce::StreamingSocket listener;
                juce::String payload;
                int port=0;
                std::atomic<int> requests {0};
            };
            const auto waitFor=[&](const std::function<bool()>& done)
            {
                const auto deadline=GetTickCount64()+6000;
                while(!done()&&GetTickCount64()<deadline)pumpMessagesFor(20);
                require(done(),"session update check completes within deadline");
            };
            const auto resultCard=[](juce::AudioProcessorEditor& view)
            {
                auto* surface=view.getChildComponent(0);
                return dynamic_cast<gacid::UpdateResultOverlay*>(surface->findChildWithID("updateResult"));
            };
            const auto manualButton=[](juce::Component& view, const auto& self)->juce::Button*
            {
                if(view.getComponentID()=="checkUpdates")return dynamic_cast<juce::Button*>(&view);
                for(int i=0;i<view.getNumChildComponents();++i)
                    if(auto* found=self(*view.getChildComponent(i),self))return found;
                return nullptr;
            };
            for(const juce::String& latest:{juce::String("9.9.9"),gacid::versionString(),juce::String("invalid")})
            {
                SessionFeedServer server("{\"latest\":\""+latest+"\"}");
                auto owner=std::make_unique<GAcidBaseProcessor>(server.url());
                std::unique_ptr<juce::AudioProcessorEditor> view(owner->createEditor());
                auto* card=resultCard(*view);require(card!=nullptr,"editor parents the update result card");
                waitFor([&]{return owner->sessionUpdates.isComplete();});pumpMessagesFor(100);
                const bool newer=latest=="9.9.9";
                require(card->isVisible()==newer,"automatic result is visible only for a newer release");
                if(newer)
                {
                    require(card->titleLabel.getText()=="UPDATE AVAILABLE"
                            &&card->downloadLink.getURL().toString(true)==gacid::installerUrl(latest),
                            "automatic update card links the offered release");
                    card->okButton.triggerClick();pumpMessagesFor(60);
                }
                require(server.requests.load()==1,"first editor opening makes one automatic GET");
                require(!owner->sessionUpdates.startOnce(),"a completed automatic check cannot restart");
                view.reset();view.reset(owner->createEditor());pumpMessagesFor(120);
                card=resultCard(*view);
                require(server.requests.load()==1&&!card->isVisible(),"editor reopening neither fetches nor repeats a notification");
                owner->setCurrentProgram(4);owner->prepareToPlay(48000,64);pumpMessagesFor(60);
                require(server.requests.load()==1,"preset changes and audio preparation do not reset the session check");
                auto* manual=manualButton(*view,manualButton);require(manual!=nullptr,"manual check button is available");
                manual->triggerClick();pumpMessagesFor(40);
                waitFor([&]{return card->isVisible();});
                require(server.requests.load()==2,"manual check still makes a fresh request");
                require(card->titleLabel.getText()==(newer?"UPDATE AVAILABLE":"G-ACIDBASE"),
                        "manual check still shows newer, up-to-date and unreachable outcomes");
                card->okButton.triggerClick();pumpMessagesFor(60);
                require(!card->isVisible(),"manual notification does not resurrect an automatic one");
            }
            // Close before completion: the processor owns the worker/result.
            SessionFeedServer server("{\"latest\":\"9.9.9\"}");
            auto owner=std::make_unique<GAcidBaseProcessor>(server.url());
            std::unique_ptr<juce::AudioProcessorEditor> view(owner->createEditor());
            view.reset();waitFor([&]{return owner->sessionUpdates.isComplete();});
            view.reset(owner->createEditor());pumpMessagesFor(100);
            require(server.requests.load()==1&&resultCard(*view)->isVisible(),
                    "a newer result fetched while closed appears on reopen without another request");
            view.reset();owner.reset();
            auto nextSession=std::make_unique<GAcidBaseProcessor>(server.url());
            view.reset(nextSession->createEditor());
            waitFor([&]{return nextSession->sessionUpdates.isComplete();});pumpMessagesFor(100);
            require(server.requests.load()==2&&resultCard(*view)->isVisible(),"a fresh processor session checks again");
            view.reset();nextSession.reset();
            auto offline=std::make_unique<GAcidBaseProcessor>("http://127.0.0.1:1/nowhere");
            view.reset(offline->createEditor());
            waitFor([&]{return offline->sessionUpdates.isComplete();});pumpMessagesFor(100);
            require(!resultCard(*view)->isVisible(),"a failed automatic connection stays silent");
            auto* manual=manualButton(*view,manualButton);require(manual!=nullptr,"offline manual check button exists");
            manual->triggerClick();pumpMessagesFor(40);
            waitFor([&]{return resultCard(*view)->isVisible();});
            require(resultCard(*view)->messageLabel.getText().contains("Could not reach the update feed"),
                    "a failed manual connection still explains the error");
            view.reset();
        }
        // The Change Log item is a native hyperlink with the full-history URL,
        // alongside Check for Updates on every TOOLS page, without overlap.
        juce::HyperlinkButton* changeLog=nullptr;
        juce::Component* checkUpdates=nullptr;
        juce::Component* featureClose=nullptr;
        std::function<void(juce::Component&)> findReleaseControls=[&](juce::Component& c)
        {
            if(c.getComponentID()=="changeLog")changeLog=dynamic_cast<juce::HyperlinkButton*>(&c);
            if(c.getComponentID()=="checkUpdates")checkUpdates=&c;
            if(c.getComponentID()=="featureClose")featureClose=&c;
            for(int i=0;i<c.getNumChildComponents();++i)findReleaseControls(*c.getChildComponent(i));
        };
        findReleaseControls(*editor);
        require(changeLog!=nullptr&&checkUpdates!=nullptr&&featureClose!=nullptr,
                "TOOLS contains Change Log, update check and Close controls");
        require(changeLog->getButtonText()=="CHANGE LOG"&&changeLog->getURL().toString(true)==gacid::changeLogUrl,
                "Change Log item uses the full-history website link");
        for(int page=0;page<4;++page)
        {
            acid->showFeaturePage(page);
            require(changeLog->isVisible()&&changeLog->getParentComponent()->isVisible(),
                    "Change Log remains available on every TOOLS page");
            require(changeLog->getParentComponent()->getLocalBounds().contains(changeLog->getBounds())
                    &&!changeLog->getBounds().intersects(checkUpdates->getBounds())
                    &&!changeLog->getBounds().intersects(featureClose->getBounds()),
                    "Change Log fits the tools header without overlapping adjacent actions");
        }
        acid->closePanels();
        // About card: the version it shows is the build's own, and each row goes to
        // exactly one address - the product page, the source repository, the
        // current release. Nothing here is read from the network.
        require(juce::String(gacid::repositoryUrl)=="https://github.com/DakaGoa/G-AcidBase"
                &&juce::String(gacid::releasesUrl)=="https://github.com/DakaGoa/G-AcidBase/releases/latest",
                "the About links name the project's repository and releases");
        gacid::AboutCard about;
        about.setBounds(0,0,1280,940);
        require(about.versionLabel.getText()=="Version "+gacid::versionString(),
                "the About card shows the running version");
        require(about.pageLink.getURL().toString(true)==gacid::siteUrl
                &&about.sourceLink.getURL().toString(true)==gacid::repositoryUrl
                &&about.releaseLink.getURL().toString(true)==gacid::releasesUrl,
                "each About row links its own address");
        require(about.pageLink.getButtonText()=="Product page"&&about.sourceLink.getButtonText()=="Source repository"
                &&about.releaseLink.getButtonText()=="Current release",
                "the About rows are named by what they are, not by their URL");
        require(about.cardBounds().contains(about.titleLabel.getBounds())&&about.cardBounds().contains(about.versionLabel.getBounds())
                &&about.cardBounds().contains(about.summaryLabel.getBounds())&&about.cardBounds().contains(about.licenceLabel.getBounds())
                &&about.cardBounds().contains(about.pageLink.getBounds())&&about.cardBounds().contains(about.sourceLink.getBounds())
                &&about.cardBounds().contains(about.releaseLink.getBounds())&&about.cardBounds().contains(about.okButton.getBounds())
                &&about.cardBounds().contains(about.closeButton.getBounds()),
                "every row of the About card sits inside its card");
        require(about.summaryLabel.getText().contains("50 factory presets")&&about.licenceLabel.getText().contains("JUCE"),
                "the About card says what this is and how JUCE is licensed");
        about.setVisible(true);about.okButton.triggerClick();pumpMessagesFor(40);
        require(!about.isVisible(),"the About card closes from its OK button");
        about.setVisible(true);
        about.mouseDown(juce::MouseEvent(*juce::Desktop::getInstance().getMouseSource(0),juce::Point<float>(4.f,4.f),juce::ModifierKeys(),0.f,0.f,0.f,0.f,0.f,&about,&about,juce::Time::getCurrentTime(),juce::Point<float>(4.f,4.f),juce::Time::getCurrentTime(),1,false));
        require(!about.isVisible(),"clicking the backdrop dismisses the About card");
        // The header's version line opens it, and Escape closes it again.
        juce::Button* aboutButton=nullptr;gacid::AboutCard* openCard=nullptr;
        std::function<void(juce::Component&)> findAbout=[&](juce::Component& c)
        {
            if(auto* b=dynamic_cast<juce::Button*>(&c);b!=nullptr&&b->getComponentID()=="about")aboutButton=b;
            if(auto* card=dynamic_cast<gacid::AboutCard*>(&c))openCard=card;
            for(int i=0;i<c.getNumChildComponents();++i)findAbout(*c.getChildComponent(i));
        };
        findAbout(*editor);
        require(aboutButton!=nullptr&&openCard!=nullptr,"the editor carries a version button and an About card");
        aboutButton->triggerClick();pumpMessagesFor(60);
        require(openCard->isVisible(),"the version button opens the About card");
        require(acid->keyPressed(juce::KeyPress(juce::KeyPress::escapeKey))&&!openCard->isVisible(),
                "Escape closes the About card");
        acid->setScaleFactor(1.5f);
        require(std::abs(editor->getTransform().getScaleFactor()-1.5f)<.001f,"host scale transform is preserved");
        for(const auto size:{juce::Point<int>(960,705),juce::Point<int>(1280,940),juce::Point<int>(1920,1410),juce::Point<int>(3840,2820),juce::Point<int>(5120,3760)})
        {
            editor->setSize(size.x,size.y);
            require(editor->getWidth()==size.x&&editor->getHeight()==size.y,"editor accepts 4K and host-scaled logical dimensions");
            const auto* surface=editor->getChildComponent(0);
            require(surface!=nullptr&&std::abs(surface->getTransform().getScaleFactor()-static_cast<float>(size.x)/1280.f)<.002f,"UI surface transform follows editor size once");
            require(std::abs(piano->getRectangleForKey(piano->getRangeStart()).getX())<.01f&&std::abs(piano->getRectangleForKey(piano->getRangeEnd()).getRight()-piano->getWidth())<.01f,"keyboard keys fill the whole width at every editor size");
        }
        editor->setSize(1280,940);acid->setScaleFactor(1.f);editor->setTopLeftPosition(0,0);
        const auto clickUI=[&](const juce::String& name)
        {
            juce::TextButton* found=nullptr;
            std::function<void(juce::Component&)> visit=[&](juce::Component& c)
            {
                if(auto* b=dynamic_cast<juce::TextButton*>(&c);b&&b->getButtonText()==name&&b->isShowing())found=b;
                for(int i=0;i<c.getNumChildComponents();++i)visit(*c.getChildComponent(i));
            };
            visit(*editor);require(found!=nullptr,"visible native button found");
            found->triggerClick();pumpMessagesFor(40);
        };
        editor->addToDesktop(juce::ComponentPeer::windowIsTemporary);editor->setVisible(true);
        pumpMessagesFor(50);
        const auto testWindow=static_cast<HWND>(editor->getPeer()->getNativeHandle());
        // Deliver a native right-click to the cutoff knob; dismiss the actual popup.
        auto* cutoffSlider=editor->findChildWithID("cutoff");
        std::function<juce::Component*(juce::Component&)> findCutoff=[&](juce::Component& c)->juce::Component*
        {
            if(c.getComponentID()=="cutoff")return &c;
            for(int i=0;i<c.getNumChildComponents();++i)if(auto* found=findCutoff(*c.getChildComponent(i)))return found;
            return nullptr;
        };
        cutoffSlider=findCutoff(*editor);require(cutoffSlider!=nullptr,"native cutoff slider is addressable");
        const auto point=editor->getLocalPoint(cutoffSlider,cutoffSlider->getLocalBounds().getCentre()).toFloat();
        auto* peer=editor->getPeer();const auto now=juce::Time::currentTimeMillis();
        auto hwnd=static_cast<HWND>(peer->getNativeHandle());RECT nativeBounds{};GetClientRect(hwnd,&nativeBounds);
        POINT physical{static_cast<LONG>(point.x*(nativeBounds.right-nativeBounds.left)/editor->getWidth()),static_cast<LONG>(point.y*(nativeBounds.bottom-nativeBounds.top)/editor->getHeight())};
        ClientToScreen(hwnd,&physical);SetForegroundWindow(hwnd);SetCursorPos(physical.x,physical.y);pumpMessagesFor(30);
        // Deterministic check: the dismissible control menu always offers a way out.
        const auto controlMenu=acid->buildControlMenu("cutoff");
        const auto entryFor=[&](const juce::String& label)->const ControlMenuEntry*
        {
            for(const auto& entry:controlMenu)if(entry.label==label)return &entry;
            return nullptr;
        };
        require(controlMenu.size()>=10,"control menu lists every action");
        require(entryFor("MIDI Learn")&&entryFor("Cancel MIDI Learn"),"control menu exposes learn and cancel learn");
        require(entryFor("Reset to default")&&entryFor("Remove CC mapping"),"control menu exposes reset and mapping removal");
        require(entryFor("Close menu"),"control menu has an explicit Close entry");
        const auto* modes=entryFor("Controller mode");
        require(modes!=nullptr&&modes->children.size()==4,"control menu offers all four controller modes");
        require(modes!=nullptr&&modes->children[1].label=="Pickup (soft takeover)"&&modes->children[2].label=="Relative signed","controller modes name jump, pickup and relative modes");
        int macroMenus=0,depths=0;
        for(const auto& entry:controlMenu)
            if(entry.label.startsWith("Macro ")&&!entry.children.empty())
            {
                ++macroMenus;depths+=static_cast<int>(entry.children.size());
                for(const auto& depth:entry.children)require(depth.label.endsWith("%"),"macro depths are percentages");
            }
        require(macroMenus==4&&depths==24,"four macros with positive and negative depths are offered");
        require(controlMenu.back().label=="Close menu"&&controlMenu.back().id==4,"Close menu is the final action");
        for(const auto& entry:controlMenu)
            require(!entry.header||entry.children.empty(),"menu headers carry no submenus");
        // Best-effort OS check: the native menu may not open when the test window cannot take focus.
        const auto rightClick=[&]
        {
            const auto time=juce::Time::currentTimeMillis();
            peer->handleMouseEvent(juce::MouseInputSource::InputSourceType::mouse,point,juce::ModifierKeys(),0,0,time);
            peer->handleMouseEvent(juce::MouseInputSource::InputSourceType::mouse,point,juce::ModifierKeys(juce::ModifierKeys::rightButtonModifier),0,0,time+1);
            peer->handleMouseEvent(juce::MouseInputSource::InputSourceType::mouse,point,juce::ModifierKeys(),0,0,time+2);
        };
        int maximumModal=0;
        rightClick();
        for(int attempt=0;attempt<12;++attempt)
        {
            pumpMessagesFor(40);
            maximumModal=juce::jmax(maximumModal,juce::Component::getNumCurrentlyModalComponents());
            if(maximumModal>0)break;
        }
        require(maximumModal<=1,"right-click never opens duplicate context menus");
        if(maximumModal>0)
        {
            peer->handleKeyPress(juce::KeyPress(juce::KeyPress::escapeKey));pumpMessagesFor(60);
            require(juce::Component::getNumCurrentlyModalComponents()==0,"Escape dismisses the native macro/controller menu");
        }
        else std::cout<<"Native context menu skipped: test window could not take OS focus in this environment\n";
        juce::PopupMenu::dismissAllActiveMenus();pumpMessagesFor(30);
        require(juce::Component::getNumCurrentlyModalComponents()==0,"no context menu remains open after dismissal");
        modulation.beginEdit();modulation.setParameter("cutoff",999);
        require(acid->keyPressed(juce::KeyPress('Z',juce::ModifierKeys(juce::ModifierKeys::ctrlModifier),26))&&std::abs(modulation.value("cutoff")-180)<.1f,"Ctrl+Z with control-character text undoes native edits");
        require(acid->keyPressed(juce::KeyPress('Y',juce::ModifierKeys(juce::ModifierKeys::ctrlModifier),25))&&std::abs(modulation.value("cutoff")-999)<.1f,"Ctrl+Y with control-character text redoes native edits");
        // Click the former blank region through the native peer and render its highest key.
        modulation.prepareToPlay(48000,64);
        // A visible, unobscured peer is required by JUCE's keyboard hit-testing.
        SetWindowPos(testWindow,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);pumpMessagesFor(30);
        // Probe inside pixels: halfway positions round to the next pixel at the right edge.
        for(int x=0;x<piano->getWidth();++x)
            require(piano->getNoteAndVelocityAtPosition({x+.25f,piano->getHeight()-2.f}).note>=0,"keyboard has no unplayable empty strip");
        const auto edge=editor->getLocalPoint(piano,juce::Point<int>(piano->getWidth()-2,piano->getHeight()-3)).toFloat();
        POINT edgePhysical{static_cast<LONG>(edge.x*(nativeBounds.right-nativeBounds.left)/editor->getWidth()),static_cast<LONG>(edge.y*(nativeBounds.bottom-nativeBounds.top)/editor->getHeight())};
        ClientToScreen(hwnd,&edgePhysical);SetCursorPos(edgePhysical.x,edgePhysical.y);pumpMessagesFor(30);
        const auto edgeTime=juce::Time::currentTimeMillis();
        peer->handleMouseEvent(juce::MouseInputSource::InputSourceType::mouse,edge,juce::ModifierKeys(),0,0,edgeTime);
        peer->handleMouseEvent(juce::MouseInputSource::InputSourceType::mouse,edge,juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier),0,0,edgeTime+1);
        require(modulation.keyboard.isNoteOn(1,84),"clicking the former blank right edge plays the final key");
        juce::AudioBuffer<float> edgeAudio(2,4096);juce::MidiBuffer edgeMidi;modulation.processBlock(edgeAudio,edgeMidi);
        require(edgeAudio.getMagnitude(0,0,4096)>.001f,"right-edge piano click produces audible synth output");
        peer->handleMouseEvent(juce::MouseInputSource::InputSourceType::mouse,edge,juce::ModifierKeys(),0,0,edgeTime+2);
        require(!modulation.keyboard.isNoteOn(1,84),"releasing the right-edge key releases its note");
        modulation.processBlock(edgeAudio,edgeMidi);modulation.releaseResources();
        SetWindowPos(testWindow,HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
        editor->setSize(1280,940);
        clickUI("TOOLS");require(acid->isPanelOpen(),"TOOLS opens native feature panel");
        for(int page=0;page<4;++page)
        {
            acid->showFeaturePage(page);
            acid->saveUiSnapshot(juce::File::getCurrentWorkingDirectory().getChildFile("artifacts/G-AcidBase-Tools-"+juce::String(page)+".png"));
        }
        clickUI("CLOSE");require(!acid->isPanelOpen(),"native Close dismisses feature panel");
        {   // Header lockup: the logo sits beside the wordmark, clear of every other control.
            // Pin the animation so the saved artifact is reproducible run to run.
            acid->setLogoAnimation(GAcidLogo::AnimationState{.25f,.6f});
            acid->saveUiSnapshot(juce::File::getCurrentWorkingDirectory().getChildFile("artifacts/G-AcidBase-Logo-Lockup.png"));
            juce::Image header(juce::Image::ARGB,1280,96,true);
            { juce::Graphics graphics(header); acid->paint(graphics); }   // flush before reading pixels
            const auto background=header.getPixelAt(10,45);
            const auto differs=[&](juce::Colour colour)
            {
                return std::abs((int)colour.getRed()-(int)background.getRed())
                     + std::abs((int)colour.getGreen()-(int)background.getGreen())
                     + std::abs((int)colour.getBlue()-(int)background.getBlue())>12;
            };
            int markPixels=0,spillPixels=0,glyphPixels=0;
            for(int y=21;y<69;++y)for(int x=24;x<72;++x)if(differs(header.getPixelAt(x,y)))++markPixels;
            for(int y=12;y<75;++y)for(int x=0;x<84;++x)if(x<24||x>=72)if(differs(header.getPixelAt(x,y)))++spillPixels;
            for(int y=23;y<71;++y)for(int x=84;x<132;++x)
            {
                const auto colour=header.getPixelAt(x,y);
                if(colour.getRed()>200&&colour.getGreen()>200&&colour.getBlue()>200)++glyphPixels;
            }
            require(markPixels>1800,"native logo mark is drawn beside the plugin name");
            require(spillPixels==0,"logo paints only inside its header bounds");
            require(glyphPixels>120,"wordmark 'G-' renders immediately after the logo");
        }
        acid->beginMidiLearnFor("cutoff");require(acid->isPanelOpen(),"learn opens matrix");
        require(acid->keyPressed(juce::KeyPress(juce::KeyPress::escapeKey))&&!acid->isPanelOpen()&&modulation.getMidiLearnParameter()<0,"Escape closes learn panel and cancels waiting learn");
        acid->beginMidiLearnFor("cutoff");clickUI("CLOSE");require(!acid->isPanelOpen(),"matrix Close restores FX controls");
        editor->removeFromDesktop();
        const juce::String report="G-AcidBase expanded feature verification\nFailures: 0\nMIDI learn/cancel, pickup, both relative encoder modes, macro/morph/chain/step state, generator scale and locks, undo/redo/A-B and Windows shortcuts, bounded exact-length MIDI file parsing, macro/morph/LFO audio, chance/ratchet rendering and block invariance, sequencer RUN transport (play engages SEQ, presets keep playback, MIDI releases the latch), CC1 vibrato, audition, host PPQ seeks, dense automation at 44.1/48/96 kHz, native right-click/Escape, panel Close/Escape, DPI, logo/raster agreement and header lockup, animated header logo (tempo phase, resonance response, no spill), update-check version comparison and all three result dialogs, quiet once-per-instance automatic checks (newer/current/offline, manual override, editor reopen), Change Log item and site version deep-links, About card links and dismissal, full-width piano geometry and audible native right-edge key.\nDAW drag/drop and physical high-DPI hardware require manual host testing.\n";
        juce::File::getCurrentWorkingDirectory().getChildFile("artifacts/feature-verification.txt").replaceWithText(report);
        std::cout<<report;

        return 0;
    }
    catch(const std::exception& error)
    {
        juce::File::getCurrentWorkingDirectory().getChildFile("artifacts/feature-verification.txt").replaceWithText(juce::String("G-AcidBase expanded feature verification\nFailures: 1\n")+error.what()+"\n");
        std::cerr<<"Feature check failed: "<<error.what()<<"\n";
        return 1;
    }
}
