#pragma once

#include "PluginProcessor.h"
#include "../Includes/GuiElements.h"
#include "../Includes/MidiDisplayWidget.h"
#include "../Includes/PresetManagerWidget.h"
#include "../Includes/StandaloneControlsWidget.h"

using namespace std;

class NeuralMidiFXPluginEditor : public juce::AudioProcessorEditor,
                                 public juce::Timer

{
public:
    explicit NeuralMidiFXPluginEditor(NeuralMidiFXPluginProcessor&);
    ~NeuralMidiFXPluginEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    juce::TabbedComponent tabs;

    const int numTabs = (int) UIObjects::Tabs::tabList.size();

    ParameterComponent* paramComponentPtr;
    std::vector<ParameterComponent*> paramComponentVector;

    unique_ptr<InputMidiPianoRollComponent> inputPianoRoll{nullptr};
    unique_ptr<OutputMidiPianoRollComponent> outputPianoRoll{nullptr};

    // standalone controls Play Button, Record Button, Tempo Rotary, and Loop Button (with attachments)
    // add a juce button amd attachment
    std::unique_ptr<StandaloneControlsWidget> tempoMeterWidget;
    unique_ptr<PresetTableComponent> presetManagerWidget;

    // shared label for info
    std::unique_ptr<juce::Label> sharedHoverText;

    /*void saveAPVTSToFile(int preset_idx);
    void loadAPVTSFromFile(int preset_idx);*/
private:
    NeuralMidiFXPluginProcessor* NeuralMidiFXPluginProcessorPointer_;
    tab_tuple currentTab;
    std::string tabName;
    double fs{};
    double qpm{};
    double playhead_pos{};
    PlaybackPolicies play_policy;
    juce::MidiMessageSequence sequence_to_display;
    LockFreeQueue<juce::MidiMessageSequence, 32>* NMP2GUI_IncomingMessageSequence;
    bool LoopingEnabled {false};
    double LoopStart {0};
    double LoopEnd {0};
    juce::MidiMessageSequence incoming_sequence;
    bool shouldActStandalone {false};

};

