#pragma once

#include "PluginProcessor.h"
#include "gui/CustomUIWidgets.h"

using namespace std;

class NeuralMidiFXPluginEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    explicit NeuralMidiFXPluginEditor(NeuralMidiFXPluginProcessor&) ;
    ~NeuralMidiFXPluginEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    /*void comboBoxChanged(juce::ComboBox* comboBox) override;*/

private:
    NeuralMidiFXPluginProcessor* NeuralMidiFXPluginProcessorPointer_;

    // gui widgets
    unique_ptr<FinalUIWidgets::GeneratedDrums::GeneratedDrumsWidget> GeneratedDrumsWidget;
    unique_ptr<FinalUIWidgets::MonotonicGrooves::MonotonicGrooveWidget>
        MonotonicGrooveWidget;

    //  GrooveControlSliders
    unique_ptr<FinalUIWidgets::ControlsWidget> ControlsWidget;

    // buttons
    unique_ptr<FinalUIWidgets::ButtonsWidget> ButtonsWidget;

    // model selector
    unique_ptr<FinalUIWidgets::ModelSelectorWidget> ModelSelectorWidget;

    // playhead position progress bar
    double playhead_pos;
    juce::ProgressBar PlayheadProgressBar {playhead_pos};

    // vel offset ranges
    array<float, 4> VelOffRanges {HVO_params::_min_vel, HVO_params::_max_vel, HVO_params::_min_offset, HVO_params::_max_offset};

};

