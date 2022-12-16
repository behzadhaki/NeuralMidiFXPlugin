#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "settings.h"

NeuralMidiFXPluginEditor::NeuralMidiFXPluginEditor(NeuralMidiFXPluginProcessor& NeuralMidiFXPluginProcessorPointer)
    : AudioProcessorEditor(&NeuralMidiFXPluginProcessorPointer)
{



    NeuralMidiFXPluginProcessorPointer_ = &NeuralMidiFXPluginProcessorPointer;
    


    // Set window size
    setResizable (true, true);
    setSize (1400, 1000);

    startTimer(50);

}

NeuralMidiFXPluginEditor::~NeuralMidiFXPluginEditor()
{
    //ButtonsWidget->removeListener(this);
    // ModelSelectorWidget->removeListener(this);
}

void NeuralMidiFXPluginEditor::resized()
{
    auto area = getLocalBounds();
    setBounds(area);                            // bounds for main Editor GUI

    // reserve right side for other controls
    area.removeFromRight(proportionOfWidth(gui_settings::PianoRolls::space_reserved_right_side_of_gui_ratio_of_width));


}

void NeuralMidiFXPluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(
        juce::ResizableWindow::backgroundColourId));

}

void NeuralMidiFXPluginEditor::timerCallback()
{

}