#include "Source/DeploymentThreads/PlaybackPreparatorThread.h"

// ===================================================================================
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//                       YOUR IMPLEMENTATION SHOULD BE DONE HERE
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// ===================================================================================

std::pair<bool, bool> PlaybackPreparatorThread::deploy(bool new_model_output_received, bool did_any_gui_params_change) {

    bool newPlaybackPolicyShouldBeSent = false;
    bool newPlaybackSequenceGeneratedAndShouldBeSent = false;

    // =================================================================================
    // ===         1. check if new model output is received
    // =================================================================================
    if (new_model_output_received || did_any_gui_params_change)
    {
        // =================================================================================
        // ===         2. ACCESSING GUI PARAMETERS
        // Refer to:
        // https://neuralmidifx.github.io/datatypes/GuiParams#accessing-the-ui-parameters
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

        PrintMessage("Here1");

        if (new_model_output_received)
        {
            // =================================================================================
            // ===         3. Extract Notes from Model Output
            // =================================================================================
            auto hits = model_output.hits;
            auto velocities = model_output.velocities;
            auto offsets = model_output.offsets;

            // print content of hits
            std::stringstream ss;
            ss << "Hits: ";
            for (int i = 0; i < hits.sizes()[0]; i++)
            {
                for (int j = 0; j < hits.sizes()[1]; j++)
                {
                    for (int k = 0; k < hits.sizes()[2]; k++)
                    {
                        if (hits[i][j][k].item<float>() > 0.5)
                            ss << "(" << i << ", " << j << ", " << k << ") ";
                    }
                }
            }
            PrintMessage(ss.str());

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




    return {newPlaybackPolicyShouldBeSent, newPlaybackSequenceGeneratedAndShouldBeSent};
}