#include "PluginProcessor.h"
#include "PluginEditor.h"

inline double mapToLoopRange(double value, double loopStart, double loopEnd) {

    double loopDuration = loopEnd - loopStart;

    // Compute the offset relative to the loop start
    double offset = fmod(value - loopStart, loopDuration);

    // Handle negative values if 'value' is less than 'loopStart'
    if (offset < 0) offset += loopDuration;

    // Add the offset to the loop start to get the mapped value
    double mappedValue = loopStart + offset;

    return mappedValue;
}

static string label2ParamID_(const string &label) {
    // capitalize label
    string paramID = label;
    std::transform(paramID.begin(), paramID.end(), paramID.begin(), ::toupper);
    return paramID;
}

NeuralMidiFXPluginEditor::NeuralMidiFXPluginEditor(NeuralMidiFXPluginProcessor& NeuralMidiFXPluginProcessorPointer)
    : AudioProcessorEditor(&NeuralMidiFXPluginProcessorPointer),
    tabs (juce::TabbedButtonBar::Orientation::TabsAtTop)
{

    NeuralMidiFXPluginProcessorPointer_ = &NeuralMidiFXPluginProcessorPointer;
    presetManagerWidget = std::make_unique<PresetTableComponent>(
        NeuralMidiFXPluginProcessorPointer.apvts,
        NeuralMidiFXPluginProcessorPointer.deploymentThread->CustomPresetData.get());

    // shared label for hover text
    sharedHoverText = make_unique<juce::Label>();
    sharedHoverText->setJustificationType(juce::Justification::left);
    sharedHoverText->setColour(juce::Label::ColourIds::textColourId, juce::Colours::whitesmoke);
    sharedHoverText->setColour(juce::Label::ColourIds::backgroundColourId, juce::Colours::darkgrey);

    addAndMakeVisible(sharedHoverText.get());

    // Set window sizes
    setResizable (UIObjects::user_resizable,true);

    auto l1 = std::max(UIObjects::user_width/4, 20);
    auto l2 = std::max(UIObjects::user_height/4, 20);

    setResizeLimits (l1, l2, 10000, 10000);

    setSize (UIObjects::user_width, UIObjects::user_height);

    // if standalone, add play, record, tempo, meter controls to the top
    // check if standalone mode
    shouldActStandalone = false;
    if (JUCEApplicationBase::isStandaloneApp()) {
        if (UIObjects::StandaloneTransportPanel::enable) {
            shouldActStandalone = true;
        }
    } else {
        if (UIObjects::StandaloneTransportPanel::enable &&
            !UIObjects::StandaloneTransportPanel::disableInPluginMode) {
            shouldActStandalone = true;
        }
    }

    if (shouldActStandalone) {
        tempoMeterWidget = std::make_unique<StandaloneControlsWidget>(
            NeuralMidiFXPluginProcessorPointer.apvts);
        addAndMakeVisible(*tempoMeterWidget);
    }


    // Iteratively generate tabs. For each tab, a parent UI component of  will be created.
    for (int i = 0; i < numTabs; i++)
    {
        currentTab = UIObjects::Tabs::tabList[i];
        tabName = std::get<0>(currentTab);

        paramComponentPtr = new ParameterComponent(currentTab, sharedHoverText.get());

        paramComponentVector.push_back(paramComponentPtr);
        addAndMakeVisible(*paramComponentPtr);
        paramComponentPtr->generateGuiElements(
            getLocalBounds(), &NeuralMidiFXPluginProcessorPointer_->apvts);

        tabs.addTab(tabName, juce::Colours::lightgrey,
                    paramComponentPtr, true);
    }

    // add preset manager tab
    addAndMakeVisible(*presetManagerWidget);
//    tabs.addTab("Preset Manager", juce::Colours::lightgrey,
//                &presetManagerWidget, true);


    addAndMakeVisible(tabs);

    NMP2GUI_IncomingMessageSequence = NeuralMidiFXPluginProcessorPointer.NMP2GUI_IncomingMessageSequence.get();

    juce::MidiMessageSequence seq_;
    if (NMP2GUI_IncomingMessageSequence->getNumberOfWrites() > 0)
    {
        seq_ = NMP2GUI_IncomingMessageSequence->getLatestDataWithoutMovingFIFOHeads();
    } else {
        seq_ = juce::MidiMessageSequence();
    }

    double len = 8.0f * 960;

    if (UIObjects::MidiInVisualizer::enable) {
        inputPianoRoll = std::make_unique<InputMidiPianoRollComponent>(
            NeuralMidiFXPluginProcessorPointer.GUI2DPL_DroppedMidiFile_Que.get(),
            seq_);
        len = std::max(len, inputPianoRoll->getLength());
    }

    if (UIObjects::GeneratedContentVisualizer::enable) {
        outputPianoRoll = std::make_unique<OutputMidiPianoRollComponent>(
            NeuralMidiFXPluginProcessorPointer.DPL2GUI_GenerationMidiFile_Que.get());
        len = std::max(len, outputPianoRoll->getLength());
    }


    if (inputPianoRoll != nullptr) {
        inputPianoRoll->setLength(len);
        addAndMakeVisible(inputPianoRoll.get());
    }

    if (outputPianoRoll != nullptr) {
        outputPianoRoll->setLength(len);
        addAndMakeVisible(outputPianoRoll.get());
    }

    setInterceptsMouseClicks(false, true);

    startTimer(50);

    // iterate through all the tabs and set the tabbed component to the first tab
    for (int i = 0; i < (numTabs+1); i++)
    {
        tabs.setCurrentTabIndex(i);
        tabs.resized();
        tabs.setInterceptsMouseClicks(false, true);
    }
    tabs.setCurrentTabIndex(0);

    // provide resources for midi visualizers
    for (auto & i : paramComponentVector)
    {
        for (auto & mV : i->midiDisplayArray)
        {
            auto paramID = mV->getParamID();
            auto vis_data = (NeuralMidiFXPluginProcessorPointer_->midiVisualizersData.get());
            mV->setPianoRollData(vis_data->getVisualizerResources(paramID));
        }
    }

    // provide resources for audio visualizers
    for (auto & i : paramComponentVector)
    {
        for (auto & aV : i->audioDisplayArray)
        {
            auto paramID = aV->getParamID();
            auto vis_data = (NeuralMidiFXPluginProcessorPointer_->audioVisualizersData.get());
            aV->setAudioData(vis_data->getVisualizerResources(paramID));
        }
    }

//    getLookAndFeel().setColour(juce::Slider::thumbColourId, juce::Colours::blueviolet);
//    getLookAndFeel().setColour(juce::Slider::trackColourId, juce::Colours::blueviolet);
    resized();
}

void NeuralMidiFXPluginEditor::resized()
{
    if (UIObjects::user_resizable) {
        if (UIObjects::user_maintain_aspect_ratio) {
            // Retrieve the current bounds of the window
            auto bounds = getLocalBounds();

            // Calculate the desired aspect ratio
            const float aspectRatio = static_cast<float>(UIObjects::user_width) / static_cast<float>(UIObjects::user_height);

            // Calculate the new width and height while maintaining the aspect ratio
            int newWidth = bounds.getWidth();
            int newHeight = static_cast<int>(newWidth / aspectRatio);

            // Check if the new height is greater than the available bounds, adjust if necessary
            if (newHeight > bounds.getHeight()) {
                newHeight = bounds.getHeight();
                newWidth = static_cast<int>(newHeight * aspectRatio);
            }

            // Apply the new bounds to the window
            auto x = std::max(bounds.getX(), 0);
            auto y = std::max(bounds.getY(), 0);
            setBounds(x , y , newWidth, newHeight);

        }
    } else {
        setSize(UIObjects::user_width, UIObjects::user_height);
    }
}

void NeuralMidiFXPluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    auto area = getLocalBounds();

    int preset_manager_width = int(area.getWidth() * .10);

    int standalone_control_height;
    int proll_height;
    int gap;

    // place shared hover text at the top
    sharedHoverText->setBounds(area.removeFromBottom(int(area.getHeight() * .02)));
    sharedHoverText->setFont(juce::Font(float(sharedHoverText->getHeight() * .8)));

    // place preset manager at the top
    area.removeFromLeft(int(area.getWidth() * .02));
    presetManagerWidget->setBounds(area.removeFromLeft(preset_manager_width));
    area.removeFromLeft(int(area.getWidth() * .02));

    // check if standalone
    if (shouldActStandalone) {
        standalone_control_height = int(area.getHeight() * .05);
        proll_height = int(area.getHeight() * .1);
        gap = int(area.getHeight() * .03);
    } else {
        standalone_control_height = 0;
        proll_height = int(area.getHeight() * .1);
        gap = int(area.getHeight() * .03);
    }

    if (shouldActStandalone) {
        auto controlArea = area.removeFromTop(standalone_control_height);
        auto cAreaGap = controlArea.getHeight() * .05;
        int width = int(controlArea.getWidth() * .2);

        // leave a gap above and below the standalone controls
        area.removeFromTop((int) cAreaGap);
        area.removeFromBottom((int) cAreaGap);

        // layout the standalone controls
        tempoMeterWidget->setBounds(controlArea);

    }

    if (outputPianoRoll != nullptr){
        outputPianoRoll->setBounds(area.removeFromBottom(proll_height));
    }

    if (inputPianoRoll != nullptr && outputPianoRoll != nullptr){
        area.removeFromBottom(gap);
    }

    if (inputPianoRoll != nullptr){
        inputPianoRoll->setBounds(area.removeFromBottom(proll_height));
    }

    tabs.setBounds(area);
    area.removeFromBottom(gap);
    for (auto & i : paramComponentVector)
    {
        i->resizeGuiElements(/*area*/);
    }
}

void NeuralMidiFXPluginEditor::timerCallback()
{

    bool newContent = false;
    bool newPlayheadPos = false;
    auto fs_ = NeuralMidiFXPluginProcessorPointer_->generationsToDisplay.getFs();

    if (fs_ != std::nullopt) {
        if ((*fs_ - fs) > 0.0001) {
            fs = *fs_;
            newContent = true;
        }
    }

    auto qpm_ = NeuralMidiFXPluginProcessorPointer_->generationsToDisplay.getQpm();
    if (qpm_ != std::nullopt) {
        if ((*qpm_ - qpm) > 0.0001) {
            qpm = *qpm_;
            newContent = true;
        }
    }

    auto policy_ = NeuralMidiFXPluginProcessorPointer_->generationsToDisplay.getPolicy();
    if (policy_ != std::nullopt) {
        play_policy = *policy_;
        newContent = true;
        if (policy_->getLoopDuration() > 0) {
            NeuralMidiFXPluginProcessorPointer_->playbackAnchorMutex.lock();
            LoopStart = NeuralMidiFXPluginProcessorPointer_->TimeAnchor.inQuarterNotes();
            NeuralMidiFXPluginProcessorPointer_->playbackAnchorMutex.unlock();
            LoopEnd = LoopStart + policy_->getLoopDuration();
            LoopingEnabled = true;
        } else {
            LoopingEnabled = false;
            LoopStart = 0;
            LoopEnd = 0;
        }
    }

    auto sequence_to_display_ = NeuralMidiFXPluginProcessorPointer_->generationsToDisplay.getSequence();
    if (sequence_to_display_ != std::nullopt) {
        sequence_to_display = *sequence_to_display_;
        newContent = true;

    }

    auto playhead_pos_ = NeuralMidiFXPluginProcessorPointer_->generationsToDisplay.getPlayheadPos();
    if (playhead_pos_ != std::nullopt){

        if (std::abs(*playhead_pos_ - playhead_pos) > 0.0001) {
            playhead_pos = *playhead_pos_;
            newPlayheadPos = true;
        }
    }

    if (NMP2GUI_IncomingMessageSequence->getNumReady() > 0) {
        incoming_sequence = NMP2GUI_IncomingMessageSequence->pop();
        if (inputPianoRoll != nullptr)
        {
            inputPianoRoll->displayMidiMessageSequence(incoming_sequence);
            newContent = true;
        }
    }

    if (newPlayheadPos)
    {
        if (inputPianoRoll != nullptr) {
            inputPianoRoll->displayMidiMessageSequence(playhead_pos);
        }

        if (outputPianoRoll != nullptr) {
            if (LoopingEnabled) {
                auto adjusted_playhead_pos = mapToLoopRange(playhead_pos, LoopStart, LoopEnd);
                outputPianoRoll->displayMidiMessageSequence(adjusted_playhead_pos);
            } else {
                outputPianoRoll->displayMidiMessageSequence(playhead_pos);
            }
        }

        // update playhead for user specified midi visualizers
        for (auto & i : paramComponentVector)
        {
            for (auto & mV : i->midiDisplayArray)
            {
                auto paramID = mV->getParamID();
                mV->updatePlayhead(playhead_pos);
            }
        }
    }

    if (newContent)
    {
        if (outputPianoRoll != nullptr) {
            if (LoopingEnabled) {
                outputPianoRoll->displayLoopedMidiMessageSequence(
                    sequence_to_display,
                    play_policy,
                    fs,
                    qpm,
                    LoopStart,
                    LoopEnd
                );
            } else {
                outputPianoRoll->displayMidiMessageSequence(
                    sequence_to_display,
                    play_policy,
                    fs,
                    qpm
                );
            }
        }
    }

    if (newContent || newPlayheadPos)
    {

        double len = 8.0f * 960;
        if (inputPianoRoll != nullptr) {
            len = std::max(inputPianoRoll->getLength(), len);
        }
        if (outputPianoRoll != nullptr) {
            len = std::max(outputPianoRoll->getLength(), len);
        }

        if (inputPianoRoll != nullptr) {
            inputPianoRoll->setLength(len);
            inputPianoRoll->repaint();
        }

        if (outputPianoRoll != nullptr) {
            if (LoopingEnabled) {
                len = std::max(outputPianoRoll->getMidiLength(), LoopEnd * 960);
            }
            outputPianoRoll->setLength(len);
            outputPianoRoll->repaint();
        }

       repaint();
    }

}

