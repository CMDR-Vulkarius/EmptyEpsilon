#include "targetingComputerCalibration.h"
#include "engine.h"
#include <cmath>
#include <cstdlib>
#include <vector>

TargetingComputerCalibrationGame::TargetingComputerCalibrationGame(GuiContainer* owner, CompletionCallback callback)
: GuiOverlay(owner, "TARGETING_COMPUTER_CALIBRATION", glm::u8vec4(0, 0, 0, 128)),
  completion_callback(callback),
  puzzle_complete(false),
  alignment_tolerance(0.12f),  // 12% of radar radius for easier alignment
  lock_tolerance(1.5f)         // ±1.5 units for slider precision
{
    initializeStage1();
    initializeStage2();
    
    // Stage 2 starts hidden
    stage2_panel->hide();
}

void TargetingComputerCalibrationGame::initializeStage1()
{
    // Main panel for Stage 1
    stage1_panel = new GuiPanel(this, "STAGE1_PANEL");
    stage1_panel->setPosition(0, 0, sp::Alignment::Center)->setSize(600, 650);
    
    // Title
    GuiLabel* title = new GuiLabel(stage1_panel, "TITLE", "TARGETING COMPUTER CALIBRATION", 30);
    title->setPosition(0, 20, sp::Alignment::TopCenter)->setSize(GuiElement::GuiSizeMax, 40);
    
    GuiLabel* subtitle = new GuiLabel(stage1_panel, "SUBTITLE", "STAGE 1: RETICLE ALIGNMENT", 20);
    subtitle->setPosition(0, 65, sp::Alignment::TopCenter)->setSize(GuiElement::GuiSizeMax, 30);
    
    // Radar display (400x400)
    radar_display = new RadarDisplayElement(stage1_panel, "RADAR", reticles);
    radar_display->setPosition(0, 110, sp::Alignment::TopCenter)->setSize(400, 400);
    
    // Initialize reticle positions
    for (int i = 0; i < num_reticles; i++) {
        // Random target positions in circular area
        float angle = (360.0f / num_reticles) * i + (rand() % 40 - 20);
        float distance = 0.5f + (rand() % 40) / 100.0f;  // 0.5 to 0.9
        
        reticles[i].target_position.x = std::cos(glm::radians(angle)) * distance;
        reticles[i].target_position.y = std::sin(glm::radians(angle)) * distance;
        
        // Random starting positions (away from target)
        float start_angle = angle + 120.0f + (rand() % 120);
        float start_distance = 0.4f + (rand() % 50) / 100.0f;
        reticles[i].position.x = std::cos(glm::radians(start_angle)) * start_distance;
        reticles[i].position.y = std::sin(glm::radians(start_angle)) * start_distance;
        
        // Create draggable reticle element
        // Radar display is 400x400, so local center is (200, 200)
        glm::vec2 radar_local_center(200.0f, 200.0f);
        float radar_radius = 180.0f;  // Display radius for reticles
        
        reticles[i].element = new DraggableReticle(radar_display, "RETICLE_" + std::to_string(i),
                                                    reticles[i], radar_radius, radar_local_center);
        reticles[i].element->setSize(30, 30);  // Clickable area
        reticles[i].element->setMoveCallback([this]() { onReticleMoved(); });
        reticles[i].element->updateVisualPosition();
    }
    
    // Status label
    alignment_status_label = new GuiLabel(stage1_panel, "STATUS", "Align all reticles to target markers", 18);
    alignment_status_label->setPosition(0, 520, sp::Alignment::TopCenter)->setSize(GuiElement::GuiSizeMax, 30);
    
    // Continue button (starts disabled)
    stage1_continue_button = new GuiButton(stage1_panel, "CONTINUE", "CONTINUE", [this]() {
        unlockStage2();
    });
    stage1_continue_button->setPosition(0, 555, sp::Alignment::TopCenter)->setSize(200, 40);
    stage1_continue_button->disable();
    
    // Cancel button
    cancel_button = new GuiButton(stage1_panel, "CANCEL", "CANCEL", [this]() {
        if (completion_callback) completion_callback(false);
        destroy();
    });
    cancel_button->setPosition(0, 600, sp::Alignment::TopCenter)->setSize(200, 40);
}

void TargetingComputerCalibrationGame::initializeStage2()
{
    // Main panel for Stage 2
    stage2_panel = new GuiPanel(this, "STAGE2_PANEL");
    stage2_panel->setPosition(0, 0, sp::Alignment::Center)->setSize(600, 550);
    
    // Title
    GuiLabel* title = new GuiLabel(stage2_panel, "TITLE", "TARGETING COMPUTER CALIBRATION", 30);
    title->setPosition(0, 20, sp::Alignment::TopCenter)->setSize(GuiElement::GuiSizeMax, 40);
    
    GuiLabel* subtitle = new GuiLabel(stage2_panel, "SUBTITLE", "STAGE 2: TARGETING SOLUTION LOCK", 20);
    subtitle->setPosition(0, 65, sp::Alignment::TopCenter)->setSize(GuiElement::GuiSizeMax, 30);
    
    GuiLabel* instructions = new GuiLabel(stage2_panel, "INSTRUCTIONS", 
        "Adjust each slider to match the TARGET value, then click LOCK", 16);
    instructions->setPosition(0, 100, sp::Alignment::TopCenter)->setSize(GuiElement::GuiSizeMax, 25);
    
    // Initialize targeting parameters
    const string param_names[num_parameters] = {
        "RANGE (km)",
        "BEARING (deg)",
        "INTERCEPT (sec)",
        "VELOCITY (km/s)"
    };
    
    // Target values and ranges for each parameter
    float target_values[num_parameters] = {
        42.5f,   // Range
        137.0f,  // Bearing
        8.2f,    // Intercept time
        2.8f     // Velocity
    };
    
    float min_values[num_parameters] = {
        20.0f,   // Range min
        0.0f,    // Bearing min
        2.0f,    // Intercept min
        0.5f     // Velocity min
    };
    
    float max_values[num_parameters] = {
        80.0f,   // Range max
        360.0f,  // Bearing max
        15.0f,   // Intercept max
        5.0f     // Velocity max
    };
    
    for (int i = 0; i < num_parameters; i++) {
        parameters[i].name = param_names[i];
        parameters[i].target_value = target_values[i];
        parameters[i].min_value = min_values[i];
        parameters[i].max_value = max_values[i];
        parameters[i].locked = false;
        
        int y_offset = 140 + (i * 85);
        
        // Parameter name label
        GuiLabel* name_label = new GuiLabel(stage2_panel, "PARAM_NAME_" + std::to_string(i),
                                            param_names[i], 18);
        name_label->setPosition(50, y_offset, sp::Alignment::TopLeft)->setSize(200, 25);
        
        // TARGET value label (shows what to aim for)
        char target_buffer[32];
        snprintf(target_buffer, sizeof(target_buffer), "TARGET: %.1f", double(target_values[i]));
        parameters[i].target_label = new GuiLabel(stage2_panel, "TARGET_" + std::to_string(i),
                                                  target_buffer, 16);
        parameters[i].target_label->setPosition(50, y_offset + 25, sp::Alignment::TopLeft)->setSize(200, 25);
        
        // Current value label (shows slider value)
        parameters[i].value_label = new GuiLabel(stage2_panel, "PARAM_VALUE_" + std::to_string(i),
                                                 "---", 24);
        parameters[i].value_label->setPosition(50, y_offset + 50, sp::Alignment::TopLeft)->setSize(200, 30);
        
        // Slider - player adjusts this to match target
        // Start at a random position away from target
        float start_value = min_values[i] + (rand() % 100) / 100.0f * (max_values[i] - min_values[i]);
        parameters[i].slider = new GuiSlider(stage2_panel, "SLIDER_" + std::to_string(i),
                                            min_values[i], max_values[i], start_value,
                                            [this, i](float value) {
                                                // Update value label as slider moves
                                                updateTargetingSolution();
                                            });
        parameters[i].slider->setPosition(260, y_offset + 10, sp::Alignment::TopLeft)->setSize(200, 50);
        
        // Lock button - enabled when slider is close to target
        parameters[i].lock_button = new GuiButton(stage2_panel, "LOCK_" + std::to_string(i), "LOCK", 
            [this, i]() {
                if (parameters[i].locked) return;  // Already locked
                
                // Check if slider value is close enough to target
                float slider_value = parameters[i].slider->getValue();
                float error = std::abs(slider_value - parameters[i].target_value);
                if (error < lock_tolerance) {
                    // SUCCESS! Locked at correct value
                    parameters[i].locked = true;
                    parameters[i].slider->disable();
                    parameters[i].lock_button->setText("LOCKED");
                    parameters[i].lock_button->disable();
                    updateTargetingSolution();
                }
            });
        parameters[i].lock_button->setPosition(475, y_offset + 10, sp::Alignment::TopLeft)->setSize(90, 50);
    }
    
    // Status label
    solution_status_label = new GuiLabel(stage2_panel, "SOLUTION_STATUS", 
        "0 / " + std::to_string(num_parameters) + " parameters locked", 18);
    solution_status_label->setPosition(0, 480, sp::Alignment::TopCenter)->setSize(GuiElement::GuiSizeMax, 30);
    
    // Complete button (hidden until all locked)
    stage2_complete_button = new GuiButton(stage2_panel, "COMPLETE", "ACTIVATE TARGETING SYSTEM", 
        [this]() {
            if (completion_callback) completion_callback(true);
            destroy();
        });
    stage2_complete_button->setPosition(0, -20, sp::Alignment::BottomCenter)->setSize(300, 50);
    stage2_complete_button->hide();
}

void TargetingComputerCalibrationGame::unlockStage2()
{
    stage1_panel->hide();
    stage2_panel->show();
}

void TargetingComputerCalibrationGame::onReticleMoved()
{
    updateReticleAlignment();
}

void TargetingComputerCalibrationGame::updateReticleAlignment()
{
    int aligned_count = 0;
    std::vector<bool> target_claimed(num_reticles, false);
    
    // For each reticle, check if it's aligned with ANY target
    for (int i = 0; i < num_reticles; i++) {
        bool reticle_aligned = false;
        
        // Check distance to each target marker
        for (int t = 0; t < num_reticles; t++) {
            if (target_claimed[t]) continue;  // Skip already claimed targets
            
            float dx = reticles[i].position.x - reticles[t].target_position.x;
            float dy = reticles[i].position.y - reticles[t].target_position.y;
            float distance = std::sqrt(dx * dx + dy * dy);
            
            if (distance < alignment_tolerance) {
                aligned_count++;
                target_claimed[t] = true;  // Mark this target as claimed
                reticle_aligned = true;
                break;  // This reticle is aligned, move to next
            }
        }
    }
    
    // Update status
    string status = std::to_string(aligned_count) + " / " + std::to_string(num_reticles) + " reticles aligned";
    alignment_status_label->setText(status);
    
    // Enable continue button when all aligned
    if (aligned_count == num_reticles) {
        stage1_continue_button->enable();
        alignment_status_label->setText("ALL RETICLES ALIGNED - READY TO CONTINUE");
    } else {
        stage1_continue_button->disable();
    }
}

void TargetingComputerCalibrationGame::updateTargetingSolution()
{
    int locked_count = 0;
    
    for (int i = 0; i < num_parameters; i++) {
        if (!parameters[i].locked) {
            // Get current slider value
            float slider_value = parameters[i].slider->getValue();
            float error = std::abs(slider_value - parameters[i].target_value);
            
            // Update value label to show current slider position
            char buffer[32];
            if (error < lock_tolerance) {
                // Close enough - show brackets to indicate ready to lock
                snprintf(buffer, sizeof(buffer), "[%.1f]", double(slider_value));
            } else {
                snprintf(buffer, sizeof(buffer), "%.1f", double(slider_value));
            }
            parameters[i].value_label->setText(buffer);
            
            // Enable/disable lock button based on proximity to target
            if (error < lock_tolerance) {
                parameters[i].lock_button->enable();
            } else {
                parameters[i].lock_button->disable();
            }
        } else {
            locked_count++;
            // Show locked value
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%.1f [LOCKED]", double(parameters[i].target_value));
            parameters[i].value_label->setText(buffer);
        }
    }
    
    // Update status
    solution_status_label->setText(std::to_string(locked_count) + " / " + 
                                   std::to_string(num_parameters) + " parameters locked");
    
    // Show complete button when all locked
    if (locked_count == num_parameters) {
        solution_status_label->setText("TARGETING SOLUTION LOCKED - SYSTEM READY");
        stage2_complete_button->show();
        cancel_button->hide();
    }
}

bool TargetingComputerCalibrationGame::checkStage1Complete()
{
    for (int i = 0; i < num_reticles; i++) {
        float dx = reticles[i].position.x - reticles[i].target_position.x;
        float dy = reticles[i].position.y - reticles[i].target_position.y;
        float distance = std::sqrt(dx * dx + dy * dy);
        
        if (distance >= alignment_tolerance) {
            return false;
        }
    }
    return true;
}

bool TargetingComputerCalibrationGame::checkStage2Complete()
{
    for (int i = 0; i < num_parameters; i++) {
        if (!parameters[i].locked) {
            return false;
        }
    }
    return true;
}

void TargetingComputerCalibrationGame::onUpdate()
{
    // Update targeting solution display in Stage 2
    if (stage2_panel && stage2_panel->isVisible()) {
        updateTargetingSolution();
    }
}

// ========== RadarDisplayElement Implementation ==========

RadarDisplayElement::RadarDisplayElement(GuiContainer* owner, string id,
    std::array<TargetingComputerCalibrationGame::Reticle, TargetingComputerCalibrationGame::num_reticles>& reticles)
: GuiElement(owner, id), reticles(reticles)
{
}

void RadarDisplayElement::onDraw(sp::RenderTarget& renderer)
{
    glm::vec2 center = getCenterPoint();
    float radius = 180.0f;
    
    drawRadarBackground(renderer, center, radius);
    
    // Draw target markers
    for (int i = 0; i < TargetingComputerCalibrationGame::num_reticles; i++) {
        drawTargetMarker(renderer, center, radius, reticles[i].target_position);
    }
}

void RadarDisplayElement::drawRadarBackground(sp::RenderTarget& renderer, glm::vec2 center, float radius)
{
    // Outer circle
    renderer.drawCircleOutline(center, radius, 2.0f, glm::u8vec4(100, 255, 100, 255));
    
    // Range rings
    for (int i = 1; i <= 3; i++) {
        float ring_radius = radius * (i / 4.0f);
        renderer.drawCircleOutline(center, ring_radius, 1.0f, glm::u8vec4(100, 255, 100, 100));
    }
    
    // Crosshairs
    renderer.fillRect(sp::Rect(center.x - radius, center.y - 1, radius * 2, 2), glm::u8vec4(100, 255, 100, 100));
    renderer.fillRect(sp::Rect(center.x - 1, center.y - radius, 2, radius * 2), glm::u8vec4(100, 255, 100, 100));
    
    // Diagonal crosshairs
    for (float angle = 45.0f; angle < 360.0f; angle += 90.0f) {
        float rad = glm::radians(angle);
        glm::vec2 end = center + glm::vec2(std::cos(rad), std::sin(rad)) * radius;
        renderer.drawLine(center, end, glm::u8vec4(100, 255, 100, 50));
    }
    
    // Center dot
    renderer.fillCircle(center, 5.0f, glm::u8vec4(255, 100, 100, 255));
}

void RadarDisplayElement::drawTargetMarker(sp::RenderTarget& renderer, glm::vec2 center, 
                                          float radius, glm::vec2 target_pos)
{
    glm::vec2 screen_pos = center + target_pos * radius;
    
    // Draw target rings (concentric circles)
    renderer.drawCircleOutline(screen_pos, 12.0f, 2.0f, glm::u8vec4(255, 255, 100, 200));
    renderer.drawCircleOutline(screen_pos, 8.0f, 1.5f, glm::u8vec4(255, 255, 100, 150));
    renderer.drawCircleOutline(screen_pos, 4.0f, 1.0f, glm::u8vec4(255, 255, 100, 100));
}

// ========== DraggableReticle Implementation ==========

DraggableReticle::DraggableReticle(GuiContainer* owner, string id,
    TargetingComputerCalibrationGame::Reticle& reticle,
    float radar_radius, glm::vec2 radar_center)
: GuiElement(owner, id), reticle(reticle), radar_radius(radar_radius), 
  radar_center(radar_center), dragging(false), drag_offset(0.0f, 0.0f)
{
}

void DraggableReticle::onDraw(sp::RenderTarget& renderer)
{
    glm::vec2 center = getCenterPoint();
    drawReticle(renderer, center);
}

void DraggableReticle::drawReticle(sp::RenderTarget& renderer, glm::vec2 position)
{
    // Crosshair reticle
    glm::u8vec4 color = dragging ? glm::u8vec4(255, 100, 100, 255) : glm::u8vec4(100, 200, 255, 255);
    
    // Horizontal line
    renderer.fillRect(sp::Rect(position.x - 12, position.y - 1, 24, 2), color);
    // Vertical line
    renderer.fillRect(sp::Rect(position.x - 1, position.y - 12, 2, 24), color);
    
    // Corner brackets
    float bracket_size = 8.0f;
    float bracket_thickness = 2.0f;
    
    // Top-left
    renderer.fillRect(sp::Rect(position.x - 12, position.y - 12, bracket_size, bracket_thickness), color);
    renderer.fillRect(sp::Rect(position.x - 12, position.y - 12, bracket_thickness, bracket_size), color);
    
    // Top-right
    renderer.fillRect(sp::Rect(position.x + 12 - bracket_size, position.y - 12, bracket_size, bracket_thickness), color);
    renderer.fillRect(sp::Rect(position.x + 12 - bracket_thickness, position.y - 12, bracket_thickness, bracket_size), color);
    
    // Bottom-left
    renderer.fillRect(sp::Rect(position.x - 12, position.y + 12 - bracket_thickness, bracket_size, bracket_thickness), color);
    renderer.fillRect(sp::Rect(position.x - 12, position.y + 12 - bracket_size, bracket_thickness, bracket_size), color);
    
    // Bottom-right
    renderer.fillRect(sp::Rect(position.x + 12 - bracket_size, position.y + 12 - bracket_thickness, bracket_size, bracket_thickness), color);
    renderer.fillRect(sp::Rect(position.x + 12 - bracket_thickness, position.y + 12 - bracket_size, bracket_thickness, bracket_size), color);
    
    // Center dot
    renderer.fillCircle(position, 3.0f, color);
}

bool DraggableReticle::onMouseDown(sp::io::Pointer::Button button, glm::vec2 position, sp::io::Pointer::ID id)
{
    dragging = true;
    // Calculate current reticle screen position
    glm::vec2 current_pos = radar_center + reticle.position * radar_radius;
    // Store offset from where user clicked to reticle center
    drag_offset = current_pos - position;
    return true;
}

void DraggableReticle::onMouseDrag(glm::vec2 position, sp::io::Pointer::ID id)
{
    if (dragging) {
        // Apply drag offset so reticle doesn't jump
        glm::vec2 target_pos = position + drag_offset;
        
        // Convert to normalized radar coordinates (-1 to 1)
        glm::vec2 offset = target_pos - radar_center;
        reticle.position = offset / radar_radius;
        
        // Clamp to circle (keep within radar)
        float distance = std::sqrt(reticle.position.x * reticle.position.x + 
                                  reticle.position.y * reticle.position.y);
        if (distance > 0.95f) {
            reticle.position = reticle.position / distance * 0.95f;
        }
        
        updateVisualPosition();
        
        if (move_callback) {
            move_callback();
        }
    }
}

void DraggableReticle::onMouseUp(glm::vec2 position, sp::io::Pointer::ID id)
{
    dragging = false;
}

void DraggableReticle::updateVisualPosition()
{
    // Update element position based on reticle normalized position
    // radar_center is in local coordinates, reticle.position is normalized (-1 to 1)
    glm::vec2 local_pos = radar_center + reticle.position * radar_radius;
    // Center the element (subtract half its size: 30/2 = 15)
    setPosition(local_pos.x - 15.0f, local_pos.y - 15.0f, sp::Alignment::TopLeft);
}
