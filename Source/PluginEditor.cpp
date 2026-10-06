#include "PluginEditor.h"
#include "Logo.h"

namespace
{
const juce::Colour background(0xff0b1018), panel(0xff141d29), border(0xff283749);
const juce::Colour green(0xffb8ff38), cyan(0xff42e8da), purple(0xffb290ff), white(0xffe8eef7), muted(0xff8798ac);
constexpr const char* modulationTargets[]{"ACCENT","FILTER (st)","REVERB","FX / GAIN"};

const ParameterSpec& spec(const char* id)
{
    for(const auto& s:parameterSpecs())if(juce::String(s.id)==id)return s;
    jassertfalse; return parameterSpecs().front();
}
juce::Font font(float size,bool bold=false) { return juce::Font(juce::FontOptions(size,bold?juce::Font::bold:juce::Font::plain)); }
void text(juce::Graphics& g,const juce::String& s,int x,int y,int w,int h,float size,juce::Colour color,bool bold=false)
{
    g.setColour(color); g.setFont(font(size,bold)); g.drawText(s,x,y,w,h,juce::Justification::centredLeft);
}
void card(juce::Graphics& g,juce::Rectangle<float> rect,const juce::String& title,const juce::String& subtitle,juce::Colour accent)
{
    g.setColour(panel); g.fillRoundedRectangle(rect,12); g.setColour(border); g.drawRoundedRectangle(rect,12,1);
    g.setColour(accent); g.fillRoundedRectangle(rect.getX()+16,rect.getY()+19,3,16,1);
    text(g,title,static_cast<int>(rect.getX()+28),static_cast<int>(rect.getY()+13),250,25,15,white,true);
    text(g,subtitle,static_cast<int>(rect.getX()+28),static_cast<int>(rect.getY()+39),350,18,11,muted);
}
}
AcidLookAndFeel::AcidLookAndFeel()
{
    setColour(juce::Slider::textBoxTextColourId,white); setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::transparentBlack);
    setColour(juce::ComboBox::backgroundColourId,background); setColour(juce::ComboBox::textColourId,white);
    setColour(juce::ComboBox::outlineColourId,border); setColour(juce::PopupMenu::backgroundColourId,panel);
    setColour(juce::PopupMenu::textColourId,white); setColour(juce::PopupMenu::highlightedBackgroundColourId,green);
    setColour(juce::PopupMenu::highlightedTextColourId,background);
    setColour(juce::TextButton::buttonColourId,border); setColour(juce::TextButton::buttonOnColourId,green);
}
void AcidLookAndFeel::drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float position,float start,float end,juce::Slider& slider)
{
    auto bounds=juce::Rectangle<float>(static_cast<float>(x),static_cast<float>(y),static_cast<float>(w),static_cast<float>(h)).reduced(9);
    const float radius=juce::jmin(bounds.getWidth(),bounds.getHeight())*.5f;
    const auto center=bounds.getCentre(); const float angle=start+position*(end-start);
    juce::Path track; track.addCentredArc(center.x,center.y,radius,radius,0,start,end,true);
    g.setColour(border); g.strokePath(track,juce::PathStrokeType(3));
    juce::Path arc; arc.addCentredArc(center.x,center.y,radius,radius,0,start,angle,true);
    const auto accentColor=slider.findColour(juce::Slider::rotarySliderFillColourId);
    g.setColour(accentColor.withAlpha(.13f)); g.strokePath(arc,juce::PathStrokeType(9));
    g.setColour(accentColor); g.strokePath(arc,juce::PathStrokeType(3));
    auto disc=juce::Rectangle<float>(radius*1.53f,radius*1.53f).withCentre(center);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff334253),center.x,disc.getY(),background,center.x,disc.getBottom(),false));
    g.fillEllipse(disc); g.setColour(juce::Colour(0xff465567)); g.drawEllipse(disc,1);
    const float dx=std::sin(angle),dy=-std::cos(angle);
    g.setColour(white); g.drawLine(center.x+dx*radius*.36f,center.y+dy*radius*.36f,center.x+dx*radius*.65f,center.y+dy*radius*.65f,2.5f);
}
void AcidLookAndFeel::drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour&,bool hover,bool down)
{
    const auto color=b.getToggleState()?green:juce::Colour(0xff223042);
    g.setColour(color.withMultipliedBrightness(down?.8f:hover?1.2f:1));
    g.fillRoundedRectangle(b.getLocalBounds().toFloat().reduced(.5f),6);
    g.setColour(b.getToggleState()?green:border); g.drawRoundedRectangle(b.getLocalBounds().toFloat().reduced(.5f),6,1);
}
void AcidLookAndFeel::drawButtonText(juce::Graphics& g,juce::TextButton& b,bool,bool)
{
    g.setColour(b.getToggleState()?background:white); g.setFont(font(11,true)); g.drawText(b.getButtonText(),b.getLocalBounds(),juce::Justification::centred);
}
void AcidLookAndFeel::drawComboBox(juce::Graphics& g,int w,int h,bool,int,int,int,int,juce::ComboBox&)
{
    g.setColour(background); g.fillRoundedRectangle(0,0,static_cast<float>(w),static_cast<float>(h),6);
    g.setColour(border); g.drawRoundedRectangle(.5f,.5f,w-1.f,h-1.f,6,1);
    juce::Path p; p.startNewSubPath(w-16.f,h*.45f); p.lineTo(w-12.f,h*.6f); p.lineTo(w-8.f,h*.45f);
    g.setColour(green); g.strokePath(p,juce::PathStrokeType(1.5f));
}
AcidKnob::AcidKnob(GAcidBaseProcessor& p,const ParameterSpec& s):title(s.name),unit(s.unit),parameterID(s.id),processor(p)
{
    addAndMakeVisible(slider); slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,86,19);
    slider.setColour(juce::Slider::rotarySliderFillColourId,green);
    slider.setTooltip(title+" - drag to adjust; double-click to reset; shift-drag for fine control.");
    slider.setDoubleClickReturnValue(true,s.initial); slider.setNumDecimalPlacesToDisplay(s.interval>=1?0:s.high>20?1:2);
    slider.setTextValueSuffix(unit.isEmpty()?"":" "+unit);
    slider.setPopupMenuEnabled(false);
    slider.setMouseDragSensitivity(150);
    slider.onDragStart=[this]{processor.beginEdit();};
    slider.setComponentID(parameterID);
    attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters,parameterID,slider);

}
void AcidKnob::resized() { slider.setBounds(0,0,getWidth(),getHeight()-21); }
void AcidKnob::paint(juce::Graphics& g) { g.setColour(muted); g.setFont(font(11,true)); g.drawText(title.toUpperCase(),0,getHeight()-21,getWidth(),20,juce::Justification::centred); }
AcidStep::AcidStep(GAcidBaseProcessor& p,int i):processor(p),index(i)
{
    addAndMakeVisible(note); note.setSliderStyle(juce::Slider::LinearBarVertical); note.setRange(-12,24,1);
    note.setTextBoxStyle(juce::Slider::TextBoxBelow,false,50,21); note.setColour(juce::Slider::trackColourId,cyan.withAlpha(.3f));
    note.setTooltip("Semitone offset from root; -12 to +24"); note.onValueChange=[this]{update();};
    for(auto* b:{&gate,&accent,&slide}) { addAndMakeVisible(b); b->setClickingTogglesState(true); b->onClick=[this]{update();}; }
    note.addMouseListener(this,true);
    for(auto* b:{&gate,&accent,&slide})b->addMouseListener(this,true);
    note.setTooltip("Semitone offset; right-click a step for probability, ratchets, timing and lock");
    gate.setTooltip("Play this step or rest"); accent.setTooltip("Accent this step"); slide.setTooltip("Tie and glide into the following step"); refresh();
}
void AcidStep::update()
{
    processor.beginEdit();auto step=processor.getStep(index);
    step.note=static_cast<int>(note.getValue());step.gate=gate.getToggleState();step.accent=accent.getToggleState();step.slide=slide.getToggleState();processor.setStep(index,step);
}
void AcidStep::mouseDown(const juce::MouseEvent& event)
{
    if(!event.mods.isPopupMenu())return;
    const int bank=processor.selectedPattern();
    const auto step=processor.getBankStep(bank,index);juce::PopupMenu menu,probability,ratchets,timing;
    for(int i=0;i<=4;++i)probability.addItem(100+i,juce::String(i*25)+"%",true,step.probability==i*25);
    for(int i=1;i<=4;++i)ratchets.addItem(200+i,juce::String(i),true,step.ratchets==i);
    for(int i=-25;i<=25;i+=5)timing.addItem(325+i,juce::String(i)+"% of step",true,step.timing==i);
    menu.addSubMenu("Probability",probability);menu.addSubMenu("Ratchets",ratchets);menu.addSubMenu("Microtiming",timing);
    menu.addItem(1,step.locked?"Unlock step":"Lock step against generation/reset");menu.addItem(2,"Cancel");
    juce::Component::SafePointer<AcidStep> safe(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),[safe,bank](int result)
    {
        if(!safe||result==0||result==2)return;
        safe->processor.beginEdit();auto s=safe->processor.getBankStep(bank,safe->index);
        if(result==1)s.locked=!s.locked;
        else if(result>=100&&result<=104)s.probability=(result-100)*25;
        else if(result>=201&&result<=204)s.ratchets=result-200;
        else if(result>=300&&result<=350)s.timing=result-325;
        safe->processor.setBankStep(bank,safe->index,s);safe->refresh();
    });
}
void AcidStep::refresh()
{
    const auto s=processor.getStep(index); note.setValue(s.note,juce::dontSendNotification);
    gate.setToggleState(s.gate,juce::dontSendNotification); accent.setToggleState(s.accent,juce::dontSendNotification); slide.setToggleState(s.slide,juce::dontSendNotification); repaint();
}
void AcidStep::resized()
{
    note.setBounds(8,22,getWidth()-16,44); gate.setBounds(6,69,getWidth()-12,18); accent.setBounds(6,90,getWidth()-12,18); slide.setBounds(6,111,getWidth()-12,18);
}
void AcidStep::paint(juce::Graphics& g)
{
    const bool active=processor.activeStep.load()==index&&processor.activePattern.load()==processor.selectedPattern();
    g.setColour(active?green.withAlpha(.1f):background); g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(1),6);
    g.setColour(active?green:border); g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1),6,1);
    g.setColour(active?green:muted); g.setFont(font(10,true));
    const auto step=processor.getStep(index);
    g.drawText(juce::String(index+1).paddedLeft('0',2)+(step.locked?" L":"")+(step.ratchets>1?" x"+juce::String(step.ratchets):""),0,3,getWidth(),17,juce::Justification::centred);
}
AcidKnob* GAcidBaseEditor::addKnob(const char* id,juce::Rectangle<int> bounds)
{
    auto k=std::make_unique<AcidKnob>(processor,spec(id)); auto* ptr=k.get(); surface.addAndMakeVisible(*k); k->setBounds(bounds); knobs.push_back(std::move(k)); return ptr;
}
juce::ComboBox* GAcidBaseEditor::addCombo(const char* id,juce::Rectangle<int> bounds)
{
    auto box=std::make_unique<juce::ComboBox>(); const auto& s=spec(id); const auto choices=juce::StringArray::fromTokens(s.choices,"|","");
    box->addItemList(choices,1); box->setTooltip(s.name); box->setComponentID(id); auto* ptr=box.get(); surface.addAndMakeVisible(*box);  box->setBounds(bounds);
    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.parameters,id,*box)); combos.push_back(std::move(box)); return ptr;
}
juce::TextButton* GAcidBaseEditor::addToggle(const char* id,const char* title,juce::Rectangle<int> bounds)
{
    auto b=std::make_unique<juce::TextButton>(title); b->setClickingTogglesState(true); b->setTooltip(spec(id).name); b->setComponentID(id); auto* ptr=b.get();
    surface.addAndMakeVisible(*b); b->setBounds(bounds); buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.parameters,id,*b)); toggles.push_back(std::move(b)); return ptr;
}
GAcidBaseEditor::GAcidBaseEditor(GAcidBaseProcessor& p):AudioProcessorEditor(p),processor(p),piano(p.keyboard,juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel(&look); addAndMakeVisible(surface); surface.setSize(1280,940);
    surface.addMouseListener(this,true);
    auto add=[this](juce::Component& c,juce::Rectangle<int> b){surface.addAndMakeVisible(c);c.setBounds(b);};
    add(preset,{480,30,380,42});
    for(int i=0;i<50;++i)preset.addItem(p.getProgramName(i),i+1);
    preset.onChange=[this]{if(preset.getSelectedId()>0) {processor.setCurrentProgram(preset.getSelectedId()-1); for(auto& s:steps)if(s)s->refresh();}};
    add(previous,{438,30,34,42}); add(next,{868,30,34,42});
    previous.onClick=[this]{preset.setSelectedId((processor.getCurrentProgram()+49)%50+1);}; next.onClick=[this]{preset.setSelectedId((processor.getCurrentProgram()+1)%50+1);};
    add(save,{918,30,66,42}); add(load,{992,30,66,42}); save.onClick=[this]{chooseFile(true);}; load.onClick=[this]{chooseFile(false);};
    add(audition,{1100,30,156,42}); audition.setClickingTogglesState(true); audition.onClick=[this]{processor.audition(audition.getToggleState());};
    audition.setTooltip("Hold a C2 MIDI note to audition the current mode. Turn down your monitors first.");
    addCombo("wave",{925,135,124,28});
    add(advanced,{1066,135,166,28}); advanced.setClickingTogglesState(true);
    advanced.onClick=[this]{circuit=advanced.getToggleState(); for(auto* c:synthControls)c->setVisible(!circuit); for(auto* c:circuitControls)c->setVisible(circuit); repaint();};
    const char* mainIds[]{"tune","cutoff","resonance","envMod","decay","accent","slideTime","sweep","attack","accDecay","accVolume","vibSpeed","vibDepth"};
    for(int i=0;i<13;++i)synthControls.push_back(addKnob(mainIds[i],{44+i*92,175,86,126}));
    const char* circuitIds[]{"trim","envCurve","accEnv","accAmp","filterBias","keyTrack","pulseWidth","clicks","noise"};
    for(int i=0;i<9;++i) { auto* c=addKnob(circuitIds[i],{110+i*120,175,108,126}); circuitControls.push_back(c); c->setVisible(false); }
    addCombo("mode",{42,411,138,34});
    addKnob("slideTime",{41,450,72,96}); addKnob("gate",{119,450,72,96});
    addToggle("distortion","ACTIVE",{430,358,72,25}); addCombo("model",{237,412,161,29}); addCombo("post",{407,412,95,29});
    addKnob("dynamics",{233,454,86,103}); addKnob("drive",{325,454,86,103}); addKnob("color",{417,454,86,103});
    add(reverbTab,{542,360,94,28}); add(delayTab,{641,360,94,28}); add(eqTab,{740,360,78,28}); add(chorusTab,{823,360,94,28});
    reverbTab.onClick=[this]{showFx(0);}; delayTab.onClick=[this]{showFx(1);}; eqTab.onClick=[this]{showFx(2);}; chorusTab.onClick=[this]{showFx(3);};
    const char* enables[]{"reverb","delay","eq","chorus"};
    const char* rv[]{"predelay","earlyLate","feedback","reverbMix"}; const char* dl[]{"delayFeedback","delayMix"};
    const char* eq[]{"eqLow","eqMid","eqHigh"}; const char* ch[]{"chorusRate","chorusDepth","chorusMix"};
    for(int page=0;page<4;++page)fxControls[static_cast<size_t>(page)].push_back(addToggle(enables[page],"ENABLE",{937,361,95,27}));
    for(int i=0;i<4;++i)fxControls[0].push_back(addKnob(rv[i],{559+i*120,432,101,119}));
    fxControls[1].push_back(addCombo("delayTime",{548,462,178,35}));
    for(int i=0;i<2;++i)fxControls[1].push_back(addKnob(dl[i],{755+i*130,432,112,119}));
    for(int i=0;i<3;++i)fxControls[2].push_back(addKnob(eq[i],{570+i*155,432,115,119}));
    for(int i=0;i<3;++i)fxControls[3].push_back(addKnob(ch[i],{570+i*155,432,115,119}));
    showFx(0);
    addToggle("bypass","BYPASS FX",{1084,398,148,24}); addToggle("limiter","LIMITER",{1084,431,148,24});
    addKnob("volume",{1082,463,104,93});
    addToggle("run","RUN",{46,642,69,28}); addCombo("rate",{124,642,91,28});
    addKnob("tempo",{228,625,75,57}); addKnob("seqRoot",{308,625,75,57}); addKnob("swing",{389,625,75,57});
    addCombo("arpDirection",{660,642,117,28}); addKnob("arpOctaves",{786,625,93,57});
    add(randomize,{1062,642,84,28}); add(resetPattern,{1154,642,78,28});
    randomize.onClick=[this]{processor.generatePattern(generatorScale?generatorScale->getSelectedId()-1:0,generatorDensity?static_cast<float>(generatorDensity->getValue()):.85f,generatorVariation?static_cast<float>(generatorVariation->getValue()):1.f);};
    resetPattern.onClick=[this]{processor.resetPattern();};
    for(int i=0;i<16;++i){steps[static_cast<size_t>(i)]=std::make_unique<AcidStep>(p,i);add(*steps[static_cast<size_t>(i)],{43+i*75,686,71,134});}
    add(piano,{24,850,1232,60}); piano.setAvailableRange(24,84); piano.setScrollButtonsVisible(false);
    int whiteKeys=0;
    for(int note=piano.getRangeStart();note<=piano.getRangeEnd();++note)
        if(!juce::MidiMessage::isMidiNoteBlack(note))++whiteKeys;
    piano.setKeyWidth(static_cast<float>(piano.getWidth())/static_cast<float>(whiteKeys));
    add(modulationToggle,{916,606,135,27});
    modulationToggle.onClick=[this]{matrixVisible=modulationToggle.getToggleState();if(matrixVisible)showFeaturePage(-1);showModulationMatrix();};
    modulationToggle.setClickingTogglesState(true);modulationToggle.setTooltip("Open the MIDI velocity / aftertouch / mod wheel modulation matrix");
    modulationPanel.setOpaque(false);surface.addAndMakeVisible(modulationPanel);
    auto matrixHeading=std::make_unique<juce::Label>();matrixHeading->setText("MIDI MOD MATRIX",juce::dontSendNotification);matrixHeading->setColour(juce::Label::textColourId,cyan);matrixHeading->setBounds(12,8,230,24);modulationPanel.addAndMakeVisible(*matrixHeading);matrixLabels.push_back(std::move(matrixHeading));
    modulationPanel.addAndMakeVisible(closeMatrix);closeMatrix.setBounds(434,8,78,24);closeMatrix.setComponentID("closeMatrix");closeMatrix.onClick=[this]{closePanels();};
    modulationPanel.addAndMakeVisible(cancelLearn);cancelLearn.setBounds(290,8,137,24);cancelLearn.onClick=[this]{processor.cancelMidiLearn();learnMessage="MIDI learn cancelled";};
    learnStatus.setFont(font(10));learnStatus.setColour(juce::Label::textColourId,cyan);
    modulationPanel.addAndMakeVisible(learnStatus);
    for(int col=0;col<4;++col)
    {
        auto label=std::make_unique<juce::Label>();label->setText(modulationTargets[col],juce::dontSendNotification);label->setFont(font(9,true));label->setColour(juce::Label::textColourId,muted);
        label->setJustificationType(juce::Justification::centred);modulationPanel.addAndMakeVisible(*label);label->setBounds(78+col*110,49,108,17);matrixLabels.push_back(std::move(label));
    }
    const char* sources[]{"VELOCITY","AFTERTOUCH","MOD WHEEL"};
    const char* routes[3][4]{{"modVelAccent","modVelFilter","modVelReverb","modVelEffects"},{"modPressureAccent","modPressureFilter","modPressureReverb","modPressureEffects"},{"modWheelAccent","modWheelFilter","modWheelReverb","modWheelEffects"}};
    for(int row=0;row<3;++row)
    {
        auto source=std::make_unique<juce::Label>();source->setText(sources[row],juce::dontSendNotification);source->setFont(font(8,true));source->setColour(juce::Label::textColourId,row==0?green:row==1?purple:cyan);source->setJustificationType(juce::Justification::centredLeft);
        modulationPanel.addAndMakeVisible(*source);source->setBounds(8,70+row*43,68,20);matrixLabels.push_back(std::move(source));
        for(int col=0;col<4;++col)
        {
            auto knob=std::make_unique<AcidSlider>();knob->setSliderStyle(juce::Slider::LinearHorizontal);knob->setTextBoxStyle(juce::Slider::TextBoxRight,false,39,18);
            const bool semitones=col==1;const bool bipolar=col==0||col==3;knob->setRange(semitones?-48.f:bipolar?-1.f:col==2?-.7f:0.f,semitones?48.f:bipolar?1.f:.7f,.01);
            knob->setColour(juce::Slider::trackColourId,green.withAlpha(.45f));knob->setTooltip(juce::String(sources[row])+" to "+juce::String(modulationTargets[col]));knob->setComponentID(routes[row][col]);
            modulationPanel.addAndMakeVisible(*knob);knob->setBounds(78+col*110,67+row*43,106,27);
            knob->onDragStart=[this]{processor.beginEdit();};matrixAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters,routes[row][col],*knob));
            matrixDepthSliders.push_back(std::move(knob));
        }
    }
    modulationPanel.addAndMakeVisible(clearMidi);clearMidi.setButtonText("CLEAR LEARN");clearMidi.setClickingTogglesState(false);clearMidi.onClick=[this]{processor.beginEdit();processor.clearMidiLearnForParameter(-1);learnMessage="MIDI mappings cleared";learnStatus.setText(learnMessage,juce::dontSendNotification);};
    clearMidi.setBounds(412,198,101,24);learnStatus.setBounds(12,200,390,20);learnStatus.setText("Right-click a continuous control to MIDI learn",juce::dontSendNotification);matrixVisible=false;showModulationMatrix();
    piano.setColour(juce::MidiKeyboardComponent::whiteNoteColourId,juce::Colour(0xffbbc5ce)); piano.setColour(juce::MidiKeyboardComponent::blackNoteColourId,background);
    piano.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId,green.withAlpha(.65f));
    add(status,{24,915,1232,20}); status.setColour(juce::Label::textColourId,muted); status.setFont(font(11));
    status.setText("4x oversampled  /  velocity accent >= 102  /  legato MIDI slide  /  pitch bend +/-2 st  /  mod wheel vibrato",juce::dontSendNotification);
    preset.setSelectedId(processor.getCurrentProgram()+1,juce::dontSendNotification);
    displayedPreset=processor.getCurrentProgram();
    updateOverlay=std::make_unique<gacid::UpdateResultOverlay>();
    aboutCard=std::make_unique<gacid::AboutCard>();
    createFeatureControls();setWantsKeyboardFocus(true);
    setResizable(true,true); setResizeLimits(960,705,5120,3760); getConstrainer()->setFixedAspectRatio(1280.0/940.0); setSize(1280,940);
    logoClockMs=juce::Time::getMillisecondCounter();
    processor.sessionUpdates.startOnce();
    startTimerHz(25);
}
GAcidBaseEditor::~GAcidBaseEditor()
{
    // Join the update fetch before anything it touches goes away.
    if(updateThread!=nullptr)updateThread->stopThread(10000);
    stopTimer();juce::PopupMenu::dismissAllActiveMenus(); if(audition.getToggleState())processor.audition(false); setLookAndFeel(nullptr);
}
void GAcidBaseEditor::showFx(int page)
{
    fxPage=page; for(int i=0;i<4;++i)for(auto* c:fxControls[static_cast<size_t>(i)])c->setVisible(i==page);
    reverbTab.setToggleState(page==0,juce::dontSendNotification); delayTab.setToggleState(page==1,juce::dontSendNotification);
    eqTab.setToggleState(page==2,juce::dontSendNotification); chorusTab.setToggleState(page==3,juce::dontSendNotification);
    if(matrixVisible)showModulationMatrix();repaint();
}
void GAcidBaseEditor::setScaleFactor(float newScale)
{
    juce::AudioProcessorEditor::setScaleFactor(juce::jlimit(.75f,4.f,newScale));
}
void GAcidBaseEditor::resized()
{
    const auto scale=juce::jlimit(.5f,4.f,static_cast<float>(getWidth())/1280.f);
    surface.setBounds(0,0,1280,940);
    surface.setTransform(juce::AffineTransform::scale(scale));
}
void GAcidBaseEditor::beginMidiLearnFor(const juce::String& id)
{
    if(auto* parameter=processor.parameters.getParameter(id))
    {
        const int parameterIndex=parameter->getParameterIndex();
        if(processor.beginMidiLearn(parameterIndex))
        {
            learnMessage=juce::String("Move a MIDI CC to link it to ")+parameter->getName(40);
            learnStatus.setText(learnMessage,juce::dontSendNotification);showFeaturePage(-1);matrixVisible=true;showModulationMatrix();
        }
    }
}
juce::String GAcidBaseEditor::getLearnStatus() const{return learnStatus.getText();}
void GAcidBaseEditor::showModulationMatrix()
{
    modulationPanel.setBounds(528,342,526,226);modulationPanel.setVisible(matrixVisible);
    for(auto* c:fxControls[static_cast<size_t>(fxPage)])c->setVisible(!matrixVisible);
    reverbTab.setVisible(!matrixVisible);delayTab.setVisible(!matrixVisible);eqTab.setVisible(!matrixVisible);chorusTab.setVisible(!matrixVisible);
    modulationToggle.setToggleState(matrixVisible,juce::dontSendNotification);
    if(matrixVisible)modulationPanel.toFront(false);
    repaint();
}
void GAcidBaseEditor::mouseDown(const juce::MouseEvent& event)
{
    if(event.eventComponent==dragMidi)midiDragStarted=false;
    auto* component=event.eventComponent;
    while(component!=nullptr&&component!=&surface)
    {
        const auto id=component->getComponentID();
        if(auto* parameter=processor.parameters.getParameter(id))
        {
            if(event.mods.isPopupMenu()&&!parameter->isDiscrete())showControlMenu(id,component);
            else if(!event.mods.isPopupMenu())processor.beginEdit();
            return;
        }
        component=component->getParentComponent();
    }
}
void GAcidBaseEditor::paint(juce::Graphics& g)
{
    g.fillAll(background);
    const auto scale=juce::jlimit(.5f,4.f,static_cast<float>(getWidth())/1280.f);
    g.addTransform(juce::AffineTransform::scale(scale));
    g.setGradientFill(juce::ColourGradient(green.withAlpha(.065f),0,0,purple.withAlpha(.035f),1280,500,false)); g.fillRect(0,0,1280,940);
    GAcidLogo::paint(g,logoBounds,logo);
    text(g,"G-",84,23,58,48,39,white,true); text(g,"AcidBase",136,23,300,48,39,green,true);
    text(g,"GOA ACID SYNTHESIZER  /  303 REIMAGINED",27,78,400,18,11,muted,true);
    
    card(g,{24,116,1232,208},circuit?"CIRCUIT":"SYNTHESIS",circuit?"Inspired circuit trims - musical controls, not component values":"Bandlimited oscillator / nonlinear resonant ladder / accent + slide",green);
    card(g,{24,342,182,232},"PLAY MODE","MIDI / sequencer / arp",cyan);
    card(g,{220,342,294,232},"DISTORTION","Four nonlinear colors",purple);
    card(g,{528,342,526,232},"","",cyan);
    if(matrixVisible)text(g,"MIDI MODULATION",548,355,250,24,14,cyan,true);
    else text(g,fxPage==0?"Atmospheric space / predelay + diffusion":fxPage==1?"Tempo-synced ping-pong / filtered feedback":fxPage==2?"100 Hz / 900 Hz / 6 kHz tone shaping":"Stereo movement / fractional-delay modulation",548,397,480,18,11,muted);
    card(g,{1068,342,188,232},"OUTPUT","Stereo / gain / safety",green);
    card(g,{24,594,1232,234},"ACID SEQUENCER","",cyan);
    text(g,"16 STEPS / GATE / ACCENT / SLIDE",290,610,350,18,10,muted,true);
    text(g,"ARP",660,615,100,20,10,muted,true); text(g,"Host tempo: "+juce::String(processor.hostTempo.load(),1)+" BPM",895,644,164,20,11,cyan);
    for(int c=0;c<2;++c)
    {
        const float level=c==0?meterL:meterR;
        for(int n=0;n<15;++n)
        {
            const float threshold=juce::Decibels::decibelsToGain(-42.f+n*3.f);
            g.setColour(level>threshold?(n>12?juce::Colour(0xffff6c80):n>10?juce::Colour(0xffffca66):green):border);
            g.fillRoundedRectangle(1203.f+c*13,550.f-n*6,8,4,1);
        }
    }
    text(g,"L  R",1200,553,36,15,9,muted);
    if(featurePage>=0)
    {
        g.setColour(panel);g.fillRoundedRectangle(24,116,1232,208,12);
        g.setColour(border);g.drawRoundedRectangle(24,116,1232,208,12,1);
    }
}
void GAcidBaseEditor::timerCallback()
{
    // Quiet startup check: keep a pending update until other cards/manual
    // checks have finished, and consume it only once across editor reopenings.
    if(!manualUpdatePending&&!updateOverlay->isVisible()&&!aboutCard->isVisible())
    {
        gacid::UpdateOutcome automatic;
        if(processor.sessionUpdates.takeNotification(automatic))
            showUpdateResult(gacid::describe(automatic,gacid::versionString()));
    }
    // The header mark runs one cycle per host tempo beat, and opens up with filter resonance.
    const auto now=juce::Time::getMillisecondCounter();
    const double seconds=juce::jlimit(0.0,0.25,(now-logoClockMs)/1000.0);logoClockMs=now;
    logo=GAcidLogo::advance(logo,processor.hostTempo.load(),seconds);
    logo.resonance=GAcidLogo::resonanceAmount(processor.value("resonance"));
    const int program=processor.getCurrentProgram();
    if(program!=displayedPreset) { preset.setSelectedId(program+1,juce::dontSendNotification); displayedPreset=program; }
    for(auto& s:steps)s->refresh();
    undo.setEnabled(processor.canUndo());redo.setEnabled(processor.canRedo());compare.setButtonText(processor.comparisonIsB()?"B / A":"A / B");
    if(bankSelector)bankSelector->setSelectedId(processor.selectedPattern()+1,juce::dontSendNotification);
    for(int i=0;i<8;++i)if(chainBanks[static_cast<size_t>(i)]&&chainRepeats[static_cast<size_t>(i)])
    {
        const auto entry=processor.getChainEntry(i);chainBanks[static_cast<size_t>(i)]->setSelectedId(entry.first+1,juce::dontSendNotification);
        chainRepeats[static_cast<size_t>(i)]->setValue(entry.second,juce::dontSendNotification);
    }
    if(processor.getMidiLearnParameter()>=0)learnMessage="MIDI LEARN: move a controller";
    else if(processor.getLastLearnedController()>=0)learnMessage="CC "+juce::String(processor.getLastLearnedController())+" -> "+processor.getLastLearnedParameter()+" / "+juce::String(processor.getLearnedMidiControllerCount())+" mapped";
    else if(processor.getLearnedMidiControllerCount()>0)learnMessage=juce::String(processor.getLearnedMidiControllerCount())+" MIDI CC mappings saved";
    else if(learnMessage.isEmpty())learnMessage="Right-click any continuous control to learn a MIDI CC";
    learnStatus.setText(learnMessage,juce::dontSendNotification);
    meterL=juce::jmax(processor.leftMeter.load(),meterL*.86f); meterR=juce::jmax(processor.rightMeter.load(),meterR*.86f); repaint();
}
void GAcidBaseEditor::chooseFile(bool saving)
{
    chooser=std::make_unique<juce::FileChooser>(saving?"Save G-AcidBase preset":"Load G-AcidBase preset",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("G-AcidBase.gacid"),"*.gacid");
    const int browserFlags=saving?juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting:juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles;
    juce::Component::SafePointer<GAcidBaseEditor> safe(this);
    chooser->launchAsync(browserFlags,[safe,saving](const juce::FileChooser& fc){if(!safe)return;auto f=fc.getResult();if(f==juce::File())return;if(saving)f=f.withFileExtension("gacid");if(!saving)safe->processor.beginEdit();const bool ok=saving?safe->processor.savePreset(f):safe->processor.loadPreset(f);safe->status.setText(ok?(saving?"Preset saved":"Preset loaded"):"Preset operation failed: invalid file or write permission",juce::dontSendNotification);});
}
void GAcidBaseEditor::saveUiSnapshot(const juce::File& file)
{
    auto image=createComponentSnapshot(getLocalBounds(),true,juce::jmax(1.f,getDesktopScaleFactor()));
    if(auto stream=file.createOutputStream())
    {
        stream->setPosition(0);
        if(stream->truncate().wasOk())juce::PNGImageFormat().writeImageToStream(image,*stream);
    }
}

//==============================================================================
//  Update check (TOOLS -> CHECK FOR UPDATES): reads the version feed the site
//  publishes on a background thread and always reports back in the message
//  thread - success, a newer release, or the reason nothing could be checked.
//  This one anonymous GET of a static file is the only request the plugin ever
//  makes.
//==============================================================================
void GAcidBaseEditor::runUpdateCheck()
{
    if(manualUpdatePending||(updateThread!=nullptr&&updateThread->isThreadRunning()))return;
    manualUpdatePending=true;

    // The callback only ever touches the editor through a SafePointer, so a
    // check that outlives the window is harmless.
    juce::Component::SafePointer<GAcidBaseEditor> safe(this);
    updateThread=std::make_unique<gacid::UpdateCheckThread>(processor.sessionUpdates.feedUrl(),[safe](const gacid::UpdateOutcome& outcome)
    {
        // Publish the whole result on the message thread in one go, so
        // updateCheckDone() can never read half-written state.
        juce::MessageManager::callAsync([safe,outcome]
        {
            if(safe==nullptr)return;
            safe->updateResult=outcome;
            safe->updateCheckDone();
        });
    });
    if(!updateThread->startThread())
    {
        updateResult={};
        updateCheckDone();
    }
}

void GAcidBaseEditor::updateCheckDone()
{
    // No thread teardown here: the worker may still be finishing its last
    // lines; the next check (or the destructor) joins it safely.
    //
    // Manual checks always answer. Automatic checks are filtered separately.
    manualUpdatePending=false;
    if(updateResult.reachable&&updateResult.newer)processor.sessionUpdates.acknowledgeNotification();
    // Every manual outcome opens this editor's own overlay - never a native box,
    // never silence. A native box cannot host a link at all, which is the
    // whole reason the download row is clickable here, and an unparented
    // native box was free to land behind the DAW window, which reads as
    // "nothing happens".
    showUpdateResult(gacid::describe(updateResult,gacid::versionString()));
}

void GAcidBaseEditor::showUpdateResult(const gacid::UpdateMessage& message)
{
    if(updateOverlay==nullptr)return;
    updateOverlay->configure(message.title,message.message,message.downloadUrl,message.notesUrl,message.downloadLabel);
    updateOverlay->setBounds(0,0,1280,940);
    updateOverlay->toFront(true);
    updateOverlay->setVisible(true);
}
