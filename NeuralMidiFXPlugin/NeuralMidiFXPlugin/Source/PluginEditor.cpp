#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "settings.h"

NeuralMidiFXPluginEditor::NeuralMidiFXPluginEditor(NeuralMidiFXPluginProcessor& NeuralMidiFXPluginProcessorPointer)
    : AudioProcessorEditor(&NeuralMidiFXPluginProcessorPointer),
    tabs (juce::TabbedButtonBar::Orientation::TabsAtTop)
{
    NeuralMidiFXPluginProcessorPointer_ = &NeuralMidiFXPluginProcessorPointer;

    // Set window size
    setResizable (true, true);
    setSize (800, 600);

    // Iteratively generate tabs. For each tab, a parent UI component of  will be created.
    for (int i = 0; i < numTabs; i++)
    {
        currentTab = UIObjects::Tabs::tabList[i];
        tabName = std::get<0>(currentTab);

        paramComponentPtr = new ParameterComponent(currentTab);

        paramComponentVector.push_back(paramComponentPtr);
        addAndMakeVisible(*paramComponentPtr);
        paramComponentPtr->generateGuiElements(getLocalBounds(), &NeuralMidiFXPluginProcessorPointer_->apvts);
        paramComponentPtr->resizeGuiElements(getLocalBounds());

        tabs.addTab(tabName, juce::Colours::darkgrey, paramComponentPtr, true);
    }

    addAndMakeVisible(tabs);

    resized(); // Is this a terrible idea?
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
    setBounds(area);

    tabs.setBounds(area);

    auto widthEdge = area.getWidth() * .05;
    auto heightEdge = area.getHeight() * .03;

//    area.removeFromTop(heightEdge);
//    area.removeFromBottom(heightEdge);
//    area.removeFromLeft(widthEdge);
//    area.removeFromRight(widthEdge);

    for (int i = 0; i < paramComponentVector.size(); i++)
    {
        paramComponentVector[i]->resizeGuiElements(area);
    }

    //parameterElements.resizeGuiElements(area);


}

void NeuralMidiFXPluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

}

void NeuralMidiFXPluginEditor::timerCallback()
{

}

