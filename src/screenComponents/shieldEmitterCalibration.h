#ifndef SHIELD_EMITTER_CALIBRATION_H
#define SHIELD_EMITTER_CALIBRATION_H

#include "gui/gui2_overlay.h"
#include "gui/gui2_panel.h"
#include "gui/gui2_button.h"
#include "gui/gui2_label.h"
#include "gui/gui2_slider.h"
#include "gui/gui2_element.h"
#include "screenComponents/signalQualityIndicator.h"
#include "timer.h"
#include <functional>
#include <string>
#include <array>

// Forward declare the visualization classes
class ShieldVisualizationElement;

class ShieldEmitterCalibrationGame : public GuiOverlay
{
public:
    using CompletionCallback = std::function<void(bool success)>;
    
    enum class ShieldSide {
        Front,
        Rear
    };

    struct Emitter {
        float position_angle;    // Position on arc (-90 to 90 for front, 90 to 270 for rear)
        float width;            // Beam width in degrees (angular spread)
        float length;           // Arc radius/reach (30-90 = percentage of max radius)
        float target_width;     // Optimal width to achieve
        float target_length;    // Optimal length to achieve
        
        GuiSlider* width_slider;
        GuiSlider* length_slider;
        GuiLabel* width_label;
        GuiLabel* length_label;
    };

    ShieldEmitterCalibrationGame(GuiContainer* owner, ShieldSide side, CompletionCallback callback);

private:

    CompletionCallback completion_callback;
    ShieldSide shield_side;
    
    GuiPanel* main_panel;
    GuiLabel* title_label;
    GuiLabel* instruction_label;
    GuiLabel* coverage_label;
    ShieldVisualizationElement* shield_visual;
    
    GuiPanel* controls_panel;
    GuiPanel* fine_tune_panel;
    GuiSignalQualityIndicator* signal_quality;
    GuiLabel* locked_label;  // "LOCKED" label shown when calibration is locked
    GuiButton* close_button;
    GuiButton* cancel_button;  // Cancel/Activate button in fine-tune panel
    
    std::array<Emitter, 4> emitters;
    bool puzzle_complete;
    bool coarse_alignment_done;
    bool fine_tuning_unlocked;
    
    // Fine-tuning parameters (normalized 0-1 like scanning dialog)
    static const int max_sliders = 2;  // Just 2 sliders like simple scanning
    float target[max_sliders];  // Target values for each slider (0-1)
    GuiSlider* sliders[max_sliders];
    bool fine_locked;
    float fine_lock_start_time;
    
    void initializeEmitters();
    void createFineTuneControls();
    void onSliderChanged();
    void updateCoverageDisplay();
    void unlockFineTuning();
    virtual void onDraw(sp::RenderTarget& renderer) override;
    float calculateCoarseMatch();
    float calculateFineTuneMatch();
    bool checkCalibrationComplete();
    void updateSignalQuality();
};

// Custom element for rendering shield arc visualization
class ShieldVisualizationElement : public GuiElement
{
public:
    ShieldVisualizationElement(GuiContainer* owner, string id, ShieldEmitterCalibrationGame::ShieldSide side,
                               std::array<ShieldEmitterCalibrationGame::Emitter, 4>& emitters);
    
    virtual void onDraw(sp::RenderTarget& renderer) override;
    
private:
    ShieldEmitterCalibrationGame::ShieldSide shield_side;
    std::array<ShieldEmitterCalibrationGame::Emitter, 4>& emitters;
    
    void drawArc(sp::RenderTarget& renderer, glm::vec2 center, float angle_start, float angle_arc, 
                 float radius, glm::u8vec4 color);
};

#endif // SHIELD_EMITTER_CALIBRATION_H
