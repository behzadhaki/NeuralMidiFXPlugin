//
// Created by Behzad Haki on 2022-08-20.
//

#pragma once

#include <shared_plugin_helpers/shared_plugin_helpers.h>
#include "../InterThreadFifos.h"


// ------------------------------------------------------------------------------------------------------------
// ==========              Building Blocks of each of the time steps in the pianorolls            =============
// ========== https://forum.juce.com/t/how-to-draw-a-vertical-and-horizontal-line-of-the-mouse-position/31115/2
// ==========
// ============================================================================================================
class ButtonWithAttachment: public juce::Component
{
public:
    juce::TextButton button;
    unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> ButtonAPVTSAttacher;
    juce::AudioProcessorValueTreeState* apvts;
    string PrameterID;

    ButtonWithAttachment(juce::AudioProcessorValueTreeState* apvtsPntr, const string& Text2show, string ParameterID_)
    {
        apvts = apvtsPntr;
        PrameterID = ParameterID_;
        ButtonAPVTSAttacher = make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (*apvts, PrameterID, button);
        button.setButtonText(Text2show);
        button.setClickingTogglesState(true);
        addAndMakeVisible(button);
    }

    void resized() override
    {
        button.setBounds(getLocalBounds());
    }
};

namespace SingleStepPianoRollBlock
{

    /***
     * (A) The smallest block to draw a drum event inside
     * If interactive, notes can be created by double clicking, moved around by dragging ...
     * Notes are drawn using offset values and velocity given a offset range (specified in settings.h)
     * Velocity range is from 0 to 1
     *
     * (B) if a queue is provided (of type LockFreeQueue<BasicNote, GeneralSettings::gui_io_queue_size>),
     * moving a note manually sends the new vel, hit, offset values to a receiving thread
     * (in this case, GrooveThread in processor)
     *
     * (C) InteractiveInstance is used for Monotonic Groove Visualization only as generated drums are not
     * manually modifiable
     */
    class InteractiveIndividualBlock : public juce::Component
    {
    public:
        bool isClickable;
        int hit;
        float velocity;
        float offset;
        array<juce::Colour, 2> backgroundcolor_array;
        int grid_index; // time step corresponding to the block
        float low_offset = min(HVO_params::_min_offset, HVO_params::_max_offset); // min func just incase min and max offsets are wrongly defined
        float hi_offset = max(HVO_params::_min_offset, HVO_params::_max_offset); // max func just incase min and max offsets are wrongly defined
        float range_offset = hi_offset - low_offset;
        LockFreeQueue<BasicNote, GeneralSettings::gui_io_queue_size>* GroovePianoRollWidget2GrooveThread_manually_drawn_noteQue;

        /**
         * Constructor
         *
         * @param isClickable_ (bool) True for interactive version
         * @param backgroundcolor_array  (array<juce::Colour, 2>) backgroundcolor_array[0] color for left half, backgroundcolor_array[1] color for right half,
         * @param grid_index_ (int) specifies which time step the block is used for
         * @param GroovePianoRollWidget2GrooveThread_manually_drawn_noteQuePntr  (LockFreeQueue<BasicNote, GeneralSettings::gui_io_queue_size>*) used to send data to a receiver via this queue if interactive and also queue is not nullptr
         *
         */
        InteractiveIndividualBlock(bool isClickable_, array<juce::Colour, 2> backgroundcolor_array_, int grid_index_,
                                   LockFreeQueue<BasicNote, GeneralSettings::gui_io_queue_size>* GroovePianoRollWidget2GrooveThread_manually_drawn_noteQuePntr = nullptr) {
            grid_index = grid_index_;
            backgroundcolor_array = backgroundcolor_array_;
            isClickable = isClickable_;
            GroovePianoRollWidget2GrooveThread_manually_drawn_noteQue = GroovePianoRollWidget2GrooveThread_manually_drawn_noteQuePntr;
            hit = 0;
            velocity = 0;
            offset = 0;
        }

        void paint(juce::Graphics& g) override
        {


            // get component dimensions
            auto w = (float) getWidth();
            auto h = (float) getHeight();

            // draw a line on the left side
            g.setColour(juce::Colours::black);
            auto x_ = (float) proportionOfWidth(0.5f);

            //background colours
            // g.fillAll(backgroundcolor_array[0]);
            g.setColour(backgroundcolor_array[0]);
            juce::Rectangle<float> rectLeftBackg (juce::Point<float> {0, 0}, juce::Point<float>{x_, (float)getHeight()});
            g.fillRect(rectLeftBackg);
            g.drawRect(rectLeftBackg, 0.0f);
            g.setColour(backgroundcolor_array[1]);
            juce::Rectangle<float> rectRightBackg (juce::Point<float> {x_, 0}, juce::Point<float>{(float)getWidth(), (float)getHeight()});
            g.fillRect(rectRightBackg);
            g.drawRect(rectRightBackg, 0.0f);

            // lines
            // draw a line on the left edge of each step
            g.setColour(juce::Colours::whitesmoke);
            juce::Point<float> p1_edge {x_, h};
            juce::Point<float> p2_edge {x_, 0};
            g.drawLine ({p1_edge, p2_edge}, 1.0f);

            /*juce::Point<float> p1_edgeL {0, h};
            juce::Point<float> p2_edgeL {0, 0};
            g.drawLine ({p1_edgeL, p2_edgeL}, 0.50f);*/

            if (hit == 1)
            {
                // draw a colored line and a rectangle on the top end for the note hit
                g.setColour(note_color);

                auto x_pos = offsetToActualX();
                auto y_pos = (float)proportionOfHeight(1-velocity);
                juce::Point<float> p1 {x_pos, h};
                juce::Point<float> p2 {x_pos, y_pos};
                auto thickness = 2.0f;
                g.drawLine ({p1, p2}, thickness);

                juce::Rectangle<float> rect {p2 , juce::Point<float> {x_pos + w * 0.3f, y_pos + w * 0.3f}};

                g.fillRect(rect);
                g.drawRect(x_pos, y_pos , w * 0.3f, w * 0.3f, thickness );
                // g.drawLine(x_pos, getHeight(),x_pos, y_pos);
            }

        }

        // only sendDataToQueue() if instance is interactive
        // only sends data when mouse key is released
        void mouseUp(const juce::MouseEvent& ) override
        {
            if (isClickable)
            {
                sendDataToQueue();
            }
        }

        // moves a note around the block on dragging and repaints
        void mouseDrag(const juce::MouseEvent& ev) override
        {
            if (isClickable)
            {
                if (hit == 1)
                {
                    // if shift pressed moves only up or down
                    if (!ev.mods.isShiftDown())
                    {
                        offset = actualXToOffset(ev.position.getX());
                        offset = fmax(fmin(offset, float(HVO_params::_max_offset)), float(HVO_params::_min_offset));
                    }
                    velocity = 1.0f - float(ev.position.getY()) / (float) getHeight();
                    velocity = fmax(fmin(velocity, float(HVO_params::_max_vel)), float(HVO_params::_min_vel));
                }
                repaint();
            }
        }

        // hides an already existing note or unhides if the block is empty
        void mouseDoubleClick(const juce::MouseEvent& ev) override
        {
            if (isClickable)
            {
                if (hit == 0) // convert to hit
                {
                    if (velocity == 0) // if no previous note here! make a new one with offset 0
                    {
                        hit = 1;
                        velocity = 1.0f - ev.position.getY()/float(getHeight());
                        offset = 0;
                    }
                    else            // if note already here, double click activates it again
                    {
                        hit = 1;
                    }
                }
                else  // convert to silence without deleting velocity/offset info
                {
                    hit = 0;
                }
                sendDataToQueue();
                repaint();
            }
        }

        // places a note in the block at the given locations
        void addEvent(int hit_, float velocity_, float offset_)
        {
            assert (hit == 0 or hit == 1);

            hit = hit_;
            velocity = velocity_;
            offset = offset_;

            repaint();
        }

        // places a BasicNote in the queue to be received by another thread
        void sendDataToQueue() const
        {
            if (hit == 1)
            {
                GroovePianoRollWidget2GrooveThread_manually_drawn_noteQue->push(BasicNote(100, velocity, grid_index, offset));
            }
            else
            {
                GroovePianoRollWidget2GrooveThread_manually_drawn_noteQue->push(BasicNote(100, 0, grid_index, 0));
            }
        }

        // converts offset to actual x value on component
        float offsetToActualX()
        {
            float ratioOfWidth = (offset - low_offset) / range_offset;
            return (float)proportionOfWidth(ratioOfWidth);
        }

        // converts offset to actual x value on component
        float actualXToOffset(float x_pixel)
        {
            float ratioOfWidth = x_pixel / (float)getWidth();
            return (ratioOfWidth * range_offset + low_offset);
        }
    };


    /**
     *  The block in which probability levels are shown
     *  if probabilities are over 0, a curved peak is drawn at the centre
     *  with height corresponding to the specified probability level
     */
    class ProbabilityLevelWidget: public juce::Component
    {
    public:
        juce::Colour backgroundcolor;
        float hit_prob = 0;
        float sampling_threshold = 0;
        int hit = 0;

        /***
         * Constructor
         * @param backgroundcolor_ (juce:: Colour)

         */
        ProbabilityLevelWidget(juce:: Colour backgroundcolor_)
        {
            backgroundcolor = backgroundcolor_;
        }

        // draws a path peaking at the probability level
        void paint(juce::Graphics& g) override
        {
            //background colour
            g.fillAll(backgroundcolor);

            auto h = (float) getHeight();
            auto w = (float) getWidth();

            g.setColour(juce::Colours::white);
            auto y_= (1 - sampling_threshold) * h;
            g.drawLine(-1.0f, y_, h*2.0f, y_);

            juce::Path myPath1;
            float from_edge = 0.4f;
            auto p = (float) proportionOfHeight(1.0f - hit_prob); // location of peak (ie probability)
            auto control_rect = juce::Rectangle<float> (juce::Point<float> ((float) proportionOfWidth(from_edge), h), juce::Point<float> ((float)proportionOfWidth(1.0f - from_edge), p));

            auto half_P = (float) proportionOfHeight(1.0f - hit_prob/2.0f);
            g.setColour(prob_color_non_hit);
            myPath1.startNewSubPath (0.0f, h);

            myPath1.lineTo(juce::Point<float> (w, h));
            g.strokePath (myPath1, juce::PathStrokeType (2.0f));

            // draw a quadratic curve if probability is over 0
            if (hit_prob > 0)
            {
                juce::Path myPath;
                myPath.startNewSubPath (0.0f, h);

                auto fillType = juce::FillType();
                fillType.setColour(prob_color_non_hit);

                if (hit == 1 )
                {
                    g.setColour(prob_color_hit);
                    fillType.setColour(prob_color_hit);
                }

                myPath.quadraticTo(control_rect.getBottomLeft(), juce::Point<float> ((float)proportionOfWidth(from_edge), half_P));
                myPath.quadraticTo(control_rect.getTopLeft(), juce::Point<float> ((float)proportionOfWidth(0.5f), p));
                myPath.quadraticTo(control_rect.getTopRight(), juce::Point<float> ((float)proportionOfWidth(1.0f - from_edge), half_P));
                myPath.quadraticTo(control_rect.getBottomRight(), juce::Point<float> (w, h));
                myPath.closeSubPath();

                g.setFillType(fillType);
                g.fillPath(myPath);
            }
        }

        /** Specify probability state,
         * i.e. what the probability level and sampling thresholds are
         * and also whether the note will actually get played (if the note is
         * one of the n_max most probable events with prob over threshold)
         * @param hit_ (int) if 1, then the color for peak will be darker
         * @param hit_prob_  (float) probability level
         * @param sampling_thresh (float) line level used for sampling threshold
         */
        void setProbability(int hit_, float hit_prob_, float sampling_thresh)
        {
            hit = hit_;
            hit_prob = hit_prob_;
            sampling_threshold = sampling_thresh;
            repaint();
        }

        void setSamplingThreshold(float thresh)
        {
            sampling_threshold = thresh;
            repaint();
        }


    };


    /**
     * Wraps a PianoRoll_InteractiveIndividualBlock and ProbabilityLevelWidget together into one component
     */
    class InteractiveIndividualBlockWithProbability : public juce::Component
    {

    public:
        unique_ptr<InteractiveIndividualBlock> pianoRollBlockWidgetPntr;  // unique_ptr to allow for initialization in the constructor
        unique_ptr<ProbabilityLevelWidget> probabilityCurveWidgetPntr;         // component instance within which we'll draw the probability curve

        InteractiveIndividualBlockWithProbability(bool isClickable_, array<juce::Colour, 2> backgroundcolor_array, int grid_index_,
                                                  LockFreeQueue<BasicNote, GeneralSettings::gui_io_queue_size>* GroovePianoRollWidget2GrooveThread_manually_drawn_noteQues=nullptr)
        {
            pianoRollBlockWidgetPntr = make_unique<InteractiveIndividualBlock>(isClickable_, backgroundcolor_array, grid_index_, GroovePianoRollWidget2GrooveThread_manually_drawn_noteQues);
            addAndMakeVisible(pianoRollBlockWidgetPntr.get());


            probabilityCurveWidgetPntr = make_unique<ProbabilityLevelWidget>(juce::Colours::black);
            addAndMakeVisible(probabilityCurveWidgetPntr.get());
        }

        void addEvent(int hit_, float velocity_, float offset, float hit_prob_, float sampling_threshold) const
        {
            if (pianoRollBlockWidgetPntr->hit != hit_ or pianoRollBlockWidgetPntr->velocity != velocity_ or
                pianoRollBlockWidgetPntr->offset != offset)
            {
                pianoRollBlockWidgetPntr->addEvent(hit_, velocity_, offset);
            }

            probabilityCurveWidgetPntr->setProbability(hit_, hit_prob_, sampling_threshold);

        }

        void resized() override
        {
            auto area = getLocalBounds();
            auto prob_to_pianoRoll_Ratio = 0.4f;
            auto h = (float) area.getHeight();
            pianoRollBlockWidgetPntr->setBounds (area.removeFromTop(int((1-prob_to_pianoRoll_Ratio)*h)));
            probabilityCurveWidgetPntr->setBounds (area.removeFromBottom(int(h*prob_to_pianoRoll_Ratio)));
        }
    };


    /**
     * An XYPad which is infact connected to two sliders.
     * Can automatically connect to APVTS and also update the y level in probability widgets
     */
    class XYPadAutomatableWithSliders: public juce::Component, public juce::Slider::Listener
    {
    public:
        vector<InteractiveIndividualBlockWithProbability*> ListenerWidgets;

        juce::Slider xSlider;
        unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> xSliderAttachement;
        juce::Slider ySlider;
        unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ySliderAttachement;
        juce::AudioProcessorValueTreeState* apvts;

        float latest_x = 0;
        float latest_y = 0;

        XYPadAutomatableWithSliders(juce::AudioProcessorValueTreeState* apvtsPntr,
                                    const juce::String xParameterID , const juce::String yParameterID)
        {
            apvts = apvtsPntr;
            xSliderAttachement = make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(*apvts, xParameterID, xSlider);
            ySliderAttachement = make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(*apvts, yParameterID, ySlider);
        }

        void sliderValueChanged (juce::Slider* slider) override
        {
            if (slider == &xSlider or slider == &ySlider)
            {
                repaint();
            }
        }

        void paint(juce::Graphics& g) override
        {
            g.fillAll(juce::Colours::black);
            g.fillAll(juce::Colours::black);
            g.setColour(juce::Colours::white);

            auto w = float(getWidth());
            auto h = float(getHeight());

            auto x_ = round(max(min(float(xSlider.getValue() - xSlider.getMinimum()) * w / float(xSlider.getMaximum() - xSlider.getMinimum()), w), 0.0f));
            auto y_ = max(min(float(1.0f - (ySlider.getValue() - ySlider.getMinimum()) / float(ySlider.getMaximum() - ySlider.getMinimum())) * h, h), 0.0f);


            // g.setColour(juce::Colours::beige);
            g.setColour(prob_color_hit);
            juce::Point<float> p1 {0, 0};
            juce::Point<float> p2 {x_, y_};
            juce::Rectangle<float> rect {p1, p2};
            g.fillRect(rect);
            g.drawRect(rect, 0.2f);
            g.setColour(juce::Colours::white);
            g.drawLine(0.0f, y_, x_, y_, 2);
            g.drawLine(x_, 0, x_, y_, 2);

        }

        void mouseDoubleClick(const juce::MouseEvent&) override
        {
            // more than 2 clicks goes back to default
            if (xSlider.getValue() == 0 and ySlider.getValue() == 1)
            {
                xSlider.setValue(latest_x);
                ySlider.setValue(latest_y);
            }
            else // regular double click sets area to 0 --> no possible hits
            {
                latest_x = (float) xSlider.getValue();
                latest_y = (float) ySlider.getValue();
                xSlider.setValue(0.0f);
                ySlider.setValue(1.0f);
            }
            repaint();
            BroadCastThresholds();
        }


        void mouseDrag(const juce::MouseEvent& ev) override
        {
            m_mousepos = ev.position;
            xSlider.setValue(m_mousepos.getX()/float(getWidth()) * (xSlider.getMaximum() - xSlider.getMinimum())+ xSlider.getMinimum());
            ySlider.setValue((1.0f - m_mousepos.getY()/float(getHeight())) * (ySlider.getMaximum() - ySlider.getMinimum())+ ySlider.getMinimum());
            repaint();
            BroadCastThresholds();
        }

        /**
         * Adds PianoRoll_InteractiveIndividualBlockWithProbability instances to an internal vector.
         * THis way sampling thresholds in the "listener" widgets can be automatically updated
         * @param widget
         */
        void addWidget(InteractiveIndividualBlockWithProbability* widget)
        {
            ListenerWidgets.push_back(widget);
            BroadCastThresholds();
        }

        // sends sampling thresholds to listener widgets
        void BroadCastThresholds()
        {
            // at least one widget should have been added using
            // addWidget(). If no listeners, you should use the basic
            // UI::XYPad component
            if (!ListenerWidgets.empty())
            {
                auto thresh = ySlider.getValue();

                for (size_t i=0; i<ListenerWidgets.size(); i++)
                {
                    ListenerWidgets[i]->probabilityCurveWidgetPntr->setSamplingThreshold(float(thresh));
                }
            }

        }


    private:
        juce::Point<float> m_mousepos;

    };

}


// ------------------------------------------------------------------------------------------------------------
// ==========              UI WIDGETS PLACED ON FINAL EDITOR GUI                                  =============
// ==========
// ============================================================================================================
namespace FinalUIWidgets {

    namespace GeneratedDrums
    {
        /***
         * a number of SingleStepPianoRollBlock::PianoRoll_InteractiveIndividualBlockWithProbability placed together in a single
         * row to represent the piano roll for a single drum voice. Also, a SingleStepPianoRollBlock::XYPadWithtListeners is used
         * to interact with the voice sampling/max number of generations allowed.
         */
        class GeneratedDrums_SingleVoice :public juce::Component
        {
        public:

            vector<shared_ptr<SingleStepPianoRollBlock::InteractiveIndividualBlockWithProbability>> interactivePRollBlocks;
            shared_ptr<SingleStepPianoRollBlock::XYPadAutomatableWithSliders> MaxCount_Prob_XYPad; // x-axis will be Max count (0 to time_steps), y-axis is threshold 0 to 1
            juce::Slider midiNumberSlider;
            unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>  midiNumberSliderAttachment;
            juce::Label label;
            int pianoRollSectionWidth {0};

            GeneratedDrums_SingleVoice(juce::AudioProcessorValueTreeState* apvtsPntr, const string label_text, const string maxCountParamID, const string threshParamID, const string midiNumberParamID)
            {
                // attach midiNumberSlider to apvts
                midiNumberSliderAttachment = make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(*apvtsPntr, midiNumberParamID, midiNumberSlider);
                addAndMakeVisible(midiNumberSlider);

                // Set Modified Label
                label.setText(label_text, juce::dontSendNotification);
                label.setColour(juce::Label::textColourId, juce::Colours::white);
                label.setJustificationType (juce::Justification::centredRight);
                addAndMakeVisible(label);


                // xy slider
                MaxCount_Prob_XYPad = make_shared<SingleStepPianoRollBlock::XYPadAutomatableWithSliders>(apvtsPntr, maxCountParamID, threshParamID);
                addAndMakeVisible(MaxCount_Prob_XYPad.get());

                // Draw up piano roll
                array<juce::Colour, 2> bc;
                for (unsigned long i=0; i<HVO_params::time_steps; i++)
                {
                    if (fmod(i,
                             HVO_params::num_steps_per_beat
                                 * HVO_params::num_beats_per_bar)
                        == 0) // bar position
                    {
                        bc = {rest_backg_color, bar_backg_color};
                    }
                    else if (fmod(i,
                                  HVO_params::num_steps_per_beat
                                      * HVO_params::num_beats_per_bar)
                             == 1)
                    {
                        bc = {bar_backg_color, rest_backg_color};
                    }
                    else if (fmod(i, HVO_params::num_steps_per_beat)
                             == 0) // beat position
                    {
                        bc = {rest_backg_color, beat_backg_color};
                    }
                    else if (fmod(i, HVO_params::num_steps_per_beat) == 1)
                    {
                        bc = {beat_backg_color, rest_backg_color};
                    }
                    else // every other position
                    {
                        bc = {rest_backg_color, rest_backg_color};
                    }

                    interactivePRollBlocks.push_back(make_shared<SingleStepPianoRollBlock::
                                                                     InteractiveIndividualBlockWithProbability>(false, bc, i));
                    MaxCount_Prob_XYPad->addWidget(interactivePRollBlocks[i].get());
                    addAndMakeVisible(interactivePRollBlocks[i].get());
                }

            }

            int getPianoRollSectionWidth()
            {
                return pianoRollSectionWidth;
            }

            int getPianoRollLeftBound()
            {
                return label.getWidth();
            }

            void addEventToTimeStep(int time_step_ix, int hit_, float velocity_, float offset_, float probability_)
            {
                interactivePRollBlocks[(size_t)time_step_ix]->addEvent(hit_, velocity_, offset_, probability_, (float)MaxCount_Prob_XYPad->ySlider.getValue());
            }

            void resized() override{
                auto area = getLocalBounds();
                area.removeFromRight(area.getWidth() - (int) area.proportionOfWidth(gui_settings::PianoRolls::label_ratio_of_width));
                label.setBounds(area.removeFromTop(area.proportionOfHeight(0.5f)));
                auto h = area.getHeight();
                midiNumberSlider.setBounds(area);
                midiNumberSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, midiNumberSlider.getWidth()/4, int(h/3.0));

                // place pianorolls for each step
                area = getLocalBounds();
                area.removeFromLeft((int) area.proportionOfWidth(gui_settings::PianoRolls::label_ratio_of_width));
                auto grid_width = area.proportionOfWidth(gui_settings::PianoRolls::timestep_ratio_of_width);
                pianoRollSectionWidth = 0;
                for (size_t i = 0; i<HVO_params::time_steps; i++)
                {
                    interactivePRollBlocks[i]->setBounds (area.removeFromLeft(grid_width));
                    pianoRollSectionWidth += interactivePRollBlocks[i]->getWidth();
                }
                MaxCount_Prob_XYPad->setBounds (area.removeFromBottom(proportionOfHeight(gui_settings::PianoRolls::prob_to_pianoRoll_Ratio)));
            }

        };

        /***
         * num_voices of PianoRoll_GeneratedDrums_SingleVoice packed on top of each other to represent the entire pianoRoll
         */
        class GeneratedDrumsWidget :public juce::Component
        {
        public:
            vector<unique_ptr<GeneratedDrums_SingleVoice>> PianoRolls;

            GeneratedDrumsWidget(juce::AudioProcessorValueTreeState* apvtsPntr)
            {
                auto DrumVoiceNames_ = nine_voice_kit_labels;
                auto DrumVoiceMidiNumbers_ = nine_voice_kit_default_midi_numbers;

                for (size_t voice_i=0; voice_i<HVO_params::num_voices; voice_i++)
                {
                    auto voice_label = DrumVoiceNames_[voice_i];
                    auto label_txt = voice_label;

                    PianoRolls.push_back(make_unique<GeneratedDrums_SingleVoice>(
                        apvtsPntr, label_txt, voice_label+"_X", voice_label+"_Y", voice_label+"_MIDI"));

                    addAndMakeVisible(PianoRolls[voice_i].get());
                }
            }

            void resized() override {
                auto area = getLocalBounds();
                int PRollheight = (int((float) area.getHeight() )) / HVO_params::num_voices;
                for (size_t voice_i=0; voice_i<HVO_params::num_voices; voice_i++)
                {
                    PianoRolls[voice_i]->setBounds(area.removeFromBottom(PRollheight));
                }
            }

            void addEventToVoice(int voice_number, int timestep_idx, int hit_, float velocity_, float offset, float probability_)
            {
                // add note
                PianoRolls[(size_t) voice_number]->addEventToTimeStep(timestep_idx, hit_, velocity_, offset, probability_);

            }

            void updateWithNewScore(const HVOLight <HVO_params::time_steps, HVO_params::num_voices> latest_generated_data)
            {

                for (int vn_= 0; vn_ < latest_generated_data.num_voices; vn_++)
                {
                    for (int t_= 0; t_ < latest_generated_data.time_steps; t_++)
                    {
                        addEventToVoice(
                            vn_,
                            t_,
                            latest_generated_data.hits[t_][vn_].item().toInt(),
                            latest_generated_data.velocities[t_][vn_].item().toFloat(),
                            latest_generated_data.offsets[t_][vn_].item().toFloat(),
                            latest_generated_data.hit_probabilities[t_][vn_].item().toFloat());
                    }

                }

                old_generated_data = latest_generated_data;
            }

            void UpdatePlayheadLocation (double playhead_percentage_)
            {
                playhead_percentage = playhead_percentage_;
                repaint();
            }

            void paint(juce::Graphics& g) override
            {
                auto w = PianoRolls[0]->getPianoRollSectionWidth();
                auto x0 = PianoRolls[0]->getPianoRollLeftBound();
                auto x = float (w * playhead_percentage + x0);
                g.setColour(playback_progressbar_color);
                g.drawLine(x, 0, x, float(getHeight()));
            }
        private:
            HVOLight <HVO_params::time_steps, HVO_params::num_voices> old_generated_data{};
            double playhead_percentage {0};
        };

    }

    namespace MonotonicGrooves
    {
        /***
     * a number of SingleStepPianoRollBlock::PianoRoll_InteractiveIndividualBlockWithProbability placed together in a single
     * row to represent either the piano roll for the unmodified (interactive) OR the modified (non-interactive) sections of
     * of the piano roll for the Input Groove.
     */
        class InteractiveMonotonicGrooveSingleRow :public juce::Component
        {
        public:

            vector<shared_ptr<SingleStepPianoRollBlock::InteractiveIndividualBlock>> interactivePRollBlocks;
            juce::Label label;

            InteractiveMonotonicGrooveSingleRow(bool isInteractive, const string label_text,
                                                LockFreeQueue<BasicNote, GeneralSettings::gui_io_queue_size>* GroovePianoRollWidget2GrooveThread_manually_drawn_noteQue = nullptr)
            {

                // Set Modified Label
                label.setText(label_text, juce::dontSendNotification);
                label.setColour(juce::Label::textColourId, juce::Colours::white);
                label.setJustificationType (juce::Justification::centredRight);
                addAndMakeVisible(label);

                // Draw up piano roll
                /*auto w_per_block = (int) size_width/num_gridlines;*/
                array<juce::Colour, 2> bc;
                for (unsigned long i=0; i<HVO_params::time_steps; i++)
                {
                    if (fmod(i,
                             HVO_params::num_steps_per_beat
                                 * HVO_params::num_beats_per_bar)
                        == 0) // bar position
                    {
                        bc = {rest_backg_color, bar_backg_color};
                    }
                    else if (fmod(i,
                                  HVO_params::num_steps_per_beat
                                      * HVO_params::num_beats_per_bar)
                             == 1)
                    {
                        bc = {bar_backg_color, rest_backg_color};
                    }
                    else if (fmod(i, HVO_params::num_steps_per_beat)
                             == 0) // beat position
                    {
                        bc = {rest_backg_color, beat_backg_color};
                    }
                    else if (fmod(i, HVO_params::num_steps_per_beat) == 1)
                    {
                        bc = {beat_backg_color, rest_backg_color};
                    }
                    else // every other position
                    {
                        bc = {rest_backg_color, rest_backg_color};
                    }
                    interactivePRollBlocks.push_back(make_shared<SingleStepPianoRollBlock::InteractiveIndividualBlock>(isInteractive, bc, i, GroovePianoRollWidget2GrooveThread_manually_drawn_noteQue));
                    addAndMakeVisible(interactivePRollBlocks[i].get());
                }


            }

            // location must be between 0 or 1
            void addEventToStep(int idx, int hit_, float velocity_, float offset_)
            {
                interactivePRollBlocks[(size_t) idx]->addEvent(hit_, velocity_, offset_);
            }


            void resized() override {
                auto area = getLocalBounds();
                label.setBounds(area.removeFromLeft((int) area.proportionOfWidth(gui_settings::PianoRolls::label_ratio_of_width)));
                auto grid_width = area.proportionOfWidth(gui_settings::PianoRolls::timestep_ratio_of_width);
                for (size_t i = 0; i<HVO_params::time_steps; i++)
                {
                    interactivePRollBlocks[i]->setBounds (area.removeFromLeft(grid_width));
                }
            }


        };

        /**
     * wraps two instances of PianoRoll_InteractiveMonotonicGroove together on top of each other.
     * Bottom one is unModified groove (interactive) and top one is Modified groove (non-interactive)
     * pianorolls.
     */
        class MonotonicGrooveWidget:public juce::Component
        {
        public:
            unique_ptr<InteractiveMonotonicGrooveSingleRow> unModifiedGrooveGui;
            unique_ptr<InteractiveMonotonicGrooveSingleRow> ModifiedGrooveGui;

            MonotonicGrooveWidget(LockFreeQueue<BasicNote, GeneralSettings::gui_io_queue_size>* GroovePianoRollWidget2GrooveThread_manually_drawn_noteQue = nullptr)
            {
                // Create Unmodified Piano ROll
                unModifiedGrooveGui = make_unique<InteractiveMonotonicGrooveSingleRow>(true, "Unmodified Groove", GroovePianoRollWidget2GrooveThread_manually_drawn_noteQue);
                addAndMakeVisible(unModifiedGrooveGui.get());
                // Create Unmodified Piano ROll
                ModifiedGrooveGui = make_unique<InteractiveMonotonicGrooveSingleRow>(false, "Adjusted Groove");
                addAndMakeVisible(ModifiedGrooveGui.get());

            }

            void resized() override {
                auto area = getLocalBounds();
                auto height = int((float) area.getHeight() *0.45f);
                ModifiedGrooveGui->setBounds(area.removeFromTop(height));
                unModifiedGrooveGui->setBounds(area.removeFromBottom(height));

            }

            void updateWithNewGroove(MonotonicGroove<HVO_params::time_steps> new_groove)
            {
                for (int i = 0; i < HVO_params::time_steps; i++)
                {
                    unModifiedGrooveGui->interactivePRollBlocks[(size_t) i]->addEvent(
                        new_groove.hvo.hits[i].item().toInt(),
                        new_groove.hvo.velocities_unmodified[i].item().toFloat(),
                        new_groove.hvo.offsets_unmodified[i].item().toFloat());

                    ModifiedGrooveGui->interactivePRollBlocks[(size_t) i]->addEvent(
                        new_groove.hvo.hits[i].item().toInt(),
                        new_groove.hvo.velocities_modified[i].item().toFloat(),
                        new_groove.hvo.offsets_modified[i].item().toFloat());
                }
            }

        };

    }

    class ControlsWidget : public juce::Component
    {
    public:
        // Checkbox for overdub and recording enabling/disabling
        juce::ToggleButton overdubToggle {"Overdub"};
        unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> overdubToggleAttachment;
        juce::ToggleButton recordToggle {"Record"};
        unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> recordToggleAttachment;

        // sliders for groove manipulation
        juce::Slider VelBiasSilder;
        juce::Label  VelBiasLabel;
        unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> VelBiasSilderAPVTSAttacher;
        juce::Slider VelRangeSilder;
        juce::Label  VelRangeLabel;
        unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> VelRangeSilderAPVTSAttacher;
        juce::Slider OffsetBiasSlider;
        juce::Label  OffsetBiasLabel;
        unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> OffsetBiasSliderAPVTSAttacher;
        juce::Slider OffsetRangeSlider;
        juce::Label  OffsetRangeLabel;
        unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> OffsetRangeSliderAPVTSAttacher;

        // sliders for sampling temperature
        juce::Slider temperatureSlider;
        juce::Label  temperatureLabel;
        unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> temperatureSliderAPVTSAttacher;

        ControlsWidget(juce::AudioProcessorValueTreeState* apvtsPntr)
        {
            // add toggles
            overdubToggleAttachment = make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (*apvtsPntr, "OVERDUB", overdubToggle);
            addAndMakeVisible (overdubToggle);
            recordToggleAttachment = make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (*apvtsPntr, "RECORD", recordToggle);
            addAndMakeVisible (recordToggle);

            // add sliders for vel offset ranges
            addAndMakeVisible (VelRangeSilder);
            addAndMakeVisible (VelRangeLabel);
            VelRangeLabel.setText ("Velocity: Dynamic Range", juce::dontSendNotification);
            VelRangeSilderAPVTSAttacher = make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(*apvtsPntr, "VEL_DYNAMIC_RANGE", VelRangeSilder);

            addAndMakeVisible (VelBiasSilder);
            addAndMakeVisible (VelBiasLabel);
            VelBiasSilderAPVTSAttacher = make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(*apvtsPntr, "VEL_BIAS", VelBiasSilder);
            VelBiasLabel.setText  ("Velocity: Bias", juce::dontSendNotification);

            addAndMakeVisible (OffsetRangeSlider);
            addAndMakeVisible (OffsetRangeLabel);
            OffsetRangeLabel.setText ("Offset: Dynamic Range", juce::dontSendNotification);
            OffsetRangeSliderAPVTSAttacher = make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(*apvtsPntr, "OFFSET_RANGE", OffsetRangeSlider);

            addAndMakeVisible (OffsetBiasSlider);
            addAndMakeVisible (OffsetBiasLabel);
            OffsetBiasLabel.setText ("Offset: Bias", juce::dontSendNotification);
            OffsetBiasSliderAPVTSAttacher = make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(*apvtsPntr, "OFFSET_BIAS", OffsetBiasSlider);


            // add temperature Slider
            addAndMakeVisible (temperatureSlider);
            addAndMakeVisible (temperatureLabel);
            temperatureLabel.setText ("Temperature", juce::dontSendNotification);
            temperatureSliderAPVTSAttacher = make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(*apvtsPntr, "Temperature", temperatureSlider);
        }

        void resized() override
        {
            float toggle_height_ratio = 0.2f;
            float slider_labels_width_ratio = 0.3f;
            {
                auto area = getLocalBounds();
                area.removeFromBottom(proportionOfHeight(1.0f - toggle_height_ratio));
                auto toggle_w = proportionOfWidth(.5f);
                overdubToggle.setBounds(area.removeFromLeft(toggle_w));
                recordToggle.setBounds(area.removeFromLeft(toggle_w));
            }

            // put vel offset range sliders
            {
                auto area = getLocalBounds();
                area.removeFromTop(proportionOfHeight(toggle_height_ratio));
                area.removeFromTop(2 * proportionOfHeight(.05f));
                area.removeFromLeft(area.proportionOfWidth(1.0f - slider_labels_width_ratio));
                auto height = area.proportionOfHeight(1.0f/5.0f);
                VelRangeLabel.setBounds(area.removeFromTop(height));
                VelBiasLabel.setBounds(area.removeFromTop(height));
                OffsetRangeLabel.setBounds(area.removeFromTop(height));
                OffsetBiasLabel.setBounds(area.removeFromTop(height));
                temperatureLabel.setBounds(area.removeFromTop(height));
            }

            {
                auto area = getLocalBounds();
                area.removeFromTop(proportionOfHeight(toggle_height_ratio));
                area.removeFromRight(area.proportionOfWidth(slider_labels_width_ratio));
                auto height = area.proportionOfHeight(1.0f/5.0f);

                VelRangeSilder.setBounds(area.removeFromTop(height));
                VelBiasSilder.setBounds(area.removeFromTop(height));
                OffsetRangeSlider.setBounds(area.removeFromTop(height));
                OffsetBiasSlider.setBounds(area.removeFromTop(height));
                temperatureSlider.setBounds(area.removeFromTop(height));

            }
        }

    };

    class ButtonsWidget: public juce::Component
    {
    public:
        ButtonsWidget(juce::AudioProcessorValueTreeState* apvtsPntr)
        {
            resetGrooveButton = make_unique<ButtonWithAttachment>(apvtsPntr, "Reset Groove", "RESET_GROOVE");
            addAndMakeVisible(*resetGrooveButton);
            resetSamplingParametersButton = make_unique<ButtonWithAttachment>(apvtsPntr, "Reset Sampling Parameters", "RESET_SAMPLINGPARAMS");
            addAndMakeVisible(*resetSamplingParametersButton);
            resetAllButton = make_unique<ButtonWithAttachment>(apvtsPntr, "Reset All", "RESET_ALL");
            addAndMakeVisible(*resetAllButton);

            randomVelButton = make_unique<ButtonWithAttachment>(apvtsPntr, "Randomize Velocity", "RANDOMIZE_VEL");
            addAndMakeVisible(*randomVelButton);
            randomOffsetButton = make_unique<ButtonWithAttachment>(apvtsPntr, "Randomize Offset", "RANDOMIZE_OFFSET");
            addAndMakeVisible(*randomOffsetButton);
            randomAllButton = make_unique<ButtonWithAttachment>(apvtsPntr, "Random Groove", "RANDOMIZE_ALL");
            addAndMakeVisible(*randomAllButton);
        }

        void resized() override
        {
            auto area = getLocalBounds();
            area.removeFromLeft(proportionOfWidth(0.15f));
            area.removeFromRight(proportionOfWidth(0.15f));
            area.removeFromTop(proportionOfHeight(0.1f));
            area.removeFromBottom(proportionOfHeight(0.1f));

            auto gap_h = area.proportionOfHeight(0.1f);
            auto button_h = area.proportionOfHeight(0.15f);

            // layout buttom up
            resetAllButton->setBounds(area.removeFromBottom(button_h));
            resetSamplingParametersButton->setBounds(area.removeFromBottom(button_h));
            resetGrooveButton->setBounds(area.removeFromBottom(button_h));

            area.removeFromBottom(gap_h);

            randomAllButton->setBounds(area.removeFromBottom(button_h));
            randomOffsetButton->setBounds(area.removeFromBottom(button_h));
            randomVelButton->setBounds(area.removeFromBottom(button_h));

        }

        // buttons for reseting groove or xyslider params
        unique_ptr<ButtonWithAttachment>  resetGrooveButton;
        unique_ptr<ButtonWithAttachment> resetSamplingParametersButton;
        unique_ptr<ButtonWithAttachment> resetAllButton;

        // buttons for randomizing groove
        unique_ptr<ButtonWithAttachment> randomVelButton;
        unique_ptr<ButtonWithAttachment> randomOffsetButton;
        unique_ptr<ButtonWithAttachment> randomAllButton;

    };

    class ModelSelectorWidget: public juce::Component
    {
    public:

        NeuralMidiFXPluginProcessor* MidiFXProcessorPntr;

        juce::StringArray model_paths;
        // Model selector
        int num_choices {0};
        juce::Label textLabel { {}, "Select Model:" };
        juce::ComboBox ComboBox;
        unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> ComboBoxAPVTSAttacher;


        ModelSelectorWidget(juce::AudioProcessorValueTreeState* apvtsPntr, string ParameterID_, juce::StringArray model_paths_)
        {
            addAndMakeVisible(ComboBox);

            model_paths = model_paths_;

            for (auto model_path: model_paths)
            {
                num_choices++;
                ComboBox.addItem((string)GeneralSettings::default_model_folder + "/" + model_path.toStdString() + ".pt", num_choices);
            }

            ComboBoxAPVTSAttacher = make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (*apvtsPntr, ParameterID_, ComboBox);

            // ComboBox.setSelectedId (1);

        }

        void resized() override
        {
            auto area = getLocalBounds();
            area.removeFromLeft(proportionOfWidth(0.15f));
            area.removeFromRight(proportionOfWidth(0.15f));
            textLabel.setBounds(area.removeFromTop(proportionOfHeight(0.5f)));
            ComboBox.setBounds(area);
        }

    };
}



