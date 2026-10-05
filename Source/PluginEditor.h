#pragma once
#include "PluginProcessor.h"
#include "Logo.h"

class AcidLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AcidLookAndFeel();
    void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;
    void drawButtonText(juce::Graphics&,juce::TextButton&,bool,bool) override;
    void drawComboBox(juce::Graphics&,int,int,bool,int,int,int,int,juce::ComboBox&) override;
};

class GAcidBaseEditor;
class AcidPanel : public juce::Component
{
public:
    void paint(juce::Graphics& g) override
    {
        g.setColour(juce::Colour(0xff141d29));g.fillRoundedRectangle(getLocalBounds().toFloat(),12);
        g.setColour(juce::Colour(0xff283749));g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(.5f),12,1);
    }
};
class AcidSlider : public juce::Slider
{
public:
    void mouseDown(const juce::MouseEvent& e) override { if(!e.mods.isPopupMenu())juce::Slider::mouseDown(e); }
};

class AcidKnob : public juce::Component
{
public:
    AcidKnob(GAcidBaseProcessor&,const ParameterSpec&);
    void resized() override;
    void paint(juce::Graphics&) override;
    AcidSlider slider;
    const juce::String& getParameterID() const { return parameterID; }
private:
    friend class GAcidBaseEditor;
    juce::String title,unit,parameterID;
    GAcidBaseProcessor& processor;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

class AcidStep : public juce::Component
{
public:
    AcidStep(GAcidBaseProcessor&,int);
    void resized() override;
    void paint(juce::Graphics&) override;
    void refresh();
    void mouseDown(const juce::MouseEvent&) override;
private:
    GAcidBaseProcessor& processor;
    int index;
    AcidSlider note;
    juce::TextButton gate {"GATE"}, accent {"ACC"}, slide {"SLIDE"};
    void update();
};

// Plain description of a context menu, so the offered actions can be checked without an OS popup.
struct ControlMenuEntry
{
    int id = 0;
    juce::String label;
    bool header = false, separator = false, enabled = true, checked = false;
    std::vector<ControlMenuEntry> children;
};

class GAcidBaseEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit GAcidBaseEditor(GAcidBaseProcessor&);
    ~GAcidBaseEditor() override;
    void setScaleFactor(float newScale) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    bool keyPressed(const juce::KeyPress&) override;
    void closePanels();
    std::vector<ControlMenuEntry> buildControlMenu(const juce::String& parameterID) const;
    void showFeaturePage(int);
    bool isPanelOpen() const { return matrixVisible||featurePage>=0; }
    void beginMidiLearnFor(const juce::String& parameterID);
    juce::String getLearnStatus() const;
    void paint(juce::Graphics&) override;
    void resized() override;
    void saveUiSnapshot(const juce::File&);
    GAcidLogo::AnimationState getLogoAnimation() const { return logo; }
    void setLogoAnimation(const GAcidLogo::AnimationState& state) { logo = state; }
private:
    friend class AcidKnob;
    GAcidBaseProcessor& processor;
    AcidLookAndFeel look;
    juce::Component surface;
    std::vector<std::unique_ptr<AcidKnob>> knobs;
    std::vector<std::unique_ptr<juce::ComboBox>> combos;
    std::vector<std::unique_ptr<juce::TextButton>> toggles;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAttachments;
    std::array<std::unique_ptr<AcidStep>,16> steps;
    juce::ComboBox preset;
    juce::TextButton previous {"<"}, next {">"}, save {"SAVE"}, load {"LOAD"}, advanced {"CIRCUIT TRIMS"}, audition {"AUDITION"};
    juce::TextButton reverbTab {"REVERB"}, delayTab {"DELAY"}, eqTab {"EQ"}, chorusTab {"CHORUS"};
    juce::TextButton features {"TOOLS"}, undo {"UNDO"}, redo {"REDO"}, compare {"A / B"}, captureAB {"STORE A/B"}, closeMatrix {"CLOSE"}, cancelLearn {"CANCEL LEARN"};
    AcidPanel featurePanel;
    juce::TextButton featureClose {"CLOSE"};
    std::array<juce::TextButton,4> featureTabs;
    std::array<std::vector<juce::Component*>,4> featureControls;
    std::vector<std::unique_ptr<juce::Component>> ownedFeatureControls;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> featureSliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> featureComboAttachments;
    std::array<juce::ComboBox*,8> chainBanks {};
    std::array<juce::Slider*,8> chainRepeats {};
    juce::ComboBox* bankSelector = nullptr;
    juce::ComboBox* generatorScale = nullptr;
    juce::Slider* generatorDensity = nullptr;
    juce::Slider* generatorVariation = nullptr;
    juce::Label featureTitle;
    juce::TextButton* dragMidi = nullptr;
    int featurePage = -1;
    bool midiDragStarted = false;
    juce::File draggedMidiFile;
    juce::TextButton randomize {"MUTATE"}, resetPattern {"RESET"}, modulationToggle {"MIDI MOD MATRIX"}, clearMidi {"CLEAR LEARN"};
    juce::Label learnStatus;
    AcidPanel modulationPanel;
    std::vector<std::unique_ptr<juce::Slider>> matrixDepthSliders;
    std::vector<std::unique_ptr<juce::Label>> matrixLabels;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> matrixAttachments;
    juce::Label status;
    juce::MidiKeyboardComponent piano;
    std::unique_ptr<juce::FileChooser> chooser;
    std::vector<juce::Component*> synthControls,circuitControls;
    std::array<std::vector<juce::Component*>,4> fxControls;
    int fxPage=0, displayedPreset=-1;
    bool circuit=false,matrixVisible=false;
    float meterL=0,meterR=0;
    juce::String learnMessage;
    GAcidLogo::AnimationState logo;
    juce::int64 logoClockMs=0;
    inline static const juce::Rectangle<float> logoBounds { 24.f, 21.f, 48.f, 48.f };

    void timerCallback() override;
    AcidKnob* addKnob(const char*,juce::Rectangle<int>);
    juce::ComboBox* addCombo(const char*,juce::Rectangle<int>);
    juce::TextButton* addToggle(const char*,const char*,juce::Rectangle<int>);
    void showFx(int);
    void showModulationMatrix();
    void chooseFile(bool);
    void createFeatureControls();
    void showControlMenu(const juce::String&,juce::Component*);
    void chooseMidiExport();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GAcidBaseEditor)
};
