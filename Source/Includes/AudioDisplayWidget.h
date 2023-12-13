//
// Created by Behzad Haki on 8/3/2023.
//

#pragma once
#include <utility>

#include "GuiParameters.h"
#include "Configs_Parser.h"

using namespace std;

// the widget for drawing the wav form
class AudioWaveformWidget : public juce::Component,
                            public juce::FileDragAndDropTarget {
public:
    bool AllowToDragInAudio{true};
    bool AllowToDragOutAsAudio{true};
    juce::String label{"Audio Display"};
    juce::Colour backgroundColour{juce::Colours::whitesmoke};

    AudioWaveformWidget() {
        setWantsKeyboardFocus(true);
    }

    void mouseDrag(const juce::MouseEvent& ) override
    {
        if (AllowToDragOutAsAudio)
        {
            // Save the audio file to a temporary file
            juce::String fileName = juce::Uuid().toString() + ".wav";  // Random filename

            juce::File outf(juce::File::getSpecialLocation(
                                juce::File::SpecialLocationType::tempDirectory
                                ).getChildFile(fileName));

            juce::TemporaryFile tempf(outf);
            {
                auto p_os = std::make_unique<juce::FileOutputStream>(tempf.getFile());
                if (p_os->openedOk())
                {
                    juce::WavAudioFormat wavFormat;
                    std::unique_ptr<juce::AudioFormatWriter> writer;
                    writer.reset(wavFormat.createWriterFor(p_os.get(), sampleRate, waveform_buffer.getNumChannels(), 16, {}, 0));

                    if (writer != nullptr)
                    {
                        bool writeSuccess = writer->writeFromAudioSampleBuffer(waveform_buffer, 0, waveform_buffer.getNumSamples());
                        if (!writeSuccess)
                        {
                            // Handle the error: Unable to write the buffer to the file
                            return;
                        }
                    }
                    else
                    {
                        // Handle the error: Unable to create the WAV writer
                        return;
                    }

                    // The AudioFormatWriter is now responsible for deleting the stream
                    p_os.release();
                }
                else
                {
                    // Handle the error: Unable to open the file output stream
                    return;
                }
            }

            bool succeeded = tempf.overwriteTargetFileWithTemporary();
            if (succeeded)
            {
                juce::DragAndDropContainer::performExternalDragDropOfFiles(
                    {outf.getFullPathName()}, false);
            }
        }
    }

    bool isInterestedInFileDrag (const juce::StringArray& /*files*/) override
    {
        // Check if the files are of a type you're interested in
        // Return true to indicate interest
        return true;
    }

    void filesDropped (const juce::StringArray& files, int /*x*/, int /*y*/) override
    {
        if (!AllowToDragInAudio) { return; }

        auto file = juce::File(files[0]);

        if (file.exists())
        {
            // load the audio file
            loadAudioFile(file);
        }

    }

    void fileDragEnter (const juce::StringArray& /*files*/, int /*x*/, int /*y*/) override {

    }
    
    void setAudioData(CrossThreadAudioVisualizerData* audioData_) {
        crossThreadAudioVisualizerData = audioData_;
    }

    bool loadAudioFile(const juce::File& file) {
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats(); // Register all the basic formats (wav, aiff, etc.)

        if (!file.existsAsFile()) {
            // File does not exist
            return false;
        }

        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
        if (reader == nullptr) {
            // The file is not an audio file
            return false;
        }

        // Now we have a valid audio file, so let's prepare the buffer
        waveform_buffer.clear();
        waveform_buffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
        reader->read(&waveform_buffer, 0, (int)reader->lengthInSamples, 0, true, true);
        sampleRate = reader->sampleRate;

        // place the content in crossThreadAudioVisualizerData so that
        // the DPL thread can access it
        if (crossThreadAudioVisualizerData != nullptr) {
            crossThreadAudioVisualizerData->setAudioBuffer(waveform_buffer, sampleRate, true);
        }

        return true;
    }

    void update_waveform_buffer(juce::AudioBuffer<float> waveform_buffer_, double sampleRate_) {
        waveform_buffer = std::move(waveform_buffer_);
        sampleRate = sampleRate_;
        repaint();
    }

    double getSequenceDuration() {
        return waveform_buffer.getNumSamples();
    }

private:
    // keep waveform_buffer data
    juce::AudioBuffer<float> waveform_buffer;
    double sampleRate{44100};
    CrossThreadAudioVisualizerData* crossThreadAudioVisualizerData{nullptr};
    juce::String currentAudioFilePath; // Store the current audio file path here

    // paints the waveform_buffer
    void paint(juce::Graphics& g) override {
        // ---------------------
        // Draw background (12 notes
        // ---------------------
        g.fillAll(backgroundColour);

        // draw the waveform_buffer
        g.setColour(juce::Colours::black);
        juce::Path waveformPath;

        auto numSamples = (float) waveform_buffer.getNumSamples();

        // for long audio files, only draw every 100th sample
        auto sample_draw_interval = 10;
        if (numSamples > 100000) {
            sample_draw_interval = 100;
        }

        // add every 100th sample to the path
        for (int i = 0; i < waveform_buffer.getNumSamples(); i += sample_draw_interval) {
            float x = (float)i / numSamples * (float)getWidth();
            auto y = float((1.0f + waveform_buffer.getSample(0, i)) / 2.0 * (float)getHeight());
            if (i == 0) {
                waveformPath.startNewSubPath(x, y);
            } else {
                waveformPath.lineTo(x, y);
            }
        }

        g.strokePath(waveformPath, juce::PathStrokeType(0.24f));

    }

};

class AudioPlayheadVisualizer : public juce::Component {
public:
    bool show_playhead{true};
    juce::Colour playheadColour = juce::Colours::red;
    float playhead_pos{0};
    float disp_length{8};
    float LoopStart{-1};
    float LoopDuration {-1};

        void paint(juce::Graphics& g) override
        {
            if (show_playhead) {
                g.setColour(playheadColour);
                // Draw playhead
                if (playhead_pos >= 0)
                {
                    auto x = playhead_pos / disp_length * (float)getWidth();
                    g.drawLine(
                        x, 0, x, (float) getHeight(), 2);
                }
            }
        }

        void resized() override
        {
            repaint();
        }

        // playhead pos is in samples only
        void setPlayheadPos(float pos, float length) {
            if (pos != playhead_pos) {
                if (length != disp_length) {
                    disp_length = length;
                }
                if (LoopStart >= 0 && LoopDuration > 0) {
                    pos = std::fmod(pos, LoopDuration);
                }

                playhead_pos = pos;
                repaint();
            }
        }

        void enableLooping(float start_sample, float duration_samples) {
            LoopStart = start_sample;
            LoopDuration = duration_samples;
        }
};


class AudioVisualizer: public juce::Component, juce::Timer {
public:
    AudioVisualizer(bool needsPlayhead_,
                    string paramID_) {
        setInterceptsMouseClicks(false, true);
        paramID = std::move(paramID_);
        std::transform(paramID.begin(), paramID.end(), paramID.begin(), ::toupper);
        needsPlayhead = needsPlayhead_;
        addAndMakeVisible(audioWaveformWidget);
        addAndMakeVisible(audioPlayheadVisualizer);

        // start timer
        startTimerHz(10);
    }

    void resized() override {
        // 10% of the height for the playhead
        // 90% of the height for the piano roll
        auto area = getLocalBounds();

       if (needsPlayhead) {
            auto phead_height = area.getHeight() * 0.1;
            audioWaveformWidget.setBounds(
                area.removeFromTop(int(area.getHeight() * 0.9)));
            audioPlayheadVisualizer.setBounds(
                area.removeFromTop((int) phead_height));
       } else {
            audioWaveformWidget.setBounds(area);
       }

    }

    void paint(juce::Graphics& g) override {
       g.fillAll(juce::Colours::black);
    }

    void enableDragInAudio(bool enable) {
        audioWaveformWidget.AllowToDragInAudio = enable;
    }

    void enableDragOutAsAudio(bool enable) {
        audioWaveformWidget.AllowToDragOutAsAudio = enable;
    }

    void enableLooping(float start_sample, float duration_samples) {
        audioPlayheadVisualizer.enableLooping(start_sample, duration_samples);
    }

    void setAudioData(CrossThreadAudioVisualizerData* audioData_) {
        audioWaveformWidget.setAudioData(audioData_);
        crossThreadAudioVisualizerData = audioData_;
    }

    // timer callback
    void timerCallback() override
    {
        if (crossThreadAudioVisualizerData != nullptr)
        {
            if (crossThreadAudioVisualizerData->shouldRepaint()
                && !crossThreadAudioVisualizerData->didUserDroppedNewAudio()) {
                    cout << "accessing deployer thread data" << endl;
                    auto [waveform_buffer, sampleRate] =
                        crossThreadAudioVisualizerData->getAudioBuffer();
                    audioWaveformWidget.update_waveform_buffer(waveform_buffer,
                                                               sampleRate);
                    resized();
            }
        }

    }

    std::string getParamID()
    {
        return paramID;
    }

    void updatePlayhead(double playhead_pos) {
        audioPlayheadVisualizer.setPlayheadPos(
            (float)playhead_pos, (float)audioWaveformWidget.getSequenceDuration());
    }

private:
    AudioWaveformWidget audioWaveformWidget;
    AudioPlayheadVisualizer audioPlayheadVisualizer;

    // all visualizers' data will be stored here.
    // thread safe access/updates via editor and/or DPL thread
    CrossThreadAudioVisualizerData* crossThreadAudioVisualizerData{nullptr};
    bool needsPlayhead{false};
    std::string paramID;
};