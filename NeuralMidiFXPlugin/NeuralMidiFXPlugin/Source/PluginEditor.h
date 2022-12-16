#pragma once

#include "PluginProcessor.h"

using namespace std;

class NeuralMidiFXPluginEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    explicit NeuralMidiFXPluginEditor(NeuralMidiFXPluginProcessor&) ;
    ~NeuralMidiFXPluginEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    NeuralMidiFXPluginProcessor* NeuralMidiFXPluginProcessorPointer_;

};

