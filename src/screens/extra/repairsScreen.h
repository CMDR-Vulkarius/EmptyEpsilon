#ifndef REPAIRS_SCREEN_H
#define REPAIRS_SCREEN_H

#include "gui/gui2_overlay.h"
#include "spaceObjects/playerSpaceship.h"

class GuiPanel;
class GuiLabel;
class GuiElement;
class GuiProgressbar;
class GuiButton;

class RepairsScreen : public GuiOverlay
{
public:
    RepairsScreen(GuiContainer* owner);
    virtual ~RepairsScreen() = default;

    virtual void onDraw(sp::RenderTarget& renderer) override;

private:
    // Main UI components
    GuiPanel* main_panel;
    GuiLabel* title_label;
    GuiLabel* ship_status_label;
    GuiElement* system_list_layout;
    
    // Per-system display structure
    struct SystemDisplay {
        GuiPanel* panel;
        GuiLabel* name_label;
        GuiLabel* health_label;
        GuiProgressbar* health_bar;
        GuiLabel* status_label;
        GuiButton* repair_button;
    };
    std::array<SystemDisplay, SYS_COUNT> system_displays;
    
    // Methods
    void createSystemList();
    void createSystemDisplay(ESystem system);
    void updateSystemDisplay(ESystem system);
    void onRepairButtonClicked(ESystem system);
};

#endif//REPAIRS_SCREEN_H
