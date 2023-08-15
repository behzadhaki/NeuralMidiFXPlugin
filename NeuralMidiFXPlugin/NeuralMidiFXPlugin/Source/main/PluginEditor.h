#pragma once

#include "PluginProcessor.h"
#include "../src/Includes/GuiElements.h"
#include "../src/Includes/MidiDisplayWidget.h"

using namespace std;

class NeuralMidiFXPluginEditor : public juce::AudioProcessorEditor,
                                 public juce::Timer,
                                 public juce::FileDragAndDropTarget

{
public:
    explicit NeuralMidiFXPluginEditor(NeuralMidiFXPluginProcessor&);
    ~NeuralMidiFXPluginEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;

    void filesDropped (const juce::StringArray& files, int /*x*/, int /*y*/) override;


    juce::TabbedComponent tabs;

    const int numTabs = UIObjects::Tabs::tabList.size();

    ParameterComponent* paramComponentPtr;
    std::vector<ParameterComponent*> paramComponentVector;

    unique_ptr<InputMidiPianoRollComponent> inputPianoRoll{nullptr};
    unique_ptr<OutputMidiPianoRollComponent> outputPianoRoll{nullptr};



private:
    NeuralMidiFXPluginProcessor* NeuralMidiFXPluginProcessorPointer_;
    UIObjects::tab_tuple currentTab;
    std::string tabName;
    double fs;
    double qpm;
    double playhead_pos;
    PlaybackPolicies play_policy;
    juce::MidiMessageSequence sequence_to_display;
    LockFreeQueue<juce::MidiMessageSequence, 32>* NMP2GUI_IncomingMessageSequence;
    bool LoopingEnabled {false};
    double LoopStart {0};
    double LoopEnd {0};
    juce::MidiMessageSequence incoming_sequence;
};

