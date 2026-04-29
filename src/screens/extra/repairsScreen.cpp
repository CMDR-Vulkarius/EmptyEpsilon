#include "repairsScreen.h"
#include "playerInfo.h"
#include "spaceObjects/playerSpaceship.h"
#include "gui/gui2_panel.h"
#include "gui/gui2_label.h"
#include "gui/gui2_element.h"
#include "gui/gui2_progressbar.h"
#include "gui/gui2_button.h"
#include "screenComponents/pipeRouting.h"
#include "screenComponents/sequenceCalibration.h"
#include "screenComponents/chemistryRepair.h"
#include "screenComponents/shieldEmitterCalibration.h"
#include "screenComponents/targetingComputerCalibration.h"

RepairsScreen::RepairsScreen(GuiContainer* owner)
: GuiOverlay(owner, "REPAIRS_SCREEN", colorConfig.background)
{
    // Main panel
    main_panel = new GuiPanel(this, "REPAIRS_MAIN_PANEL");
    main_panel->setPosition(0, 0, sp::Alignment::Center)->setSize(1200, 800);
    
    // Title
    title_label = new GuiLabel(main_panel, "REPAIRS_TITLE", tr("REPAIRS CONSOLE"), 30);
    title_label->setSize(GuiElement::GuiSizeMax, 50)->setPosition(0, 20, sp::Alignment::TopCenter);
    
    // Ship status
    ship_status_label = new GuiLabel(main_panel, "REPAIRS_SHIP_STATUS", "", 20);
    ship_status_label->setSize(GuiElement::GuiSizeMax, 30)->setPosition(0, 70, sp::Alignment::TopCenter);
    
    // System list with vertical layout
    system_list_layout = new GuiElement(main_panel, "REPAIRS_SYSTEM_LIST");
    system_list_layout->setPosition(20, 110, sp::Alignment::TopLeft)->setSize(1160, 650)->setAttribute("layout", "vertical");
    
    createSystemList();
}

void RepairsScreen::createSystemList()
{
    // Create display for each system
    for (int n = 0; n < SYS_COUNT; n++)
    {
        createSystemDisplay(static_cast<ESystem>(n));
    }
}

void RepairsScreen::createSystemDisplay(ESystem system)
{
    SystemDisplay& display = system_displays[system];
    
    // Panel for this system
    display.panel = new GuiPanel(system_list_layout, "");
    display.panel->setSize(GuiElement::GuiSizeMax, 70);
    
    // System name
    display.name_label = new GuiLabel(display.panel, "", getLocaleSystemName(system), 22);
    display.name_label->setPosition(20, 10, sp::Alignment::TopLeft);
    display.name_label->setSize(250, 25);
    
    // Health percentage
    display.health_label = new GuiLabel(display.panel, "", "100%", 22);
    display.health_label->setPosition(280, 10, sp::Alignment::TopLeft);
    display.health_label->setSize(80, 25);
    
    // Health bar
    display.health_bar = new GuiProgressbar(display.panel, "", 0.0, 1.0, 1.0);
    display.health_bar->setPosition(370, 12, sp::Alignment::TopLeft);
    display.health_bar->setSize(400, 20);
    display.health_bar->setColor(glm::u8vec4(64, 128, 64, 192)); // Green
    
    // Status text
    display.status_label = new GuiLabel(display.panel, "", tr("System operational"), 18);
    display.status_label->setPosition(20, 40, sp::Alignment::TopLeft);
    display.status_label->setSize(750, 20);
    
    // REPAIR button
    display.repair_button = new GuiButton(display.panel, "", tr("REPAIR"), [this, system]() {
        onRepairButtonClicked(system);
    });
    display.repair_button->setPosition(-20, 15, sp::Alignment::TopRight);
    display.repair_button->setSize(150, 40);
    display.repair_button->setVisible(false); // Hidden until system is damaged
}

void RepairsScreen::updateSystemDisplay(ESystem system)
{
    if (!my_spaceship) return;
    if (!my_spaceship->hasSystem(system)) return;
    
    SystemDisplay& display = system_displays[system];
    
    float health = my_spaceship->getSystemHealth(system);
    float health_max = my_spaceship->getSystemHealthMax(system);
    
    if (health_max <= 0.0f) return; // Invalid system
    
    // Update health percentage (show absolute health, not relative to reduced max)
    float health_percent = health * 100.0f;  // Absolute percentage
    display.health_label->setText(string(int(health_percent)) + "%");
    
    // Update health bar (show relative to current max for visual)
    float health_ratio = health / health_max;
    display.health_bar->setValue(health_ratio);
    
    // Color code health bar
    if (health_ratio < 0.25f) {
        display.health_bar->setColor(glm::u8vec4(255, 0, 0, 255)); // Red
    } else if (health_ratio < 0.50f) {
        display.health_bar->setColor(glm::u8vec4(255, 165, 0, 255)); // Orange
    } else if (health_ratio < 1.0f) {
        display.health_bar->setColor(glm::u8vec4(255, 255, 0, 255)); // Yellow
    } else {
        display.health_bar->setColor(glm::u8vec4(64, 255, 64, 255)); // Bright green
    }
    
    // Update status and repair button visibility
    if (health >= health_max) {
        // System is at maximum health
        if (health_max < 1.0f) {
            // Maximum capacity is reduced due to permanent damage
            int max_percent = int(health_max * 100.0f);
            display.status_label->setText(tr("Operational - Max capacity: ") + string(max_percent) + "%");
        } else {
            display.status_label->setText(tr("System operational"));
        }
        display.repair_button->setVisible(false);
    } else {
        // Check if repair boost is active
        if (my_spaceship->systems[system].repair_boost_end_time > 0.0f) {
            int seconds_remaining = int(my_spaceship->systems[system].repair_boost_end_time);
            display.status_label->setText(tr("RAPID REPAIR ACTIVE - ") + string(seconds_remaining) + tr(" seconds"));
            display.repair_button->setVisible(false);
        } else {
            display.status_label->setText(tr("Damaged - requires repair"));
            display.repair_button->setVisible(true);
            display.repair_button->enable();
        }
    }
}

void RepairsScreen::onRepairButtonClicked(ESystem system)
{
    LOG(INFO) << "Launching repair mini-game for system: " << getSystemName(system);
    
    // Use sequence calibration for Jump Drive and Warp Drive
    if (system == SYS_JumpDrive || system == SYS_Warp)
    {
        new SequenceCalibrationGame(this, [this, system](bool success) {
            if (success)
            {
                LOG(INFO) << "Calibration completed successfully for " << getSystemName(system);
                // Activate repair boost via command
                if (my_spaceship)
                    my_spaceship->commandBoostSystemRepair(system);
            }
            else
            {
                LOG(INFO) << "Calibration closed without completion for " << getSystemName(system);
            }
        });
    }
    // Use targeting computer calibration for Missile System
    else if (system == SYS_MissileSystem)
    {
        new TargetingComputerCalibrationGame(this, [this, system](bool success) {
            if (success)
            {
                LOG(INFO) << "Targeting computer calibration completed successfully for " << getSystemName(system);
                // Activate repair boost via command
                if (my_spaceship)
                    my_spaceship->commandBoostSystemRepair(system);
            }
            else
            {
                LOG(INFO) << "Targeting computer calibration closed without completion for " << getSystemName(system);
            }
        });
    }
    // Use chemistry repair for Beam Weapons
    else if (system == SYS_BeamWeapons)
    {
        new ChemistryRepairGame(this, [this, system](bool success) {
            if (success)
            {
                LOG(INFO) << "Chemistry repair completed successfully for " << getSystemName(system);
                // Activate repair boost via command
                if (my_spaceship)
                    my_spaceship->commandBoostSystemRepair(system);
            }
            else
            {
                LOG(INFO) << "Chemistry repair closed without completion for " << getSystemName(system);
            }
        });
    }
    // Use shield emitter calibration for Front Shield
    else if (system == SYS_FrontShield)
    {
        new ShieldEmitterCalibrationGame(this, ShieldEmitterCalibrationGame::ShieldSide::Front, [this, system](bool success) {
            if (success)
            {
                LOG(INFO) << "Shield emitter calibration completed successfully for " << getSystemName(system);
                // Activate repair boost via command
                if (my_spaceship)
                    my_spaceship->commandBoostSystemRepair(system);
            }
            else
            {
                LOG(INFO) << "Shield emitter calibration closed without completion for " << getSystemName(system);
            }
        });
    }
    // Use shield emitter calibration for Rear Shield
    else if (system == SYS_RearShield)
    {
        new ShieldEmitterCalibrationGame(this, ShieldEmitterCalibrationGame::ShieldSide::Rear, [this, system](bool success) {
            if (success)
            {
                LOG(INFO) << "Shield emitter calibration completed successfully for " << getSystemName(system);
                // Activate repair boost via command
                if (my_spaceship)
                    my_spaceship->commandBoostSystemRepair(system);
            }
            else
            {
                LOG(INFO) << "Shield emitter calibration closed without completion for " << getSystemName(system);
            }
        });
    }
    else
    {
        // Use pipe routing for all other systems
        new PipeRoutingGame(this, [this, system](bool success) {
            if (success)
            {
                LOG(INFO) << "Mini-game completed successfully for " << getSystemName(system);
                // Activate repair boost via command
                if (my_spaceship)
                    my_spaceship->commandBoostSystemRepair(system);
            }
            else
            {
                LOG(INFO) << "Mini-game closed without completion for " << getSystemName(system);
            }
        });
    }
}

void RepairsScreen::onDraw(sp::RenderTarget& renderer)
{
    // Update ship status label
    if (my_spaceship)
    {
        ship_status_label->setText(my_spaceship->getTypeName() + 
                                   "   Hull: " + string(int(my_spaceship->hull_strength)) + "/" + string(int(my_spaceship->hull_max)) +
                                   "   Energy: " + string(int(my_spaceship->energy_level)) + "/" + string(int(my_spaceship->max_energy_level)));
        
        // Update all system displays
        for (int n = 0; n < SYS_COUNT; n++)
        {
            ESystem system = static_cast<ESystem>(n);
            SystemDisplay& display = system_displays[system];
            
            if (my_spaceship->hasSystem(system))
            {
                display.panel->setVisible(true);
                updateSystemDisplay(system);
            }
            else
            {
                display.panel->setVisible(false);
            }
        }
    }
    
    GuiOverlay::onDraw(renderer);
}
