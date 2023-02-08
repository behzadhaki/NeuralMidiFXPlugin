#pragma once

#include "PluginProcessor.h"
#include "Includes/GuiElements.h"

using namespace std;

class NeuralMidiFXPluginEditor : public juce::AudioProcessorEditor,
                                 public juce::Timer

{
public:
    explicit NeuralMidiFXPluginEditor(NeuralMidiFXPluginProcessor&);
    ~NeuralMidiFXPluginEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    juce::TabbedComponent tabs;

    const int numTabs = UIObjects::Tabs::tabList.size();


    ParameterComponent* paramComponentPtr;
    std::vector<ParameterComponent*> paramComponentVector;

private:

    NeuralMidiFXPluginProcessor* NeuralMidiFXPluginProcessorPointer_;
    UIObjects::tab_tuple currentTab;
    std::string tabName;
};

