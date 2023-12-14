#include "Source/DeploymentThreads/DeploymentThread.h"

// ===================================================================================
// ===         Refer to:
// https://neuralmidifx.github.io/DeploymentStages/????
// ===================================================================================
std::pair<bool, bool>
    DeploymentThread::deploy(
        std::optional<MidiFileEvent> & new_midi_event_dragdrop,
        std::optional<EventFromHost> & new_event_from_host,
        bool gui_params_changed_since_last_call,
        bool new_preset_loaded_since_last_call,
        bool new_midi_file_dropped_on_visualizers,
        bool new_audio_file_dropped_on_visualizers) {

    // flags to keep track if any new data is generated and should
    // be sent to the main thread
    bool newPlaybackPolicyShouldBeSent{false};
    bool newPlaybackSequenceGeneratedAndShouldBeSent{false};

    // =================================================================================
    // ===         0. LOADING THE MODEL
    // =================================================================================
    // Try loading the model if it hasn't been loaded yet
    if (!isModelLoaded) {
        load("drumLoopVAE.pt");
    }

    // =================================================================================
    // ===         1. ACCESSING GUI PARAMETERS
    // Refer to:
    //
    // =================================================================================
    auto ButtonTrigger = gui_params.wasButtonClicked("Randomize");

    if (ButtonTrigger) {
        if (isModelLoaded)
        {
            cout << "Generating new sequence..." << endl;
            // Generate a random latent vector
            auto latentVector = torch::randn({ 1, 128});

            // Prepare other inputs
            auto voice_thresholds = torch::ones({9 }, torch::kFloat32) * 0.5f;
            auto max_counts_allowed = torch::ones({9 }, torch::kFloat32) * 32;
            int sampling_mode = 0;
            float temperature = 1.0f;

            // Prepare above for inference
            std::vector<torch::jit::IValue> inputs;
            inputs.emplace_back(latentVector);
            inputs.emplace_back(voice_thresholds);
            inputs.emplace_back(max_counts_allowed);
            inputs.emplace_back(sampling_mode);
            inputs.emplace_back(temperature);

            // Get the scripted method
            auto sample_method = model.get_method("sample");

            // Run inference
            auto output = sample_method(inputs);

            // Extract the generated tensors from the output
            auto hits = output.toTuple()->elements()[0].toTensor();
            auto velocities = output.toTuple()->elements()[1].toTensor();
            auto offsets = output.toTuple()->elements()[2].toTensor();

            // =================================================================================
            // ===         2. ACCESSING GUI PARAMETERS
            // Refer to:
            // https://neuralmidifx.github.io/docs/v2_0_0/datatypes/GuiParams
            // =================================================================================
            std::map<int, int> voiceMap;
            voiceMap[0] = int(gui_params.getValueFor("Kick"));
            voiceMap[1] = int(gui_params.getValueFor("Snare"));
            voiceMap[2] = int(gui_params.getValueFor("ClosedHat"));
            voiceMap[3] = int(gui_params.getValueFor("OpenHat"));
            voiceMap[4] = int(gui_params.getValueFor("LowTom"));
            voiceMap[5] = int(gui_params.getValueFor("MidTom"));
            voiceMap[6] = int(gui_params.getValueFor("HighTom"));
            voiceMap[7] = int(gui_params.getValueFor("Crash"));
            voiceMap[8] = int(gui_params.getValueFor("Ride"));


            // =================================================================================
            // ===         3. Extract Generations into a PlaybackPolicy and PlaybackSequence
            // Refer to:
            // https://neuralmidifx.github.io/docs/v2_0_0/datatypes/PlaybackPolicy
            // https://neuralmidifx.github.io/docs/v2_0_0/datatypes/PlaybackSequence
            // =================================================================================
            if (!hits.sizes().empty()) // check if any hits are available
            {
                // clear playback sequence
                playbackSequence.clear();

                // set the flag to notify new playback sequence is generated
                newPlaybackSequenceGeneratedAndShouldBeSent = true;

                // iterate through all voices, and time steps
                int batch_ix = 0;
                for (int step_ix = 0; step_ix < 32; step_ix++)
                {
                    for (int voice_ix = 0; voice_ix < 9; voice_ix++)
                    {

                        // check if the voice is active at this time step
                        if (hits[batch_ix][step_ix][voice_ix].item<float>() > 0.5)
                        {
                            auto midi_num = voiceMap[voice_ix];
                            auto velocity = velocities[batch_ix][step_ix][voice_ix].item<float>();
                            auto offset = offsets[batch_ix][step_ix][voice_ix].item<float>();
                            // we are going to convert the onset time to a ratio of quarter notes
                            auto time = (step_ix + offset) * 0.25f;

                            playbackSequence.addNoteWithDuration(
                                0, midi_num, velocity, time, 0.1f);

                        }
                    }
                }
            }

            // Specify the playback policy
            playbackPolicy.SetPlaybackPolicy_RelativeToAbsoluteZero();
            playbackPolicy.SetTimeUnitIsPPQ();
            playbackPolicy.SetOverwritePolicy_DeleteAllEventsInPreviousStreamAndUseNewStream(true);
            playbackPolicy.ActivateLooping(8);
            newPlaybackPolicyShouldBeSent = true;
        }
    }

    // your implementation goes here
    return {newPlaybackPolicyShouldBeSent, newPlaybackSequenceGeneratedAndShouldBeSent};
}
