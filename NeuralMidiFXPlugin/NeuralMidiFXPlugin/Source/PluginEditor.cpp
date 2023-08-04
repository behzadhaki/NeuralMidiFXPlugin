#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "DeploymentSettings/GuiAndParams.h"


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

        tabs.addTab(tabName, juce::Colours::lightgrey, paramComponentPtr, true);
    }

    addAndMakeVisible(tabs);


    addAndMakeVisible(&inputPianoRoll);
    addAndMakeVisible(&outputPianoRoll);


    inputPianoRoll.generateRandomMidiFile(10);
    outputPianoRoll.generateRandomMidiFile(10);

    setInterceptsMouseClicks(false, true);

    startTimer(50);
    resized(); // Is this a terrible idea?


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

    auto proll_height = int(area.getHeight() * .1);
    auto gap = int(area.getHeight() * .03);

    outputPianoRoll.setBounds(area.removeFromBottom(proll_height));
    area.removeFromBottom(gap);
    inputPianoRoll.setBounds(area.removeFromBottom(proll_height));

    tabs.setBounds(area);

//    auto widthEdge = area.getWidth() * .05;
//    auto heightEdge = area.getHeight() * .03;

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

bool NeuralMidiFXPluginEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& file : files)
        if (file.endsWith (".mid") || file.endsWith (".midi"))
            return true;

    return false;
}

void NeuralMidiFXPluginEditor::filesDropped (const juce::StringArray& files, int /*x*/, int /*y*/)
{
    for (auto& file : files)
    {
        if (file.endsWith(".mid") || file.endsWith(".midi"))
        {
            juce::File midiFileToLoad(file);
            if (inputPianoRoll.loadMidiFile(midiFileToLoad))
            {
                repaint();
                break;
            }
        }
    }
}
