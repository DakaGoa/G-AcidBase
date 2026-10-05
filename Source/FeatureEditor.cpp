#include "PluginEditor.h"

namespace
{
const juce::Colour cyan(0xff42e8da),muted(0xff8798ac);
const ParameterSpec& findSpec(const char* id)
{
    for(const auto& s:parameterSpecs())if(juce::String(s.id)==id)return s;
    jassertfalse;return parameterSpecs().front();
}
}
void GAcidBaseEditor::createFeatureControls()
{
    const auto top=[this](juce::Component& c,juce::Rectangle<int> bounds){surface.addAndMakeVisible(c);c.setBounds(bounds);};
    top(features,{916,77,142,25});features.onClick=[this]{if(featurePage>=0)closePanels();else showFeaturePage(0);};
    top(undo,{480,78,70,24});top(redo,{556,78,70,24});top(compare,{632,78,76,24});top(captureAB,{714,78,146,24});
    undo.onClick=[this]{processor.undoEdit();};redo.onClick=[this]{processor.redoEdit();};
    compare.onClick=[this]{processor.switchComparison();};captureAB.onClick=[this]{processor.captureComparison();status.setText("A/B initialized from current state; edit each side independently",juce::dontSendNotification);};
    surface.addChildComponent(featurePanel);featurePanel.setBounds(24,116,1232,208);
    const char* titles[]{"LFOs","MACROS / MORPH","PATTERNS","CONTROLLERS"};
    for(int i=0;i<4;++i)
    {
        auto& tab=featureTabs[static_cast<size_t>(i)];featurePanel.addAndMakeVisible(tab);tab.setButtonText(titles[i]);tab.setBounds(12+i*160,10,154,25);tab.onClick=[this,i]{showFeaturePage(i);};
    }
    featurePanel.addAndMakeVisible(featureClose);featureClose.setBounds(1140,10,78,25);featureClose.setComponentID("featureClose");featureClose.onClick=[this]{closePanels();};
    featurePanel.addAndMakeVisible(featureTitle);featureTitle.setBounds(660,10,470,25);featureTitle.setColour(juce::Label::textColourId,cyan);
    const auto own=[this](std::unique_ptr<juce::Component> component,int page,juce::Rectangle<int> bounds)
    {
        auto* ptr=component.get();featurePanel.addAndMakeVisible(*ptr);ptr->setBounds(bounds);featureControls[static_cast<size_t>(page)].push_back(ptr);ownedFeatureControls.push_back(std::move(component));return ptr;
    };
    const auto label=[&](juce::String title,int page,int x,int y,int width)
    {
        auto l=std::make_unique<juce::Label>();l->setText(title,juce::dontSendNotification);l->setColour(juce::Label::textColourId,muted);own(std::move(l),page,{x,y,width,18});
    };
    const auto slider=[&](const char* id,int page,int x,int y,int width)->juce::Slider*
    {
        label(findSpec(id).name,page,x,y,width);
        auto s=std::make_unique<AcidSlider>();s->setSliderStyle(juce::Slider::LinearHorizontal);s->setTextBoxStyle(juce::Slider::TextBoxRight,false,49,20);s->setComponentID(id);
        s->onDragStart=[this]{processor.beginEdit();};auto* ptr=static_cast<juce::Slider*>(own(std::move(s),page,{x,y+19,width,25}));
        featureSliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters,id,*ptr));return ptr;
    };
    const auto combo=[&](const char* id,int page,int x,int y,int width)->juce::ComboBox*
    {
        label(findSpec(id).name,page,x,y,width);
        auto box=std::make_unique<juce::ComboBox>();box->addItemList(juce::StringArray::fromTokens(findSpec(id).choices,"|",""),1);box->setComponentID(id);
        auto* ptr=static_cast<juce::ComboBox*>(own(std::move(box),page,{x,y+19,width,25}));
        featureComboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.parameters,id,*ptr));return ptr;
    };
    const auto button=[&](juce::String title,int page,juce::Rectangle<int> bounds,std::function<void()> action)->juce::TextButton*
    {
        auto b=std::make_unique<juce::TextButton>(title);b->onClick=std::move(action);return static_cast<juce::TextButton*>(own(std::move(b),page,bounds));
    };
    for(int lfo=0;lfo<2;++lfo)
    {
        const int x=12+lfo*608;
        const char* ids1[]{"lfo1Sync","lfo1Division","lfo1Shape","lfo1Retrigger","lfo1Rate","lfo1Filter","lfo1Accent","lfo1Reverb","lfo1Effects"};
        const char* ids2[]{"lfo2Sync","lfo2Division","lfo2Shape","lfo2Retrigger","lfo2Rate","lfo2Filter","lfo2Accent","lfo2Reverb","lfo2Effects"};
        const auto* ids=lfo==0?ids1:ids2;
        for(int i=0;i<4;++i)combo(ids[i],0,x+i*148,46,138);
        slider(ids[4],0,x,101,180);slider(ids[5],0,x+197,101,185);slider(ids[6],0,x+395,101,185);
        slider(ids[7],0,x,154,280);slider(ids[8],0,x+302,154,280);
    }
    for(int macro=0;macro<4;++macro)
    {
        const char* ids[]{"macro1","macro2","macro3","macro4"};slider(ids[macro],1,12+macro*300,50,282);
    }
    button("CAPTURE MORPH A",1,{12,115,182,28},[this]{processor.captureMorph(0);});
    button("CAPTURE MORPH B",1,{205,115,182,28},[this]{processor.captureMorph(1);});
    slider("morph",1,407,105,410);
    button("CLEAR MORPH",1,{835,115,170,28},[this]{processor.beginEdit();processor.clearMorph();});
    label("Right-click sound knobs: assign Macro 1-4 with positive/negative depth. Morph uses captured sounds; switches change at halfway.",1,12,174,1200);

    auto banks=std::make_unique<juce::ComboBox>();for(int i=0;i<8;++i)banks->addItem("Pattern "+juce::String(i+1),i+1);
    bankSelector=static_cast<juce::ComboBox*>(own(std::move(banks),2,{12,62,155,27}));bankSelector->setSelectedId(1,juce::dontSendNotification);
    bankSelector->onChange=[this]{processor.beginEdit();processor.selectPattern(bankSelector->getSelectedId()-1);};
    label("EDIT BANK",2,12,43,155);
    combo("chainEnabled",2,180,43,125);slider("chainLength",2,318,43,160);slider("seqSeed",2,490,43,160);
    auto scale=std::make_unique<juce::ComboBox>();scale->addItemList({"Phrygian","Minor","Major","Dorian pent."},1);
    generatorScale=static_cast<juce::ComboBox*>(own(std::move(scale),2,{672,62,150,27}));generatorScale->setSelectedId(1,juce::dontSendNotification);label("GENERATOR SCALE",2,672,43,150);
    const auto generatorSlider=[&](const char* title,int x,double initial)
    {
        label(title,2,x,43,165);auto s=std::make_unique<juce::Slider>();s->setSliderStyle(juce::Slider::LinearHorizontal);s->setTextBoxStyle(juce::Slider::TextBoxRight,false,40,20);s->setRange(0,1,.01);s->setValue(initial);
        return static_cast<juce::Slider*>(own(std::move(s),2,{x,62,165,27}));
    };
    generatorDensity=generatorSlider("NOTE DENSITY",835,.85);generatorVariation=generatorSlider("VARIATION",1010,1);
    for(int slot=0;slot<8;++slot)
    {
        const int x=12+slot*150;label("CHAIN "+juce::String(slot+1)+" / repeats",2,x,99,140);
        auto bank=std::make_unique<juce::ComboBox>();for(int i=0;i<8;++i)bank->addItem("P"+juce::String(i+1),i+1);
        auto* b=static_cast<juce::ComboBox*>(own(std::move(bank),2,{x,119,67,25}));chainBanks[static_cast<size_t>(slot)]=b;b->setSelectedId(slot+1,juce::dontSendNotification);
        auto r=std::make_unique<juce::Slider>();r->setSliderStyle(juce::Slider::IncDecButtons);r->setTextBoxStyle(juce::Slider::TextBoxLeft,false,27,25);r->setRange(1,16,1);r->setValue(1);
        auto* repeat=static_cast<juce::Slider*>(own(std::move(r),2,{x+72,119,68,25}));chainRepeats[static_cast<size_t>(slot)]=repeat;
        const auto update=[this,slot,b,repeat]{processor.beginEdit();processor.setChainEntry(slot,b->getSelectedId()-1,static_cast<int>(repeat->getValue()));};b->onChange=update;repeat->onValueChange=update;
    }
    button("GENERATE",2,{12,165,140,27},[this]{randomize.onClick();});
    button("EXPORT MIDI",2,{166,165,150,27},[this]{chooseMidiExport();});
    dragMidi=button("DRAG MIDI TO DAW",2,{330,165,198,27},[]{});dragMidi->setComponentID("dragMidi");
    label("Right-click steps for chance / ratchets / timing / lock. Export uses chain when enabled.",2,550,169,655);
    combo("controllerMode",3,12,55,248);combo("midiChannel",3,280,55,180);
    button("CANCEL MIDI LEARN",3,{480,74,205,27},[this]{processor.cancelMidiLearn();});
    button("CLEAR ALL CC MAPS",3,{700,74,205,27},[this]{processor.beginEdit();processor.clearMidiLearnForParameter(-1);});
    label("Jump: absolute. Pickup: cross the saved value. Relative signed: 1 / 127. Relative offset: 65 / 63.",3,12,125,1200);
    label("Right-click any continuous control to change its existing CC mode or remove its mapping. Escape / CLOSE dismiss panels.",3,12,162,1200);
    showFeaturePage(-1);
}
void GAcidBaseEditor::showFeaturePage(int page)
{
    featurePage=page;featurePanel.setVisible(page>=0);
    for(auto* c:synthControls)c->setVisible(page<0&&!circuit);
    for(auto* c:circuitControls)c->setVisible(page<0&&circuit);
    advanced.setVisible(page<0);
    for(auto& box:combos)if(box->getComponentID()=="wave")box->setVisible(page<0);
    for(int p=0;p<4;++p)
    {
        featureTabs[static_cast<size_t>(p)].setToggleState(p==page,juce::dontSendNotification);
        for(auto* c:featureControls[static_cast<size_t>(p)])c->setVisible(p==page);
    }
    if(page>=0)
    {
        matrixVisible=false;showModulationMatrix();featurePanel.toFront(false);
        featureTitle.setText(page==0?"Two bipolar modulation sources":page==1?"Sound design / no Performance Mode":page==2?"Eight banks / repeatable chains":"CC pickup and relative encoders",juce::dontSendNotification);
        grabKeyboardFocus();
    }
    repaint();
}
void GAcidBaseEditor::closePanels()
{
    juce::PopupMenu::dismissAllActiveMenus();processor.cancelMidiLearn();matrixVisible=false;showModulationMatrix();showFeaturePage(-1);
}
bool GAcidBaseEditor::keyPressed(const juce::KeyPress& key)
{
    if(key==juce::KeyPress::escapeKey){closePanels();return true;}
    if(key.getModifiers().isCtrlDown())
    {
        const auto code=juce::CharacterFunctions::toLowerCase(static_cast<juce::juce_wchar>(key.getKeyCode()));
        if(code=='z'){if(key.getModifiers().isShiftDown())processor.redoEdit();else processor.undoEdit();return true;}
        if(code=='y'){processor.redoEdit();return true;}
    }
    return false;
}
std::vector<ControlMenuEntry> GAcidBaseEditor::buildControlMenu(const juce::String& id) const
{
    std::vector<ControlMenuEntry> entries;auto* parameter=processor.parameters.getParameter(id);
    if(parameter==nullptr)return entries;
    const int index=parameter->getParameterIndex();
    entries.push_back({ 0, parameter->getName(40), true, false, true, false, {} });
    entries.push_back({ 5, "Reset to default", false, false, true, false, {} });
    entries.push_back({ 1, "MIDI Learn", false, false, true, false, {} });
    entries.push_back({ 2, "Cancel MIDI Learn", false, false, processor.getMidiLearnParameter()>=0, false, {} });
    entries.push_back({ 3, "Remove CC mapping", false, false, processor.getControllerNumber(index)>=0, false, {} });
    ControlMenuEntry modes; modes.id=0; modes.label="Controller mode"; modes.enabled=processor.getControllerNumber(index)>=0;
    const char* modeNames[]{"Jump","Pickup (soft takeover)","Relative signed","Relative offset"};
    for(int mode=0;mode<4;++mode)
        modes.children.push_back({ 20+mode, modeNames[mode], false, false, true, processor.getControllerMode(index)==mode, {} });
    entries.push_back(modes);
    for(int macro=0;macro<4;++macro)
    {
        ControlMenuEntry depths; depths.label="Macro "+juce::String(macro+1); depths.enabled=processor.canAssignMacro(index);
        const float options[]{-.5f,-.25f,0,.25f,.5f,1};
        for(int d=0;d<6;++d)
            depths.children.push_back({ 100+macro*10+d, juce::String(options[d]*100,0)+"%", false, false, true,
                                        std::abs(processor.macroDepth(macro,index)-options[d])<.001f, {} });
        entries.push_back(depths);
    }
    entries.push_back({ 0, {}, false, true, true, false, {} });
    entries.push_back({ 4, "Close menu", false, false, true, false, {} });
    return entries;
}
void GAcidBaseEditor::showControlMenu(const juce::String& id,juce::Component* target)
{
    auto* parameter=processor.parameters.getParameter(id);if(!parameter)return;
    const int index=parameter->getParameterIndex();
    juce::PopupMenu menu;
    std::function<void(const std::vector<ControlMenuEntry>&, juce::PopupMenu&)> fill;
    fill=[&](const std::vector<ControlMenuEntry>& entries, juce::PopupMenu& target)
    {
        for(const auto& entry:entries)
        {
            if(entry.separator){target.addSeparator();continue;}
            if(entry.header){target.addSectionHeader(entry.label);continue;}
            if(entry.children.empty()){target.addItem(entry.id,entry.label,entry.enabled,entry.checked);continue;}
            juce::PopupMenu sub;fill(entry.children,sub);target.addSubMenu(entry.label,sub,entry.enabled);
        }
    };
    fill(buildControlMenu(id),menu);
    juce::Component::SafePointer<GAcidBaseEditor> safe(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(target),[safe,id,index](int result)
    {
        if(!safe||result==0||result==4)return;
        if(result==1){safe->beginMidiLearnFor(id);safe->grabKeyboardFocus();return;}
        if(result==2){safe->processor.cancelMidiLearn();return;}
        safe->processor.beginEdit();
        if(result==5)
        {
            if(auto* p=safe->processor.parameters.getParameter(id)){p->beginChangeGesture();p->setValueNotifyingHost(p->getDefaultValue());p->endChangeGesture();}
        }
        else if(result==3)safe->processor.clearMidiLearnForParameter(index);
        else if(result>=20&&result<=23)safe->processor.setControllerMode(index,result-20);
        else if(result>=100&&result<=135)
        {
            const float options[]{-.5f,-.25f,0,.25f,.5f,1};const int macro=(result-100)/10,d=(result-100)%10;
            if(d<6)safe->processor.assignMacro(macro,index,options[d]);
        }
    });
}
void GAcidBaseEditor::chooseMidiExport()
{
    chooser=std::make_unique<juce::FileChooser>("Export acid pattern MIDI",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("G-AcidBase.mid"),"*.mid");
    juce::Component::SafePointer<GAcidBaseEditor> safe(this);
    chooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting,[safe](const juce::FileChooser& fc)
    {
        if(!safe||fc.getResult()==juce::File())return;
        const bool ok=safe->processor.exportMidi(fc.getResult().withFileExtension("mid"),safe->processor.value("chainEnabled")>.5f);
        safe->status.setText(ok?"MIDI exported: notes, accents, ties, timing and ratchets":"MIDI export failed",juce::dontSendNotification);
    });
}
void GAcidBaseEditor::mouseDrag(const juce::MouseEvent& event)
{
    if(event.eventComponent!=dragMidi||midiDragStarted||event.getDistanceFromDragStart()<8)return;
    midiDragStarted=true;
    draggedMidiFile=juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("G-AcidBase-pattern",".mid");
    if(!processor.exportMidi(draggedMidiFile,processor.value("chainEnabled")>.5f)){status.setText("MIDI drag export failed",juce::dontSendNotification);return;}
    if(!juce::DragAndDropContainer::performExternalDragDropOfFiles({draggedMidiFile.getFullPathName()},false,dragMidi))
        status.setText("DAW drag unavailable; use EXPORT MIDI instead",juce::dontSendNotification);
}
