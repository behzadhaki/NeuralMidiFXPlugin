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
        paramComponentPtr->generateGuiElements(
            getLocalBounds(), &NeuralMidiFXPluginProcessorPointer_->apvts);
        paramComponentPtr->resizeGuiElements(getLocalBounds());

        tabs.addTab(tabName, juce::Colours::lightgrey,
                    paramComponentPtr, true);
    }

    addAndMakeVisible(tabs);

    NMP2GUI_IncomingMessageSequence = NeuralMidiFXPluginProcessorPointer.NMP2GUI_IncomingMessageSequence.get();

    juce::MidiMessageSequence seq_;
    if (NMP2GUI_IncomingMessageSequence->getNumberOfWrites() > 0)
    {
        seq_ = NMP2GUI_IncomingMessageSequence->getLatestDataWithoutMovingFIFOHeads();
    } else {
        seq_ = juce::MidiMessageSequence();
    }

    inputPianoRoll = std::make_unique<InputMidiPianoRollComponent>(
        NeuralMidiFXPluginProcessorPointer.GUI2ITP_DroppedMidiFile_Que.get(),
        seq_);

    outputPianoRoll = std::make_unique<OutputMidiPianoRollComponent>(
        NeuralMidiFXPluginProcessorPointer.PPP2GUI_GenerationMidiFile_Que.get());

    auto len = std::max(inputPianoRoll->getLength(), outputPianoRoll->getLength());
    len = std::max(double(8.0f * 960), len);
    inputPianoRoll->setLength(len);
    outputPianoRoll->setLength(len);

    addAndMakeVisible(inputPianoRoll.get());
    addAndMakeVisible(outputPianoRoll.get());

//    inputPianoRoll->generateRandomMidiFile(10);
//    outputPianoRoll->generateRandomMidiFile(10);

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

    if (outputPianoRoll != nullptr){
        outputPianoRoll->setBounds(area.removeFromBottom(proll_height));
    }
    if (inputPianoRoll != nullptr || outputPianoRoll != nullptr){
        area.removeFromBottom(gap);
    }
    if (inputPianoRoll != nullptr){
        inputPianoRoll->setBounds(area.removeFromBottom(proll_height));
    }

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
    bool newContent = false;
    bool newPlayheadPos = false;
    auto fs_ = NeuralMidiFXPluginProcessorPointer_->generationsToDisplay.getFs();
    if (fs_.value() != fs)
    {
        fs = fs_.value();
        newContent = true;
    }

    auto qpm_ = NeuralMidiFXPluginProcessorPointer_->generationsToDisplay.getQpm();
    if (qpm_.value() != qpm)
    {
        qpm = qpm_.value();
        newContent = true;
    }


    auto policy_ = NeuralMidiFXPluginProcessorPointer_->generationsToDisplay.getPolicy();
    if (policy_.has_value())
    {
        play_policy = policy_.value();
        newContent = true;
    }

    auto sequence_to_display_ = NeuralMidiFXPluginProcessorPointer_->generationsToDisplay.getSequence();
    if (sequence_to_display_.has_value())
    {
        sequence_to_display = sequence_to_display_.value();
        newContent = true;
    }

    auto playhead_pos_ = NeuralMidiFXPluginProcessorPointer_->generationsToDisplay.getPlayheadPos();
    if (playhead_pos_.value() != playhead_pos)
    {
        playhead_pos = playhead_pos_.value();
        newPlayheadPos = true;
    }

    if (NMP2GUI_IncomingMessageSequence->getNumReady() > 0)
    {
        cout << "NMP2GUI_IncomingMessageSequence->getNumReady() = " << NMP2GUI_IncomingMessageSequence->getNumReady() << endl;
        incoming_sequence = NMP2GUI_IncomingMessageSequence->pop();
        cout << "incoming_sequence.getNumEvents() = " << incoming_sequence.getNumEvents() << endl;
        if (incoming_sequence.getNumEvents() > 0)
        {
            cout << incoming_sequence.getEventPointer(0)->message.getDescription() << endl;
        }
        inputPianoRoll->displayMidiMessageSequence(incoming_sequence);
        newContent = true;
    }

    if (newContent)
    {
        outputPianoRoll->displayMidiMessageSequence(
            sequence_to_display,
            play_policy,
            fs,
            qpm
            );
    }

    if (newPlayheadPos)
    {
        inputPianoRoll->displayMidiMessageSequence(playhead_pos);
        outputPianoRoll->displayMidiMessageSequence(playhead_pos);
    }

    if (newContent || newPlayheadPos)
    {
        auto len = std::max(inputPianoRoll->getLength(), outputPianoRoll->getLength());
        inputPianoRoll->setLength(len);
        outputPianoRoll->setLength(len);
      inputPianoRoll->repaint();
      outputPianoRoll->repaint();
        repaint();
    }

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
            if (inputPianoRoll->loadMidiFile(midiFileToLoad))
            {
                repaint();
                break;
            }
        }
    }
}
