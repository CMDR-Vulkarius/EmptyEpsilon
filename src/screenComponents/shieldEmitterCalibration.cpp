#include "shieldEmitterCalibration.h"
#include "preferenceManager.h"
#include "playerInfo.h"
#include "spaceObjects/playerSpaceship.h"
#include "random.h"
#include "engine.h"
#include <algorithm>

ShieldEmitterCalibrationGame::ShieldEmitterCalibrationGame(GuiContainer* owner, ShieldSide side, CompletionCallback callback)
: GuiOverlay(owner, "SHIELD_CALIBRATION", colorConfig.background), completion_callback(callback), shield_side(side)
{
    puzzle_complete = false;
    coarse_alignment_done = false;
    fine_tuning_unlocked = false;
    
    // Main panel
    main_panel = new GuiPanel(this, "SHIELD_PANEL");
    main_panel->setPosition(0, 0, sp::Alignment::Center)->setSize(1200, 700);
    
    // Title
    std::string title_text = (side == ShieldSide::Front) ? "FRONT SHIELD EMITTER CALIBRATION" : "REAR SHIELD EMITTER CALIBRATION";
    title_label = new GuiLabel(main_panel, "TITLE", title_text, 26);
    title_label->setPosition(0, 15, sp::Alignment::TopCenter)->setSize(0, 35);
    
    // Instructions
    instruction_label = new GuiLabel(main_panel, "INSTRUCTION", "Match emitter width (angular spread) and length (radial reach) to achieve optimal calibration", 14);
    instruction_label->setPosition(0, 55, sp::Alignment::TopCenter)->setSize(1000, 30);
    
    // Calibration accuracy display
    coverage_label = new GuiLabel(main_panel, "COVERAGE", "CALIBRATION: 0%", 20);
    coverage_label->setPosition(0, 90, sp::Alignment::TopCenter)->setSize(0, 30);
    
    // Shield visualization with real-time arc drawing
    shield_visual = new ShieldVisualizationElement(main_panel, "SHIELD_VISUAL", shield_side, emitters);
    shield_visual->setPosition(0, 130, sp::Alignment::TopCenter)->setSize(600, 400);
    
    // Controls panel for emitter sliders
    controls_panel = new GuiPanel(main_panel, "CONTROLS_PANEL");
    controls_panel->setPosition(0, 540, sp::Alignment::TopCenter)->setSize(1100, 110);
    
    // Fine-tuning panel (initially hidden) - like scanning dialog
    fine_tune_panel = new GuiPanel(main_panel, "FINE_TUNE_PANEL");
    fine_tune_panel->setPosition(0, 0, sp::Alignment::Center)->setSize(500, 405);
    fine_tune_panel->hide();
    
    // Initialize emitters and create sliders
    initializeEmitters();
    
    // Control buttons
    close_button = new GuiButton(main_panel, "CLOSE", "CANCEL", [this]() {
        if (puzzle_complete && completion_callback) {
            completion_callback(true);
        } else if (completion_callback) {
            completion_callback(false);
        }
        destroy();
    });
    close_button->setPosition(0, 660, sp::Alignment::TopCenter)->setSize(200, 40);
    
    updateCoverageDisplay();
}

void ShieldEmitterCalibrationGame::initializeEmitters()
{
    // Generate ONE target length for all emitters (so they form a continuous arc when complete)
    float common_target_length = 50.0f + (rand() % 31); // 50-80 - same for all emitters
    
    // Generate 4 random widths that ADD UP to exactly 180 degrees
    // This ensures complete edge-to-edge coverage with no gaps
    float widths[4];
    float total = 0.0f;
    
    // Generate 4 random values
    for (int i = 0; i < 4; i++) {
        widths[i] = 30.0f + (rand() % 31); // 30-60° base range
        total += widths[i];
    }
    
    // Scale them so they sum to exactly 180°
    for (int i = 0; i < 4; i++) {
        widths[i] = (widths[i] / total) * 180.0f;
    }
    
    // Position emitters edge-to-edge based on cumulative widths
    float current_angle = (shield_side == ShieldSide::Rear) ? 90.0f : -90.0f;
    
    for (int i = 0; i < 4; i++) {
        // Position at the CENTER of this emitter's arc
        emitters[i].position_angle = current_angle + (widths[i] / 2.0f);
        
        // Set the target width
        emitters[i].target_width = widths[i];
        
        // Move to next emitter's start position
        current_angle += widths[i];
        
        // ALL emitters get the SAME target length (to form continuous arc when complete)
        emitters[i].target_length = common_target_length;
        
        // Start at significantly wrong values - far from target to require real adjustment
        // Width: random value across the full slider range, not near target
        emitters[i].width = 20.0f + (rand() % 51); // 20-70° completely random
        
        // Length: random value across the full slider range, not near target
        emitters[i].length = 30.0f + (rand() % 61); // 30-90 completely random
        
        // Make sure they're NOT accidentally close to target (at least 15 units away)
        if (std::abs(emitters[i].width - emitters[i].target_width) < 15.0f) {
            emitters[i].width = emitters[i].target_width + ((emitters[i].width < emitters[i].target_width) ? -20.0f : 20.0f);
            emitters[i].width = std::max(20.0f, std::min(70.0f, emitters[i].width));
        }
        if (std::abs(emitters[i].length - emitters[i].target_length) < 15.0f) {
            emitters[i].length = emitters[i].target_length + ((emitters[i].length < emitters[i].target_length) ? -20.0f : 20.0f);
            emitters[i].length = std::max(30.0f, std::min(90.0f, emitters[i].length));
        }
        
        // Create sliders for this emitter
        int x_base = 15 + (i * 270);  // 15, 285, 555, 825 for 4 emitters in 1100px panel
        int y_pos = 15;
        
        // Emitter label
        GuiLabel* emitter_label = new GuiLabel(controls_panel, "EMITTER_" + std::to_string(i) + "_LABEL", 
                                               "EMITTER " + std::to_string(i + 1), 14);
        emitter_label->setPosition(x_base + 100, y_pos, sp::Alignment::TopLeft)->setSize(200, 20);
        
        // Width slider
        GuiLabel* width_text = new GuiLabel(controls_panel, "WIDTH_TEXT_" + std::to_string(i), "Width:", 12);
        width_text->setPosition(x_base + 10, y_pos + 25, sp::Alignment::TopLeft)->setSize(50, 20);
        
        emitters[i].width_slider = new GuiSlider(controls_panel, "WIDTH_SLIDER_" + std::to_string(i), 
                                                  20.0f, 70.0f, emitters[i].width, 
                                                  [this](float value) { onSliderChanged(); });
        emitters[i].width_slider->setPosition(x_base + 65, y_pos + 25, sp::Alignment::TopLeft)->setSize(150, 20);
        
        emitters[i].width_label = new GuiLabel(controls_panel, "WIDTH_VAL_" + std::to_string(i), "20°", 12);
        emitters[i].width_label->setPosition(x_base + 220, y_pos + 25, sp::Alignment::TopLeft)->setSize(40, 20);
        
        // Length slider
        GuiLabel* length_text = new GuiLabel(controls_panel, "LENGTH_TEXT_" + std::to_string(i), "Length:", 12);
        length_text->setPosition(x_base + 10, y_pos + 50, sp::Alignment::TopLeft)->setSize(50, 20);
        
        emitters[i].length_slider = new GuiSlider(controls_panel, "LENGTH_SLIDER_" + std::to_string(i), 
                                                   30.0f, 90.0f, emitters[i].length, 
                                                   [this](float value) { onSliderChanged(); });
        emitters[i].length_slider->setPosition(x_base + 65, y_pos + 50, sp::Alignment::TopLeft)->setSize(150, 20);
        
        emitters[i].length_label = new GuiLabel(controls_panel, "LENGTH_VAL_" + std::to_string(i), "30°", 12);
        emitters[i].length_label->setPosition(x_base + 220, y_pos + 50, sp::Alignment::TopLeft)->setSize(40, 20);
    }
    
    // Initialize fine-tune parameters (exactly like scanning dialog)
    fine_locked = false;
    fine_lock_start_time = 0.0f;
    
    for(int n=0; n<max_sliders; n++)
    {
        target[n] = random(0.0, 1.0);
        // Start values will be set in createFineTuneControls
    }
    
    createFineTuneControls();
}

void ShieldEmitterCalibrationGame::createFineTuneControls()
{
    // Exactly like scanning dialog layout
    GuiLabel* signal_label = new GuiLabel(fine_tune_panel, "SIGNAL_LABEL", "Shield emitter frequency", 30);
    signal_label->addBackground()->setPosition(0, 20, sp::Alignment::TopCenter)->setSize(450, 50);
    
    signal_quality = new GuiSignalQualityIndicator(fine_tune_panel, "SIGNAL_QUALITY");
    signal_quality->setPosition(0, 80, sp::Alignment::TopCenter)->setSize(450, 100);
    
    // Locked label (shown when calibration is complete) - like scanning dialog
    locked_label = new GuiLabel(signal_quality, "LOCK_LABEL", "LOCKED", 50);
    locked_label->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);
    locked_label->hide();
    
    // Create sliders exactly like scanning dialog - no callbacks, just nullptr
    for(int n=0; n<max_sliders; n++)
    {
        sliders[n] = new GuiSlider(fine_tune_panel, "SLIDER_" + std::to_string(n), 0.0, 1.0, 0.0, nullptr);
        sliders[n]->setPosition(0, 200 + n * 70, sp::Alignment::TopCenter)->setSize(450, 50);
        
        // Set initial value - ensure at least 0.2 away from target
        sliders[n]->setValue(random(0.0, 1.0));
        while(fabsf(target[n] - sliders[n]->getValue()) < 0.2f)
            sliders[n]->setValue(random(0.0, 1.0));
    }
    
    // Cancel button like scanning dialog - but changes to "ACTIVATE REPAIR" when complete
    cancel_button = new GuiButton(fine_tune_panel, "CANCEL", "Cancel", [this]() {
        // If puzzle is complete, activate repair; otherwise cancel
        if (completion_callback)
            completion_callback(puzzle_complete);
        destroy();  // Close the dialog
    });
    cancel_button->setPosition(0, -20, sp::Alignment::BottomCenter)->setSize(300, 50);
    
    updateSignalQuality();
}

void ShieldEmitterCalibrationGame::onSliderChanged()
{
    // Update emitter values from sliders
    for (int i = 0; i < 4; i++) {
        emitters[i].width = emitters[i].width_slider->getValue();
        emitters[i].length = emitters[i].length_slider->getValue();
        
        emitters[i].width_label->setText(std::to_string(int(emitters[i].width)) + "°");
        emitters[i].length_label->setText(std::to_string(int(emitters[i].length)) + "°");
    }
    
    updateCoverageDisplay();
}

void ShieldEmitterCalibrationGame::onDraw(sp::RenderTarget& renderer)
{
    // Call updateSignalQuality every frame in onDraw (exactly like scanning dialog)
    if (fine_tuning_unlocked && fine_tune_panel->isVisible())
    {
        updateSignalQuality();
        
        // Show/hide locked label based on lock progress (like scanning dialog)
        if (fine_locked && engine->getElapsedTime() - fine_lock_start_time > 0.25f)
        {
            locked_label->show();
        }else{
            locked_label->hide();
        }
        
        // Update cancel button text when puzzle is complete
        if (puzzle_complete && cancel_button->getText() != "ACTIVATE REPAIR")
        {
            cancel_button->setText("ACTIVATE REPAIR");
        }
        else if (!puzzle_complete && cancel_button->getText() != "Cancel")
        {
            cancel_button->setText("Cancel");
        }
    }
    GuiOverlay::onDraw(renderer);
}

void ShieldEmitterCalibrationGame::unlockFineTuning()
{
    if (!fine_tuning_unlocked) {
        fine_tuning_unlocked = true;
        controls_panel->hide();
        shield_visual->hide();
        close_button->hide();  // Hide main cancel button - fine-tune panel has its own
        fine_tune_panel->show();
        instruction_label->setText("COARSE ALIGNMENT COMPLETE - Adjust sliders for fine calibration");
    }
}

void ShieldEmitterCalibrationGame::updateCoverageDisplay()
{
    if (!fine_tuning_unlocked) {
        // Coarse alignment phase - show percentage
        float coarse_match = calculateCoarseMatch();
        coverage_label->setText("COARSE ALIGNMENT: " + std::to_string(int(coarse_match)) + "%");
        
        if (coarse_match >= 90.0f && !coarse_alignment_done) {
            coarse_alignment_done = true;
            unlockFineTuning();
        }
    } else {
        // Fine-tuning phase - hide percentage (scanning dialog doesn't show percentage)
        coverage_label->hide();
    }
}

float ShieldEmitterCalibrationGame::calculateCoarseMatch()
{
    // Calculate how close width and length are to targets
    float total_match = 0.0f;
    const float tolerance = 15.0f; // Within 15° is considered perfect (more forgiving)
    
    for (const auto& emitter : emitters) {
        float width_diff = std::abs(emitter.width - emitter.target_width);
        float length_diff = std::abs(emitter.length - emitter.target_length);
        
        float width_match = std::max(0.0f, 100.0f - (width_diff / tolerance) * 100.0f);
        float length_match = std::max(0.0f, 100.0f - (length_diff / tolerance) * 100.0f);
        
        total_match += (width_match + length_match) / 2.0f;
    }
    
    return total_match / 4.0f; // Average across 4 emitters
}

float ShieldEmitterCalibrationGame::calculateFineTuneMatch()
{
    // Calculate total error from all sliders (0-1 normalized values)
    float total_error = 0.0f;
    for(int n=0; n<max_sliders; n++)
    {
        if (sliders[n]->isVisible())
        {
            total_error += fabsf(target[n] - sliders[n]->getValue());
        }
    }
    
    // Convert to percentage match (lower error = higher match)
    // max_error for 2 sliders is 2.0
    float match = std::max(0.0f, 100.0f - (total_error / 2.0f) * 100.0f);
    return match;
}

bool ShieldEmitterCalibrationGame::checkCalibrationComplete()
{
    // Just check if locked - puzzle_complete is set in updateSignalQuality
    return fine_locked;
}

// ========== ShieldVisualizationElement Implementation ==========

ShieldVisualizationElement::ShieldVisualizationElement(GuiContainer* owner, string id, 
    ShieldEmitterCalibrationGame::ShieldSide side,
    std::array<ShieldEmitterCalibrationGame::Emitter, 4>& emitters)
: GuiElement(owner, id), shield_side(side), emitters(emitters)
{
}

void ShieldVisualizationElement::onDraw(sp::RenderTarget& renderer)
{
    // Center of the visualization area
    glm::vec2 center = getCenterPoint();
    float arc_radius = 150.0f;
    float ship_radius = 30.0f;
    
    // Determine arc range based on shield side
    float arc_start_angle = (shield_side == ShieldEmitterCalibrationGame::ShieldSide::Front) ? -90.0f : 90.0f;
    
    // Draw ship center (small circle)
    renderer.fillCircle(center, ship_radius, glm::u8vec4(100, 100, 150, 255));
    renderer.drawCircleOutline(center, ship_radius, 2.0f, glm::u8vec4(150, 150, 200, 255));
    
    // Emitter colors
    glm::u8vec4 emitter_colors[4] = {
        glm::u8vec4(255, 80, 80, 100),   // Red
        glm::u8vec4(80, 255, 80, 100),   // Green  
        glm::u8vec4(80, 80, 255, 100),   // Blue
        glm::u8vec4(255, 255, 80, 100)   // Yellow
    };
    
    // Draw each emitter's coverage arc
    for (int i = 0; i < 4; i++) {
        const auto& emitter = emitters[i];
        
        // Draw target arc as OUTLINE (dashed line showing where it should be)
        float target_radius = 150.0f * (emitter.target_length / 90.0f);
        float target_half_width = emitter.target_width / 2.0f;
        float target_start_angle = emitter.position_angle - target_half_width;
        float target_end_angle = emitter.position_angle + target_half_width;
        
        // Draw target arc outline in white/bright color
        for (float a = target_start_angle; a <= target_end_angle; a += 2.0f) {
            float angle_rad = glm::radians(a - 90.0f);
            glm::vec2 pos = center + glm::vec2(std::cos(angle_rad), std::sin(angle_rad)) * target_radius;
            renderer.fillCircle(pos, 2.0f, glm::u8vec4(255, 255, 255, 150));
        }
        
        // Draw actual arc (current settings) as solid colored arc
        float actual_radius = 150.0f * (emitter.length / 90.0f);
        float actual_half_width = emitter.width / 2.0f;
        float actual_start = emitter.position_angle - actual_half_width;
        float actual_arc = emitter.width;
        
        drawArc(renderer, center, actual_start, actual_arc, actual_radius, emitter_colors[i]);
        
        // Draw emitter position marker at the arc radius
        float marker_angle_rad = glm::radians(emitter.position_angle - 90.0f);
        glm::vec2 marker_pos = center + glm::vec2(std::cos(marker_angle_rad), std::sin(marker_angle_rad)) * 150.0f;
        renderer.fillCircle(marker_pos, 8.0f, emitter_colors[i]);
        renderer.drawCircleOutline(marker_pos, 8.0f, 2.0f, glm::u8vec4(255, 255, 255, 255));
    }
    
    // Draw arc boundary lines
    float outer_radius = 170.0f;
    for (int deg = 0; deg <= 180; deg += 15) {
        float angle = arc_start_angle + deg;
        float angle_rad = glm::radians(angle - 90.0f);
        glm::vec2 inner = center + glm::vec2(std::cos(angle_rad), std::sin(angle_rad)) * ship_radius;
        glm::vec2 outer = center + glm::vec2(std::cos(angle_rad), std::sin(angle_rad)) * outer_radius;
        renderer.drawLine(inner, outer, glm::u8vec4(80, 80, 80, 100));
    }
}

void ShieldVisualizationElement::drawArc(sp::RenderTarget& renderer, glm::vec2 center, 
    float angle_start, float angle_arc, float radius, glm::u8vec4 color)
{
    // Fill the arc area from center outward - create triangle fan manually
    const int segments = std::max(8, static_cast<int>(angle_arc / 5.0f));
    
    std::vector<glm::vec2> points;
    std::vector<uint16_t> indices;
    
    // Add center point first
    points.push_back(center);
    
    // Generate arc edge points
    for (int i = 0; i <= segments; i++) {
        float angle = angle_start + (i * angle_arc / segments);
        float angle_rad = glm::radians(angle - 90.0f);
        glm::vec2 edge_point = center + glm::vec2(std::cos(angle_rad), std::sin(angle_rad)) * radius;
        points.push_back(edge_point);
    }
    
    // Create triangle fan indices: center (0) + pairs of edge points
    for (int i = 1; i <= segments; i++) {
        indices.push_back(0);      // center
        indices.push_back(i);      // current edge point
        indices.push_back(i + 1);  // next edge point
    }
    
    // Draw as indexed triangles
    renderer.drawTriangles(points, indices, color);
}

void ShieldEmitterCalibrationGame::updateSignalQuality()
{
    // EXACTLY like scanning dialog - accumulate all slider errors
    float noise = 0.0;
    float period = 0.0;
    float phase = 0.0;
    
    for(int n=0; n<max_sliders; n++)
    {
        if (sliders[n]->isVisible())
        {
            noise += fabsf(target[n] - sliders[n]->getValue());
            period += fabsf(target[n] - sliders[n]->getValue());
            phase += fabsf(target[n] - sliders[n]->getValue());
        }
    }
    
    // Check for lock (all errors < 0.05)
    if (noise < 0.05f && period < 0.05f && phase < 0.05f)
    {
        if (!fine_locked)
        {
            fine_lock_start_time = engine->getElapsedTime();
            fine_locked = true;
        }
        // Fade to perfect after 0.5 seconds
        if (engine->getElapsedTime() - fine_lock_start_time > 0.5f)
        {
            noise = period = phase = 0.0f;
            // Complete the puzzle
            if (!puzzle_complete)
            {
                puzzle_complete = true;
                if (completion_callback)
                    completion_callback(true);
            }
        }else{
            float f = 1.0f - (engine->getElapsedTime() - fine_lock_start_time) / 0.5f;
            noise *= f;
            period *= f;
            phase *= f;
        }
    }else{
        fine_locked = false;
    }
    
    signal_quality->setNoiseError(noise);
    signal_quality->setPeriodError(period);
    signal_quality->setPhaseError(phase);
}
