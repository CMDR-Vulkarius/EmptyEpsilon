#ifndef TARGETING_COMPUTER_CALIBRATION_H
#define TARGETING_COMPUTER_CALIBRATION_H

#include "gui/gui2_overlay.h"
#include "gui/gui2_panel.h"
#include "gui/gui2_button.h"
#include "gui/gui2_label.h"
#include "gui/gui2_element.h"
#include "gui/gui2_slider.h"
#include "timer.h"
#include <functional>
#include <string>
#include <array>
#include <glm/vec2.hpp>

// Forward declarations
class RadarDisplayElement;
class DraggableReticle;

class TargetingComputerCalibrationGame : public GuiOverlay
{
public:
    using CompletionCallback = std::function<void(bool success)>;

    struct Reticle {
        glm::vec2 position;         // Current position on radar (normalized -1 to 1)
        glm::vec2 target_position;  // Target position
        DraggableReticle* element;  // GUI element for dragging
    };

    struct TargetingParameter {
        string name;
        float target_value;
        float min_value;
        float max_value;
        bool locked;
        GuiSlider* slider;          // Player adjusts this
        GuiLabel* value_label;      // Shows current slider value
        GuiLabel* target_label;     // Shows target value
        GuiButton* lock_button;
    };

    TargetingComputerCalibrationGame(GuiContainer* owner, CompletionCallback callback);

private:
    CompletionCallback completion_callback;
    bool puzzle_complete;

    // Stage 1: Reticle Alignment
    GuiPanel* stage1_panel;
    RadarDisplayElement* radar_display;
    static constexpr int num_reticles = 5;
    std::array<Reticle, num_reticles> reticles;
    GuiLabel* alignment_status_label;
    GuiButton* stage1_continue_button;
    float alignment_tolerance;

    // Stage 2: Targeting Solution Lock
    GuiPanel* stage2_panel;
    static constexpr int num_parameters = 4;
    std::array<TargetingParameter, num_parameters> parameters;
    GuiLabel* solution_status_label;
    GuiButton* stage2_complete_button;
    float lock_tolerance;

    // Common
    GuiButton* cancel_button;

    void initializeStage1();
    void initializeStage2();
    void unlockStage2();
    
    void updateReticleAlignment();
    void updateTargetingSolution();
    bool checkStage1Complete();
    bool checkStage2Complete();

    void onReticleMoved();
    
    virtual void onUpdate() override;

    friend class RadarDisplayElement;
    friend class DraggableReticle;
};

// Custom GUI element for radar display with reticles
class RadarDisplayElement : public GuiElement
{
public:
    RadarDisplayElement(GuiContainer* owner, string id, 
                       std::array<TargetingComputerCalibrationGame::Reticle, TargetingComputerCalibrationGame::num_reticles>& reticles);
    
    virtual void onDraw(sp::RenderTarget& renderer) override;

private:
    std::array<TargetingComputerCalibrationGame::Reticle, TargetingComputerCalibrationGame::num_reticles>& reticles;
    void drawRadarBackground(sp::RenderTarget& renderer, glm::vec2 center, float radius);
    void drawTargetMarker(sp::RenderTarget& renderer, glm::vec2 center, float radius, glm::vec2 target_pos);
};

// Draggable reticle element
class DraggableReticle : public GuiElement
{
public:
    DraggableReticle(GuiContainer* owner, string id, TargetingComputerCalibrationGame::Reticle& reticle,
                     float radar_radius, glm::vec2 radar_center);
    
    virtual void onDraw(sp::RenderTarget& renderer) override;
    virtual bool onMouseDown(sp::io::Pointer::Button button, glm::vec2 position, sp::io::Pointer::ID id) override;
    virtual void onMouseDrag(glm::vec2 position, sp::io::Pointer::ID id) override;
    virtual void onMouseUp(glm::vec2 position, sp::io::Pointer::ID id) override;

    void setMoveCallback(std::function<void()> callback) { move_callback = callback; }
    void updateVisualPosition();

private:
    TargetingComputerCalibrationGame::Reticle& reticle;
    float radar_radius;
    glm::vec2 radar_center;
    bool dragging;
    glm::vec2 drag_offset;  // Offset from reticle center when grabbed
    std::function<void()> move_callback;

    void drawReticle(sp::RenderTarget& renderer, glm::vec2 position);
};

#endif // TARGETING_COMPUTER_CALIBRATION_H
