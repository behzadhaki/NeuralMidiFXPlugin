#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "settings.h"

MidiFXProcessorEditor::MidiFXProcessorEditor(MidiFXProcessor& MidiFXProcessorPointer)
    : AudioProcessorEditor(&MidiFXProcessorPointer)
{
    MidiFXProcessorPointer_ = &MidiFXProcessorPointer;


    // initialize widgets
    GeneratedDrumsWidget = make_unique<FinalUIWidgets::GeneratedDrums::GeneratedDrumsWidget>(
        &MidiFXProcessorPointer_->apvts);
    {   // re-draw events if Editor reconstructed mid-session
        auto ptr_ = MidiFXProcessorPointer_->ModelThreadToDrumPianoRollWidgetQue.get();
        if (ptr_->getNumberOfWrites() > 0)
        {
            auto latest_score =
                ptr_->getLatestDataWithoutMovingFIFOHeads();
            GeneratedDrumsWidget->updateWithNewScore(latest_score);
        }
    }

    MonotonicGrooveWidget = make_unique<FinalUIWidgets::MonotonicGrooves::MonotonicGrooveWidget>
        (MidiFXProcessorPointer_->get_pointer_GroovePianoRollWidget2GrooveThread_manually_drawn_noteQue());
    {   // re-draw events if Editor reconstructed mid-session
        auto ptr_ = MidiFXProcessorPointer_->GrooveThread2GGroovePianoRollWidgetQue.get();
        if (ptr_->getNumberOfWrites() > 0)
        {
            auto latest_groove =
                ptr_->getLatestDataWithoutMovingFIFOHeads();
            MonotonicGrooveWidget->updateWithNewGroove(latest_groove);
        }
    }

    // Add widgets to Main Editor GUI
    addAndMakeVisible(GeneratedDrumsWidget.get());
    addAndMakeVisible(MonotonicGrooveWidget.get());

    // Progress Bar
    playhead_pos = MidiFXProcessorPointer_->get_playhead_pos();
    PlayheadProgressBar.setColour(PlayheadProgressBar.foregroundColourId, playback_progressbar_color);
    addAndMakeVisible(PlayheadProgressBar);

    // add buttons
    ButtonsWidget = make_unique<FinalUIWidgets::ButtonsWidget>(&MidiFXProcessorPointer_->apvts);
    addAndMakeVisible (ButtonsWidget.get());
    // ButtonsWidget->addListener(this);

    // initialize GrooveControlSliders
    ControlsWidget = make_unique<FinalUIWidgets::ControlsWidget> (&MidiFXProcessorPointer_->apvts);
    addAndMakeVisible (ControlsWidget.get());

    // model selector
    ModelSelectorWidget = make_unique<FinalUIWidgets::ModelSelectorWidget> (&MidiFXProcessorPointer_->apvts,
                                                                           "MODEL",
                                                                           MidiFXProcessorPointer_->model_paths);
    addAndMakeVisible (ModelSelectorWidget.get());


    // Set window size
    setResizable (true, true);
    setSize (1400, 1000);

    startTimer(50);

}

MidiFXProcessorEditor::~MidiFXProcessorEditor()
{
    //ButtonsWidget->removeListener(this);
    // ModelSelectorWidget->removeListener(this);
}

void MidiFXProcessorEditor::resized()
{
    auto area = getLocalBounds();
    setBounds(area);                            // bounds for main Editor GUI

    // reserve right side for other controls
    area.removeFromRight(proportionOfWidth(gui_settings::PianoRolls::space_reserved_right_side_of_gui_ratio_of_width));

    // layout pianoRolls for generated drums at top
    GeneratedDrumsWidget->setBounds (area.removeFromTop(area.proportionOfHeight(gui_settings::PianoRolls::completePianoRollHeight))); // piano rolls at top

    // layout pianoRolls for generated drums on top
    MonotonicGrooveWidget->setBounds(area.removeFromBottom(area.proportionOfHeight(gui_settings::PianoRolls::completeMonotonicGrooveHeight))); // groove at bottom

    // layout Playhead Progress Bar
    area.removeFromLeft(area.proportionOfWidth(gui_settings::PianoRolls::label_ratio_of_width));
    PlayheadProgressBar.setBounds(area.removeFromLeft(
        GeneratedDrumsWidget->PianoRolls[0]->getPianoRollSectionWidth()));

    // put buttons and GrooveControlSliders
    area = getLocalBounds();
    area.removeFromLeft(area.proportionOfWidth(1.0f - gui_settings::PianoRolls::space_reserved_right_side_of_gui_ratio_of_width));
    ButtonsWidget->setBounds(area.removeFromTop(area.proportionOfHeight(0.3)));
    ModelSelectorWidget->setBounds(area.removeFromTop(area.proportionOfHeight(0.1)));
    ControlsWidget->setBounds(area.removeFromBottom(area.proportionOfHeight(0.4)));

}

void MidiFXProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(
        juce::ResizableWindow::backgroundColourId));

}

void MidiFXProcessorEditor::timerCallback()
{
    // get Generations and probs from model thread to display on drum piano rolls
    {
        auto ptr_ = MidiFXProcessorPointer_->ModelThreadToDrumPianoRollWidgetQue.get();
        if (ptr_->getNumReady() > 0)
        {
            GeneratedDrumsWidget->updateWithNewScore(
                ptr_->getLatestOnly());
        }
    }

    // get Grooves from GrooveThread to display in Groove Piano Rolls
    {
        auto ptr_ = MidiFXProcessorPointer_->GrooveThread2GGroovePianoRollWidgetQue.get();

        if (ptr_->getNumReady() > 0)
        {
            MonotonicGrooveWidget->updateWithNewGroove(
                ptr_->getLatestOnly());
        }

    }

    // get playhead position to display on progress bar
    playhead_pos = MidiFXProcessorPointer_->get_playhead_pos();
    GeneratedDrumsWidget->UpdatePlayheadLocation(playhead_pos);
}