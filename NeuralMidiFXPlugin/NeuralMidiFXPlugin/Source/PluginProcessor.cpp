#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Includes/UtilityMethods.h"
#include "Includes/EventTracker.h"

using namespace std;



NeuralMidiFXPluginProcessor::NeuralMidiFXPluginProcessor():
    apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    //////////////////////////////////////////////////////////////////
    //// Make_unique pointers for Queues
    //////////////////////////////////////////////////////////////////


    //////////////////////////////////////////////////////////////////
    //// Create shared pointers for Threads (shared with APVTSMediator)
    //////////////////////////////////////////////////////////////////
    inputTensorPreparatorThread = make_shared<InputTensorPreparatorThread>();

    /////////////////////////////////
    //// Start Threads
    /////////////////////////////////

    // give access to resources and run threads

    inputTensorPreparatorThread->startThreadUsingProvidedResources();
}

NeuralMidiFXPluginProcessor::~NeuralMidiFXPluginProcessor(){
    if (!inputTensorPreparatorThread->readyToStop)
    {
        inputTensorPreparatorThread->prepareToStop();
    }
}

void NeuralMidiFXPluginProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer& midiMessages)
{
    tempBuffer.clear();

    // STEP 1
    // get Playhead info and buffer size and sample rate from host
    auto playhead = getPlayHead();
    auto Pinfo = playhead->getPosition();
    auto fs = getSampleRate();
    auto buffSize = buffer.getNumSamples();

    // STEP 2
    // check if new pattern is generated and available for playback


    // Step 3
    // In playback mode, add drum note to the buffer if the time is right
    if (Pinfo->getIsPlaying())
    {

        if (*Pinfo->getPpqPosition() < startPpq)
        {
            // if playback head moved backwards or playback paused and restarted
            // change the registration_times of groove events to ensure the
            // groove properly overdubs

        }

        startPpq = *Pinfo->getPpqPosition();
        auto qpm = *Pinfo->getBpm();
        auto start_ = fmod(startPpq, HVO_params::time_steps/4); // start_ should be always between 0 and 8
        playhead_pos = fmod(float(startPpq + HVO_params::_32_note_ppq), float(HVO_params::time_steps/4.0f)) / (HVO_params::time_steps/4.0f);

        auto new_grid = floor(start_/HVO_params::_16_note_ppq);
        if (new_grid != current_grid) {
            current_grid = new_grid;

        }

    }

    // Step 4. see if new notes are played on the input side
    if (not midiMessages.isEmpty() /*and groove_thread_ready*/)
    {

        // send BasicNotes to the GrooveThread and also gui logger for notes


    }

    midiMessages.swapWith(tempBuffer);


    buffer.clear(); //
}

juce::AudioProcessorEditor* NeuralMidiFXPluginProcessor::createEditor()
{
    auto editor = new NeuralMidiFXPluginEditor(*this);
    /*modelThread.addChangeListener(editor);*/
    return editor;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NeuralMidiFXPluginProcessor();
}

float NeuralMidiFXPluginProcessor::get_playhead_pos()
{
    return playhead_pos;
}


juce::AudioProcessorValueTreeState::ParameterLayout NeuralMidiFXPluginProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    int version_hint = 1;
    // toggles
    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID("OVERDUB", version_hint), "OVERDUB", 0, 1, 1));
    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID("RECORD", version_hint), "RECORD", 0, 1, 1));

    // model selector combobox
    // auto modelfiles = get_pt_files_in_default_path();
    // layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID("MODEL", version_hint), "MODEL", 0, modelfiles.size()-1, 0));

    // buttons
    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID("RESET_GROOVE", version_hint), "RESET_GROOVE", 0, 1, 0));
    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID("RESET_SAMPLINGPARAMS", version_hint), "RESET_SAMPLINGPARAMS", 0, 1, 0));
    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID("RESET_ALL", version_hint), "RESET_ALL", 0, 1, 0));
    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID("RANDOMIZE_VEL", version_hint), "RANDOMIZE_VEL", 0, 1, 0));
    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID("RANDOMIZE_OFFSET", version_hint), "RANDOMIZE_OFFSET", 0, 1, 0));
    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID("RANDOMIZE_ALL", version_hint), "RANDOMIZE_ALL", 0, 1, 0));

    // sliders
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("VEL_DYNAMIC_RANGE", version_hint), "VEL_DYNAMIC_RANGE", -200.0f, 200.0f, 100.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("VEL_BIAS", version_hint), "VEL_BIAS", -1.0f, 1.0f, 0));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("OFFSET_RANGE", version_hint), "OFFSET_RANGE", -200.0f, 200.0f, 100.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("OFFSET_BIAS", version_hint), "OFFSET_BIAS", -1, 1, 0));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("Temperature", version_hint), "Temperature", 0.00001f, 2.0f, 1.0f));

    // these parameters are used with the xySliders for each individual voice
    // Because xySliders are neither slider or button, we
    for (size_t i=0; i < HVO_params::num_voices; i++)
    {
        auto voice_label = nine_voice_kit_labels[i];
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID(voice_label+"_X", version_hint), voice_label+"_X", 0.f, HVO_params::time_steps, nine_voice_kit_default_max_voices_allowed[i])); // max num voices
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID(voice_label+"_Y", version_hint), voice_label+"_Y", 0.f, 1.f, nine_voice_kit_default_sampling_thresholds[i]));                   // threshold level for sampling
    }

    // midi sliders
    for (size_t i=0; i < HVO_params::num_voices; i++)
    {
        auto voice_label = nine_voice_kit_labels[i];
        layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID(voice_label+"_MIDI", version_hint), voice_label+"_MIDI", 0, 127, (int) nine_voice_kit_default_midi_numbers[i])); // drum voice midi numbers
    }

    return layout;
}

void NeuralMidiFXPluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void NeuralMidiFXPluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

