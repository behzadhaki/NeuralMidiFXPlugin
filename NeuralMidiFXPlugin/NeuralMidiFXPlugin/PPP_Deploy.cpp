#include "Source/DeploymentThreads/PlaybackPreparatorThread.h"

// ===================================================================================
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//                       YOUR IMPLEMENTATION SHOULD BE DONE HERE
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// ===================================================================================

std::pair<bool, bool> PlaybackPreparatorThread::deploy(bool new_model_output_received, bool did_any_gui_params_change) {

    bool newPlaybackPolicyShouldBeSent = false;
    bool newPlaybackSequenceGeneratedAndShouldBeSent = false;

    // ---------------------------------------------------------------------------
    // Call Section (i.e. user is playing)
    // ---------------------------------------------------------------------------
    if (new_model_output_received)
    {
        // check if any host data is received from model thread via model_output
        auto new_event_from_host = model_output.new_event_from_host;
        if (new_event_from_host.has_value()) {

            // store the bar locations
            if (new_event_from_host->isNewBarEvent()) {
                PPPdata.bar_locations.emplace_back(new_event_from_host->Time().inQuarterNotes());
            }

            // keep track of played notes
            if (new_event_from_host->isNoteOnEvent()) {

                // if user is playing. we are in Call section
                PPPdata.UserPerformanceIsBeingRecorded_FLAG = true;

                // Clear playing generations if any
                playbackSequence.clear();
                newPlaybackSequenceGeneratedAndShouldBeSent = true;
                newPlaybackPolicyShouldBeSent = true;

                // store note
                PPPdata.note_on_events_since_call_section_started.emplace_back(*new_event_from_host);

                // store time of last note
                PPPdata.last_note_time = new_event_from_host->Time().inSeconds();

            }

            // check if two seconds elapsed since last note
            if (new_event_from_host->Time().inSeconds() - PPPdata.last_note_time > 2.0) {

                // if user is not playing. we are in Response section
                // however check if any notes were played, otherwise keep waiting
                if (!PPPdata.note_on_events_since_call_section_started.empty()) {

                    PPPdata.UserPerformanceIsBeingRecorded_FLAG = false;

                }
            }
        }
    }

    // -----------------------------------------------------------------------
    // Response Section (i.e. user is not playing)
    // -----------------------------------------------------------------------

    // check if we should respond to the user
    if (!PPPdata.UserPerformanceIsBeingRecorded_FLAG) {
        PrintMessage(" Response Section");
        // Find the location of the first bar in the call section
        auto first_note = PPPdata.note_on_events_since_call_section_started.front();
        auto fnote_time = first_note.Time().inQuarterNotes();
        auto closest_bar_location = *std::min_element(
            PPPdata.bar_locations.begin(), PPPdata.bar_locations.end(),
            [fnote_time](double a, double b) {
                return std::abs(a - fnote_time) < std::abs(b - fnote_time);
            });

        // if the closest bar location is after the first note, move it back one bar (four quarter notes)
        if (closest_bar_location > fnote_time) {
            closest_bar_location -= 4.0;
        }

        // -----------------------------------------------------------------------
        // Process Notes for Inference
        // (MODEL SPECIFIC - Don't worry about the details if you are not
        // familiar with the model)
        // -----------------------------------------------------------------------

        // create input to the model
        torch::Tensor groove = torch::zeros({1, 32, 27}, torch::kFloat32);

        for (auto &note_event: PPPdata.note_on_events_since_call_section_started) {
            auto ppq = note_event.Time().inQuarterNotes() - closest_bar_location;
            auto velocity = note_event.getVelocity(); // velocity
            auto div = round(ppq / .25f);
            auto offset = (ppq - (div * .25f)) / 0.125 * 0.5 ;
            auto grid_index = (long long) fmod(div, 32);

            // check if louder if overlapping
            if (groove[0][grid_index][2].item<float>() > 0) {
                if (groove[0][grid_index][11].item<float>() < velocity) {
                    groove[0][grid_index][11] = velocity;
                    groove[0][grid_index][20] = offset;
                }
            } else {
                groove[0][grid_index][2] = 1;
                groove[0][grid_index][11] = velocity;
                groove[0][grid_index][20] = offset;
            }
        }

        DisplayTensor(groove, "Groove");

        // preparing the input to encode() method
        std::vector<torch::jit::IValue> enc_inputs;
        enc_inputs.emplace_back(groove);
        enc_inputs.emplace_back(torch::tensor(0.9, torch::kFloat32).unsqueeze_(0));

        // get the encode method
        auto encode = PPPdata.model.get_method("encode");

        // encode the input
        auto encoder_output = encode(enc_inputs);

        // get latent vector from encoder output
        auto latent_vector = encoder_output.toTuple()->elements()[2].toTensor();

        //
        std::vector<torch::jit::IValue> inputs;
        inputs.emplace_back(latent_vector);
        inputs.emplace_back(torch::ones({9 }, torch::kFloat32) * 0.5f);
        inputs.emplace_back(torch::ones({9 }, torch::kFloat32) * 32);
        inputs.emplace_back(0);
        inputs.emplace_back(1.0f);
        auto sample_method = PPPdata.model.get_method("sample");
        auto output = sample_method(inputs);
        // Extract the generated tensors from the output
        auto hits = output.toTuple()->elements()[0].toTensor();
        auto velocities = output.toTuple()->elements()[1].toTensor();
        auto offsets = output.toTuple()->elements()[2].toTensor();

        // -----------------------------------------------------------------------
        // Add Notes to Playback Sequence
        // -----------------------------------------------------------------------
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

        // clear the playback sequence
        playbackSequence.clear();

        // add notes to playback sequence
        // iterate through all voices, and time steps
        int batch_ix = 0;

        // get Upcoming bar location
        // find closest higher multiple of 8 to time_now
        auto time_now = realtimePlaybackInfo->get().time_in_ppq;
        auto upcoming_bar_location = ceil(time_now / 4.0) * 4.0;

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
                    auto time = (step_ix + offset) * 0.25f + upcoming_bar_location;

                    playbackSequence.addNoteWithDuration(
                        0, midi_num, velocity, time, 0.25f);
                    newPlaybackSequenceGeneratedAndShouldBeSent = true;
                }
            }
        }
        // Specify the playback policy
        playbackPolicy.SetPlaybackPolicy_RelativeToAbsoluteZero();
        playbackPolicy.SetTimeUnitIsPPQ();
        playbackPolicy.SetOverwritePolicy_DeleteAllEventsInPreviousStreamAndUseNewStream(true);
        playbackPolicy.DisableLooping();
        newPlaybackPolicyShouldBeSent = true;

        // Prepare for Call Section Again
        PPPdata.note_on_events_since_call_section_started.clear();
        PPPdata.UserPerformanceIsBeingRecorded_FLAG = true;
    }

    return {newPlaybackPolicyShouldBeSent, newPlaybackSequenceGeneratedAndShouldBeSent};
}