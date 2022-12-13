//
// Created by Behzad Haki on 2022-02-11.
//

#pragma once

#include <torch/script.h> // One-stop header.

// ======================================================================================
// ==================     UI Settings                      ==============================
// ======================================================================================

// GUI settings
namespace UIObjects
{
    namespace Sliders
    {
        // labels
        const std::array<const char*, 4> labels {"Slider 1", "Slider 2", "Slider 3", "Slider 4"};
        // default values
        const std::array<float, 4> default_values {0.0f, 0.0f, 0.0f, 0.0f};

        constexpr int N_per_row {2};    // number of sliders per row
    }

    namespace Rotaries
    {
        const std::array<const char*, 4> labels {"Rotary 1", "Rotary 2", "Rotary 3", "Rotary 4"};
        const std::array<float, 4> default_values {0.0f, 0.0f, 0.0f, 0.0f};

        constexpr int N_per_row {2};    // number of rotaries per row
    }

    namespace Buttons
    {
        const std::array<const char*, 4> labels {"Button 1", "Button 2", "Button 3", "Button 4"};
        constexpr int N_per_row {2};    // number of buttons per row
    }

    namespace Drag_in_regions
    {
        const std::array<const char*, 4> labels {"Drag-in 1", "Drag-in 2", "Drag-in 3", "Drag-in 4"};
        constexpr int N_per_row {2};    // number of buttons per row
    }

    namespace Drag_out_regions
    {
        const std::array<const char*, 4> labels {"Drag-out 1", "Drag-out 2", "Drag-out 3", "Drag-out 4"};
        constexpr int N_per_row {2};    // number of buttons per row
    }
}

namespace GenerationTimeUnit
{

    constexpr bool isSeconds {true};            // if false, time unit is in terms of quarter notes (ppq)
}

namespace GenerationSettingDefaults
{
    constexpr bool should_generate_every_n_notes {false};
    constexpr int n_notes {1};

    constexpr bool should_generate_every_T_seconds {false};
    constexpr float T_seconds {1.0f};

    constexpr bool should_generate_every_P_quarter_notes {false};
    constexpr float P_quarter_notes {0.5f};

    constexpr bool should_generate_once_only {true};   // if true, if there is a backlog of generation requests, only one generation is performed
}












// ======================================================================================
// ==================     Drum Kit Defaults                ==============================
// ======================================================================================

// Voice Mappings corresponding to Kick, Snare, CH, OH, LoT, MidT, HiT, Crash, Ride
#define nine_voice_kit_default_sampling_thresholds std::vector<float>({0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5})
#define nine_voice_kit_default_max_voices_allowed std::vector<float>({32, 32, 32, 32, 32, 32, 32, 32, 32})


// #define nine_voice_kit_default_midi_numbers std::array<int, 9>({36, 38, 42, 46, 43, 47, 48, 50, 51})
#define nine_voice_kit_default_midi_numbers std::array<int, 9>({36, 38, 42, 46, 43, 47, 48, 50, 51})
#define nine_voice_kit_labels std::vector<std::string>({"Kick", "Snare", "Closed Hat", "Open Hat", "Low Tom", "Mid Tom", "Hi Tom", "Crash", "Ride"})
#define monotonic_trigger std::vector<int> ({20, 21, 22})

#define prob_color_hit juce::Colours::lemonchiffon.withAlpha(0.7f)
#define prob_color_non_hit juce::Colours::lemonchiffon.withAlpha(0.3f)
#define hit_color juce::Colours::lemonchiffon.withAlpha(.85f)


#define rest_backg_color juce::Colour::fromFloatRGBA(1.0f,1.0f,1.0f,0.8f)
#define  beat_backg_color juce::Colour::fromFloatRGBA(.8f,.8f,.8f, 0.5f)
#define bar_backg_color juce::Colour::fromFloatRGBA(.4f,.4f,.4f, 0.4f)

#define note_color      juce::Colour::fromRGBA(37, 150, 190, 200)

#define playback_progressbar_color juce::Colours::whitesmoke

// ======================================================================================
// ==================     General Settings                 ==============================
// ======================================================================================
namespace GeneralSettings
{
    // Queue Sizes for communication
    constexpr int gui_io_queue_size { 16 };
    constexpr int processor_io_queue_size { 64 };

    // model_settings
    // char constexpr* default_model_path {(char*)"/Users/behzadhaki/Github/Groove2DrumVST/NeuralMidiFXPlugin/NeuralMidiFXPlugin/Source/model/misunderstood_bush_246-epoch_26_tst.pt"};
    // FIXME add to readme.me for setup ==> model should be placed in root (/Library/NeuralMidiFXPlugin/trained_models) folder
    char constexpr* default_model_path {(char*)"/Library/NeuralMidiFXPlugin/trained_models/model_1.pt"};
    char constexpr* default_model_folder {(char*)"/Library/NeuralMidiFXPlugin/trained_models"};

}

// ======================================================================================
// ==================     HVO Settings                     ==============================
// ======================================================================================
namespace HVO_params
{
    constexpr int time_steps { 32 };
    constexpr int num_voices { 9 };
    constexpr double _16_note_ppq { 0.25 };         // each 16th note is 1/4th of a quarter
    constexpr double _32_note_ppq { 0.125 };        // each 32nd note is 1/8th of a quarter
    constexpr int _n_16_notes { 32 };               // duration of the HVO tensors is 32 16th notes (2 bars of 4-4)
    constexpr int num_steps_per_beat { 4 };
    constexpr int num_beats_per_bar { 4 };

    constexpr double _max_offset { 0.5 };           // offsets are defined in range (-0.5, 0.5)
    constexpr double _min_offset { -0.5 };          // offsets are defined in range (-0.5, 0.5)
    constexpr double _max_vel { 1 };                // velocities are defined in range (0, 1)
    constexpr double _min_vel { 0 };                // velocities are defined in range (0, 1)
}

// ======================================================================================
// ==================     Thread Settings                  ==============================
// ======================================================================================
namespace thread_settings
{
    namespace GrooveThread
    {
        constexpr int waitTimeBtnIters {5}; //ms between two consecutive iterations of the thread loop in run()
    }
    namespace ModelThread
    {
        constexpr int waitTimeBtnIters {5}; //ms between two consecutive iterations of the thread loop in run()
    }
    namespace APVTSMediatorThread
    {
        constexpr int waitTimeBtnIters {20}; //ms between two consecutive iterations of the thread loop in run()
    }
}

// ======================================================================================
// ==================        GUI Settings                  ==============================
// ======================================================================================
namespace gui_settings{
    //
    namespace BasicNoteStructLoggerTextEditor{
        constexpr int maxChars { 500 };
        constexpr int nNotesPerLine { 4 };
    }

    namespace TextMessageLoggerTextEditor{
        constexpr int maxChars { 3000 };
    }

    namespace PianoRolls
    {
        constexpr float label_ratio_of_width {0.1f};        // ratio of label on left side relative to local width
        constexpr float timestep_ratio_of_width {1.0f / (HVO_params::time_steps + 4.0f)};
        constexpr float prob_to_pianoRoll_Ratio {0.4f};
        constexpr float space_reserved_right_side_of_gui_ratio_of_width {0.3f};

        constexpr float total_pianoRoll_ratio_of_width {timestep_ratio_of_width * HVO_params::time_steps};
        constexpr float XYPad_ratio_of_width {1.0f - (label_ratio_of_width + total_pianoRoll_ratio_of_width)};

        constexpr float completePianoRollHeight {0.85f};
        constexpr float completeMonotonicGrooveHeight {0.9f};
    }
}


