#ifndef CHEMISTRY_REPAIR_H
#define CHEMISTRY_REPAIR_H

#include "gui/gui2_overlay.h"
#include "gui/gui2_panel.h"
#include "gui/gui2_button.h"
#include "gui/gui2_label.h"
#include <functional>

class ChemistryRepairGame : public GuiOverlay
{
private:
    GuiPanel* main_panel;
    GuiLabel* title_label;
    GuiLabel* component_label;
    GuiLabel* instruction_label;
    GuiLabel* target_label;
    GuiLabel* build_display;
    GuiLabel* status_label;
    GuiLabel* bond_label;
    
    GuiPanel* atom_panel;
    GuiPanel* bond_panel;
    GuiButton* clear_button;
    GuiButton* apply_button;
    GuiButton* close_button;
    
    std::function<void(bool)> completion_callback;
    
    struct Compound {
        string component_name;
        string formula;
        string display_name;
        string technical_description;
    };
    
    std::vector<Compound> sequence;
    int current_compound_index;
    string current_build;
    bool puzzle_complete;
    
    void initializeSequence();
    void setupAtomButtons();
    void setupBondButtons();
    void onAtomClicked(string atom);
    void onBondClicked(string bond);
    void onClearClicked();
    void onApplyClicked();
    void updateDisplay();
    bool validateFormula(const string& built, const string& target);
    
public:
    ChemistryRepairGame(GuiContainer* owner, std::function<void(bool)> callback);
    virtual ~ChemistryRepairGame() {}
};

#endif // CHEMISTRY_REPAIR_H
