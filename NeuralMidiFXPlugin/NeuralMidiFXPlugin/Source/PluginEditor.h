#pragma once

#include "PluginProcessor.h"
#include "Includes/GuiElements.h"
#include "Includes/MidiDisplayWidget.h"

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

    unique_ptr<MidiPianoRollComponent> inputPianoRoll{nullptr};
    unique_ptr<MidiPianoRollComponent> outputPianoRoll{nullptr};

private:

    NeuralMidiFXPluginProcessor* NeuralMidiFXPluginProcessorPointer_;
    UIObjects::tab_tuple currentTab;
    std::string tabName;
    double fs;
    double qpm;
    double playhead_pos;
    PlaybackPolicies play_policy;
    juce::MidiMessageSequence sequence_to_display;
};

