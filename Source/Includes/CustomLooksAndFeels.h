#include <utility>

//
// Created by u153171 on 12/19/2023.
//

#pragma once


using json = nlohmann::json; // Alias for convenience
using namespace juce;

class MyCustomLookAndFeel : public juce::LookAndFeel_V4 {
public:
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle style, juce::Slider& slider) override {

        if (slider.isRotary()) {
            return;
        }

        // make background transparent
        g.fillAll(juce::Colours::transparentBlack);

        g.setColour(juce::Colours::darkgrey);

        if(slider.isVertical())
        {
            auto sliderWidth =  float(slider.getWidth());
            auto sliderHeight = float(slider.getHeight());

            auto top_bottom_margin = 0.0f;
            auto left_right_margin = sliderWidth * 0.1f / 2.0f;
            auto sliderBoundaryWidth = sliderWidth - 2.0f * left_right_margin;
            auto thumbHeight = sliderHeight * 0.1f;
            auto sliderBoundaryHeight = sliderHeight;

            auto thumbY = sliderPos;

            Rectangle<float> sliderBoundary { left_right_margin, top_bottom_margin , sliderBoundaryWidth, sliderBoundaryHeight};
            g.setColour(juce::Colours::black);
            g.drawRoundedRectangle(sliderBoundary, 5.0f, 1.0f);

            Rectangle<float> sliderThumb{ left_right_margin, thumbY-thumbHeight/2.0f, sliderBoundaryWidth, thumbHeight };
            g.setColour(Colour(juce::Colours::black).withAlpha(0.8f));
            g.drawRoundedRectangle(sliderThumb, 5.0f, 1.0f);

            Rectangle<float> sliderActiveRange { sliderThumb.getCentreX() - sliderWidth / 4.0f
                                                , sliderPos+thumbHeight/2.0f, 2 * (sliderThumb.getCentreX() - sliderWidth / 4.0f), sliderBoundaryHeight - sliderPos - thumbHeight/2.0f};
            // transparent grey
            g.setColour(Colour(juce::Colours::darkgrey).withAlpha(0.15f));
            g.fillRoundedRectangle(sliderActiveRange, 5.0f);

            // Draw a line in the middle of the thumb
            g.setColour(juce::Colours::black);
            g.drawLine(sliderThumb.getX(), sliderThumb.getCentreY(), sliderThumb.getRight(), sliderThumb.getCentreY(), 2.0f);

        } else if (slider.isHorizontal()){

            auto sliderWidth = float(slider.getWidth());
            auto sliderHeight = float(slider.getHeight());

            auto top_bottom_margin = sliderHeight * 0.1f / 2.0f;
            auto left_right_margin = 0.0f;
            auto sliderBoundaryHeight = sliderHeight - 2.0f * top_bottom_margin;
            auto thumbWidth = std::min(sliderWidth * 0.1f, 20.0f);
            auto sliderBoundaryWidth = sliderWidth;

            auto thumbX = sliderPos;

            Rectangle<float> sliderBoundary{ left_right_margin, top_bottom_margin, sliderBoundaryWidth, sliderBoundaryHeight};
            g.setColour(juce::Colours::black);
            g.drawRoundedRectangle(sliderBoundary, 5.0f, 1.0f);

            Rectangle<float> sliderThumb{ thumbX - thumbWidth / 2.0f, top_bottom_margin, thumbWidth, sliderBoundaryHeight };
            g.setColour(Colour(juce::Colours::black).withAlpha(0.8f));
            g.drawRoundedRectangle(sliderThumb, 5.0f, 1.0f);

            Rectangle<float> sliderActiveRange{ left_right_margin, top_bottom_margin,
                                                thumbX - left_right_margin - thumbWidth / 2.0f, sliderBoundaryHeight };
            // transparent grey
            g.setColour(Colour(juce::Colours::darkgrey).withAlpha(0.15f));
            g.fillRoundedRectangle(sliderActiveRange, 5.0f);

            // Draw a line in the middle of the thumb for horizontal sliders
            g.setColour(juce::Colours::black);
            g.drawLine(sliderThumb.getCentreX(), sliderThumb.getY(), sliderThumb.getCentreX(), sliderThumb.getBottom(), 2.0f);
        }
    }

};
