#include <utility>

//
// Created by u153171 on 12/19/2023.
//

#pragma once

class TriangleSliders : public juce::Component, public juce::Slider::Listener
{
public:

    explicit TriangleSliders(json triangleSlidersJson_,
                             juce::AudioProcessorValueTreeState* apvtsPntr_,
                             juce::Label *sharedInfoLabel_)
    {

        triangleSlidersJson = std::move(triangleSlidersJson_);
        apvtsPntr = apvtsPntr_;
        sharedInfoLabel = sharedInfoLabel_;

        // DistanceFromASlider Attachment and don't display the slider
        auto sl_a_param = label2ParamID(
            triangleSlidersJson["DistanceFromBottomLeftCornerSlider"].get<std::string>());

        DistanceFromASliderAttachment = std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
            *apvtsPntr, sl_a_param, DistanceFromASlider);

        // DistanceFromCSlider Attachment and don't display the slider
        auto sl_c_param = label2ParamID(
            triangleSlidersJson["HeightSlider"].get<std::string>());

        DistanceFromCSliderAttachment = std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
            *apvtsPntr, sl_c_param, DistanceFromCSlider);

        // listen to the sliders
        DistanceFromASlider.addListener(this);
        DistanceFromCSlider.addListener(this);

        // set the slider values
        checkSliderValues();
        updateCircleCentre();
        repaint();
    }

    void updateCircleCentre(bool dontUpdateSliders = false)
    {
        auto ab_interp = DistanceFromASlider.getValue();
        circleCentre = A + (B - A) * ab_interp;
        auto cc_interp = DistanceFromCSlider.getValue();
        circleCentre = circleCentre + (C - circleCentre) * cc_interp;

        if (!dontUpdateSliders) {
            CirclePosToSliders();
        }
    }

    void CirclePosToSliders()
    {
        // check if circle is aligned with the mid point of AB
        if (circleCentre.x == (A.x + B.x) / 2.0f) {
            DistanceFromASlider.setValue(0.5f);
        } else {
            // find the intersection of C/CircleCentre with AB
            auto ab_slope = (B.y - A.y) / (B.x - A.x);
            auto ab_intercept = A.y - ab_slope * A.x;
            auto circle_slope = (C.y - circleCentre.y) / (C.x - circleCentre.x);
            auto circle_intercept = circleCentre.y - circle_slope * circleCentre.x;
            auto x = (circle_intercept - ab_intercept) / (ab_slope - circle_slope);
            auto ab_interp = (x - A.x) / (B.x - A.x);

            // DistanceFromCSlider
            // find C and CircleCentre distance
            auto c_circle_dist = circleCentre.y - A.y;
            DistanceFromCSlider.setValue(c_circle_dist / (C.y - A.y));

            DistanceFromASlider.setValue(ab_interp);


        }

        repaint();

    }

    // listen to the sliders
    void sliderValueChanged(juce::Slider* /*slider*/) override
    {
        checkSliderValues(false, true);
    }

    bool checkSliderValues(bool forceUpdate = false, bool ignore_loop_update = false)
    {
        if (std::abs(prevDistanceFromASliderValue - DistanceFromASlider.getValue())
                < 0.0001f &&
            std::abs(prevDistanceFromCSliderValue - DistanceFromCSlider.getValue())
                < 0.0001f
            && !forceUpdate){

            return false;
        }

        prevDistanceFromASliderValue = DistanceFromASlider.getValue();
        prevDistanceFromCSliderValue = DistanceFromCSlider.getValue();

        // update the circle centre
        updateCircleCentre(ignore_loop_update);
        repaint();

        return true;
    }

    void paint (juce::Graphics& g) override
    {

        auto color_a = juce::Colours::lightgrey ;
        auto color_b = juce::Colours::white;
        auto color_c = juce::Colours::lightblue;

        // Draw the triangle
        juce::Path triangle;
        triangle.addTriangle(A, B, C);

        g.setColour(juce::Colours::darkgrey);
        g.setOpacity(1.0f);
        // draw the edges of the triangle
        g.strokePath(triangle, juce::PathStrokeType(5.0f));
        g.setColour(juce::Colours::black);
        g.fillPath(triangle);

        // draw three small triangles in the corners
        // to indicate the position of the circle
        juce::Path triangle_a; // A to AB/10 to AC/10
        triangle_a.addTriangle(A, A + (B - A) / 20.0f, A + (C - A) / 20.0f);
        g.setColour(color_a);
        g.setOpacity(1.0f);
        g.fillPath(triangle_a);


        juce::Path triangle_b; // B to BA/10 to BC/10
        triangle_b.addTriangle(B, B + (A - B) / 20.0f, B + (C - B) / 20.0f);
        g.setColour(color_b);
        g.setOpacity(1.0f);
        g.fillPath(triangle_b);

        juce::Path triangle_c; // C to CA/10 to CB/10
        triangle_c.addTriangle(C, C + (A - C) / 20.0f, C + (B - C) / 20.0f);
        g.setColour(color_c);
        g.setOpacity(1.0f);
        g.fillPath(triangle_c);


        // Draw the lines to edges
        // set the opacity of the lines based on contribution of each vertex
        // in the interpolation of the circle centre
        auto alpha = DistanceFromASlider.getValue();
        auto beta = 1.0f - DistanceFromCSlider.getValue();
        auto gamma = beta;
        auto a_opacity = gamma * (1.0f - alpha);
        auto b_opacity = gamma * alpha;
        auto c_opacity = 1.0f - beta;

        // Compute the weighted average for each color channel
        auto totalOpacity = (float) (a_opacity + b_opacity + c_opacity);
        auto averageRed = (color_a.getFloatRed() * a_opacity +
                           color_b.getFloatRed() * b_opacity +
                           color_c.getFloatRed() * c_opacity) / totalOpacity;
        auto averageGreen = (color_a.getFloatGreen() * a_opacity +
                             color_b.getFloatGreen() * b_opacity +
                             color_c.getFloatGreen() * c_opacity) / totalOpacity;
        auto averageBlue = (color_a.getFloatBlue() * a_opacity +
                            color_b.getFloatBlue() * b_opacity +
                            color_c.getFloatBlue() * c_opacity) / totalOpacity;

        // use the normalized distances to change the opacity of the lines
        g.setColour(color_a);
        g.setOpacity(float(a_opacity));
        g.drawLine(juce::Line<float>(circleCentre, A),
                   float(3));
        g.setColour(color_b);
        g.setOpacity(float(b_opacity));
        g.drawLine(juce::Line<float>(circleCentre, B),
                   (float)(3));
        g.setColour(color_c);
        g.setOpacity(float(c_opacity));
        g.drawLine(juce::Line<float>(circleCentre, C),
            float(3));


        // Create the blended color
        auto color_circle = juce::Colour::fromFloatRGBA(
            (float) averageRed, (float) averageGreen,
            (float) averageBlue, 1);


        // Draw the circle
        g.setColour(color_circle);
        // draw the outline of the circle
        g.setOpacity(1.0f);
        g.fillEllipse(circleCentre.x - circleRadius,
                      circleCentre.y - circleRadius,
                      circleRadius * 2, circleRadius * 2);

    }

    void mouseDrag(const juce::MouseEvent& event) override
    {
        // Update the position of the circle
        circleCentre = event.position;
        if (!isInsideTriangle(circleCentre)) {
            circleCentre = closestPointInTriangle(circleCentre);
        }

        CirclePosToSliders();

        repaint();

        updateInfoText();
    }

    void mouseUp(const juce::MouseEvent& event) override
    {
        if (!isPointInTriangle(event.position.x, event.position.y)) {
            return ;
        }
        // Update the position of the circle
        circleCentre = event.position;
        if (!isInsideTriangle(circleCentre)) {
            circleCentre = closestPointInTriangle(circleCentre);
        }

        CirclePosToSliders();

        repaint();

        updateInfoText();
    }

    void mouseMove(const juce::MouseEvent& event) override
    {

        // Check if the point is inside the triangle (allowing for a small margin of error)
        const double epsilon = 0.0001;

        if (isPointInTriangle(event.position.x, event.position.y)) {
            updateInfoText();
        } else {
            sharedInfoLabel->setText("", juce::dontSendNotification);
        }
    }

    static float area(float x1, float y1, float x2, float y2, float x3, float y3) {
        return (float) abs((x1*(y2-y3) + x2*(y3-y1)+ x3*(y1-y2))/2.0);
    }

    [[nodiscard]] bool isPointInTriangle (float x, float y) const {

        float ABC_area = area(A.x, A.y, B.x, B.y, C.x, C.y);

        // check if point is inside the triangle
        float PBC_area = area(x, y, B.x, B.y, C.x, C.y);
        float PAC_area = area(A.x, A.y, x, y, C.x, C.y);
        float PAB_area = area(A.x, A.y, B.x, B.y, x, y);

        return abs((ABC_area) - (PBC_area + PAC_area + PAB_area)) < 0.1f;
    }

    void updateInfoText() {
        // Update the info label
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2);

        ss << triangleSlidersJson["label"].get<std::string>() << " | ";
        ss << triangleSlidersJson["DistanceFromBottomLeftCornerSlider"].get<std::string>() <<
            ": " << DistanceFromASlider.getValue()  << " | ";
        ss << triangleSlidersJson["HeightSlider"].get<std::string>() <<
            ": " << DistanceFromCSlider.getValue();

        if (triangleSlidersJson.contains("info")) {
            auto info = triangleSlidersJson["info"].get<std::string>();
            ss << " | " << info;
        }

        ss << std::endl;
        sharedInfoLabel->setText(ss.str(), juce::dontSendNotification);
    }

    void mouseExit(const juce::MouseEvent& /*event*/) override
    {
        // Update the info label
        sharedInfoLabel->setText("", juce::dontSendNotification);
    }

    void resized() override
    {
        // Calculate the position of the triangle

        bool ensureEquilateral = false;
        if (triangleSlidersJson.contains("ensure_equilateral")) {
            ensureEquilateral = triangleSlidersJson["ensure_equilateral"].get<bool>();
        }

        if (!ensureEquilateral) {
            A = juce::Point<float>(0 + edgeAroundTrianglePixels,
                                   (float) getHeight() - edgeAroundTrianglePixels);
            B = juce::Point<float>((float) getWidth() - edgeAroundTrianglePixels,
                                   (float) getHeight() - edgeAroundTrianglePixels);
            C = juce::Point<float>((float) getWidth() / 2.0f,
                                   0 + edgeAroundTrianglePixels);

        } else {
            float edge_len;
            float height;

            if (getWidth() > getHeight()) {
                height = (float) getHeight();
                edge_len = float (getHeight() * 2.0 / std::sqrt(3.0f));
            } else {
                height = (float) getWidth() * std::sqrt(3.0f) / 2.0f;
                edge_len = float (getWidth());
            }
            // make tri a bit smaller
            edge_len = edge_len * 0.9f;
            auto gap_size = height * 0.1f;
            height = height * (0.9f);

            // bottom left corner of component
            auto BL = juce::Point<float>(
                0, (float) getHeight() - gap_size/2.0f);
            auto BR = juce::Point<float>(
                (float) getWidth(), (float) getHeight() - gap_size/2.0f);

            // mid point of bottom edge
            auto midPoint = (BL + BR) / 2.0f;

            // ABC
            A = midPoint + juce::Point<float>(-edge_len / 2.0f, 0);
            B = midPoint + juce::Point<float>(edge_len / 2.0f, 0);
            C = midPoint + juce::Point<float>(0, -height);

        }

        checkSliderValues(true);
    }

private:
    json triangleSlidersJson;
    juce::AudioProcessorValueTreeState* apvtsPntr;
    juce::Label *sharedInfoLabel;
    juce::Point<float> circleCentre;
    const float circleRadius = 5.0f;

    // Sliders
    juce::Slider DistanceFromASlider;
    juce::Slider DistanceFromCSlider;

    // prev slider values
    double prevDistanceFromASliderValue = -10.0f;
    double prevDistanceFromCSliderValue = -10.0f;

    // float Edge Around Triangle
    float edgeAroundTrianglePixels = 30.0f;

    unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> DistanceFromASliderAttachment;
    unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> DistanceFromCSliderAttachment;

    juce::Point<float> A;  // Lower left corner
    juce::Point<float> B; // Lower right corner
    juce::Point<float> C; // Top corner

    static float sign (juce::Point<float> p1, juce::Point<float> p2, juce::Point<float> p3)
    {
        return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
    }

    bool isInsideTriangle(const juce::Point<float>& /*P*/)
    {

        auto d1 = sign(circleCentre, A, B);
        auto d2 = sign(circleCentre, B, C);
        auto d3 = sign(circleCentre, C, A);

        auto has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
        auto has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

        return !(has_neg && has_pos);
    }

    juce::Point<float> closestPointInTriangle(const juce::Point<float>& P)
    {
        if (isInsideTriangle(P)) {
            return P;
        }

        // Function to calculate the closest point on a line segment to point P
        auto closestPointOnLine = [](const juce::Point<float>& L1, const juce::Point<float>& L2, const juce::Point<float>& P) {
            juce::Point<float> lineVec = L2 - L1;
            juce::Point<float> pointVec = P - L1;
            float lineLen = lineVec.getDistanceFromOrigin();
            float dot = pointVec.x * lineVec.x + pointVec.y * lineVec.y;
            float t = dot / (lineLen * lineLen);
            t = std::max(0.0f, std::min(1.0f, t));
            return L1 + lineVec * t;
        };

        // Find the closest points on each side of the triangle
        juce::Point<float> closestOnAB = closestPointOnLine(A, B, P);
        juce::Point<float> closestOnBC = closestPointOnLine(B, C, P);
        juce::Point<float> closestOnCA = closestPointOnLine(C, A, P);

        // Find the closest of these points to P
        float distAB = closestOnAB.getDistanceFrom(P);
        float distBC = closestOnBC.getDistanceFrom(P);
        float distCA = closestOnCA.getDistanceFrom(P);

        if (distAB < distBC && distAB < distCA) {
            return closestOnAB;
        } else if (distBC < distCA) {
            return closestOnBC;
        } else {
            return closestOnCA;
        }
    }
};
