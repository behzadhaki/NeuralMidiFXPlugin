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

    addAndMakeVisible(&midiPianoRoll);
    setInterceptsMouseClicks(false, true);
    resized(); // Is this a terrible idea?
    startTimer(50);

    midiPianoRoll.generateRandomMidiFile(10);
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

    midiPianoRoll.setBounds(area.removeFromTop(area.proportionOfHeight(0.1)));

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
            if (midiPianoRoll.loadMidiFile(midiFileToLoad))
            {
                repaint();
                break;
            }
        }
    }
}
