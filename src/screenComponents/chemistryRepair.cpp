#include "chemistryRepair.h"
#include "preferenceManager.h"
#include "playerInfo.h"
#include "spaceObjects/playerSpaceship.h"

ChemistryRepairGame::ChemistryRepairGame(GuiContainer* owner, std::function<void(bool)> callback)
: GuiOverlay(owner, "CHEMISTRY_REPAIR", colorConfig.background), completion_callback(callback)
{
    current_compound_index = 0;
    current_build = "";
    puzzle_complete = false;
    
    // Main panel
    main_panel = new GuiPanel(this, "CHEMISTRY_PANEL");
    main_panel->setPosition(0, 0, sp::Alignment::Center)->setSize(1100, 600);
    
    // Title
    title_label = new GuiLabel(main_panel, "TITLE", "BEAM WEAPON CHEMISTRY REPAIR", 30);
    title_label->setPosition(0, 20, sp::Alignment::TopCenter)->setSize(0, 40);
    
    // Component name
    component_label = new GuiLabel(main_panel, "COMPONENT", "", 20);
    component_label->setPosition(0, 70, sp::Alignment::TopCenter)->setSize(0, 30);
    
    // Instructions with technical description
    instruction_label = new GuiLabel(main_panel, "INSTRUCTION", "", 14);
    instruction_label->setPosition(0, 110, sp::Alignment::TopCenter)->setSize(1000, 50);
    
    // Target formula
    target_label = new GuiLabel(main_panel, "TARGET", "", 24);
    target_label->setPosition(0, 150, sp::Alignment::TopCenter)->setSize(0, 35);
    
    // Build display
    build_display = new GuiLabel(main_panel, "BUILD", "", 28);
    build_display->setPosition(0, 210, sp::Alignment::TopCenter)->setSize(0, 40);
    
    // Status message
    status_label = new GuiLabel(main_panel, "STATUS", "", 18);
    status_label->setPosition(0, 270, sp::Alignment::TopCenter)->setSize(0, 30);
    
    // Atom buttons panel
    atom_panel = new GuiPanel(main_panel, "ATOM_PANEL");
    atom_panel->setPosition(0, 320, sp::Alignment::TopCenter)->setSize(800, 100);
    
    // Bonds label
    bond_label = new GuiLabel(main_panel, "BOND_LABEL", "Bonds", 18);
    bond_label->setPosition(0, 430, sp::Alignment::TopCenter)->setSize(0, 25);
    
    // Bond buttons panel
    bond_panel = new GuiPanel(main_panel, "BOND_PANEL");
    bond_panel->setPosition(0, 460, sp::Alignment::TopCenter)->setSize(250, 60);
    
    // Control buttons
    clear_button = new GuiButton(main_panel, "CLEAR", "CLEAR", [this]() { onClearClicked(); });
    clear_button->setPosition(-180, 520, sp::Alignment::TopCenter)->setSize(150, 50);
    
    apply_button = new GuiButton(main_panel, "APPLY", "APPLY", [this]() { onApplyClicked(); });
    apply_button->setPosition(0, 520, sp::Alignment::TopCenter)->setSize(150, 50);
    
    close_button = new GuiButton(main_panel, "CLOSE", "CANCEL", [this]() {
        if (puzzle_complete && completion_callback) {
            completion_callback(true);
        } else if (completion_callback) {
            completion_callback(false);
        }
        destroy();
    });
    close_button->setPosition(180, 520, sp::Alignment::TopCenter)->setSize(150, 50);
    
    initializeSequence();
    setupAtomButtons();
    setupBondButtons();
    updateDisplay();
}

void ChemistryRepairGame::initializeSequence()
{
    // Define component repair sequences with technical descriptions
    std::vector<std::vector<Compound>> component_recipes = {
        {
            // Focusing Lens - optical cleaning and oxidizer
            {"Focusing Lens", "H-O-H", "Water", "Clean optical surfaces with polar solvent"},
            {"Focusing Lens", "H-O-O-H", "Hydrogen Peroxide", "Apply oxidizing bleach to remove carbon scoring"}
        },
        {
            // Emitter Crystal - atmospheric purge and inert shielding
            {"Emitter Crystal", "O=O", "Oxygen", "Purge crystal chamber with reactive gas"},
            {"Emitter Crystal", "O=C=O", "Carbon Dioxide", "Establish inert atmosphere to prevent oxidation"}
        },
        {
            // Power Coupling - ionic conductor and coolant
            {"Power Coupling", "Na-Cl", "Sodium Chloride", "Restore ionic conductivity in plasma conduits"},
            {"Power Coupling", "N-H-H-H", "Ammonia", "Flash-freeze superconductor with cryogenic ammonia"}
        },
        {
            // Cooling System - thermal transfer and phase change
            {"Cooling System", "H-O-H", "Water", "Replenish primary coolant in heat exchangers"},
            {"Cooling System", "O=C=O", "Carbon Dioxide", "Pressurize secondary loop for phase-change cooling"}
        },
        {
            // Beam Modulator - frequency stabilizer and reactive flush
            {"Beam Modulator", "H-O-O-H", "Hydrogen Peroxide", "Stabilize resonance frequency with peroxide solution"},
            {"Beam Modulator", "O=O", "Oxygen", "Flush modulator pathways with pure oxygen"}
        }
    };
    
    // Choose random component
    int component_choice = rand() % component_recipes.size();
    sequence = component_recipes[component_choice];
}

void ChemistryRepairGame::setupAtomButtons()
{
    std::vector<string> atoms = {"H", "O", "C", "N", "Na", "Cl"};
    
    int button_width = 90;
    int button_height = 80;
    int spacing = 20;
    int total_width = atoms.size() * button_width + (atoms.size() - 1) * spacing;
    int start_x = (800 - total_width) / 2;  // Center in the 800px wide panel
    
    for (size_t i = 0; i < atoms.size(); i++) {
        string atom = atoms[i];
        int x_pos = start_x + i * (button_width + spacing);
        
        GuiButton* btn = new GuiButton(atom_panel, "ATOM_" + atom, atom, [this, atom]() {
            onAtomClicked(atom);
        });
        btn->setPosition(x_pos, 10, sp::Alignment::TopLeft)->setSize(button_width, button_height);
    }
}

void ChemistryRepairGame::setupBondButtons()
{
    GuiButton* single_bond = new GuiButton(bond_panel, "BOND_SINGLE", "-", [this]() {
        onBondClicked("-");
    });
    single_bond->setPosition(25, 10, sp::Alignment::TopLeft)->setSize(90, 50);
    
    GuiButton* double_bond = new GuiButton(bond_panel, "BOND_DOUBLE", "=", [this]() {
        onBondClicked("=");
    });
    double_bond->setPosition(135, 10, sp::Alignment::TopLeft)->setSize(90, 50);
}

void ChemistryRepairGame::onAtomClicked(string atom)
{
    if (puzzle_complete) return;
    current_build += atom;
    updateDisplay();
}

void ChemistryRepairGame::onBondClicked(string bond)
{
    if (puzzle_complete) return;
    current_build += bond;
    updateDisplay();
}

void ChemistryRepairGame::onClearClicked()
{
    current_build = "";
    status_label->setText("");
    updateDisplay();
}

void ChemistryRepairGame::onApplyClicked()
{
    if (puzzle_complete) return;
    
    const Compound& target = sequence[current_compound_index];
    
    if (validateFormula(current_build, target.formula)) {
        // Correct!
        status_label->setText("Correct! " + target.display_name + " synthesized.");
        current_compound_index++;
        
        if (current_compound_index >= (int)sequence.size()) {
            // All compounds complete!
            puzzle_complete = true;
            status_label->setText("SUCCESS! Beam weapon chemistry restored.");
            apply_button->setText("SUCCESS");
            apply_button->setEnable(false);
            
            // Change close button to activate
            close_button->setText("ACTIVATE REPAIR");
            close_button->setEnable(true);
            
            // Completion callback happens when activate is pressed (already wired in close_button)
        } else {
            // Move to next compound
            current_build = "";
            status_label->setText("");  // Clear status for next compound
            updateDisplay();
        }
    } else {
        // Incorrect
        status_label->setText("Incorrect formula. Try again!");
    }
}

void ChemistryRepairGame::updateDisplay()
{
    if (current_compound_index < (int)sequence.size()) {
        const Compound& target = sequence[current_compound_index];
        
        component_label->setText("Component: " + target.component_name);
        target_label->setText("Target: " + target.display_name);
        build_display->setText("Build: " + (current_build.empty() ? "(empty)" : current_build));
        
        // Show progress and technical description
        instruction_label->setText("Compound " + string(current_compound_index + 1) + " of " + string((int)sequence.size()) + ": " + target.technical_description);
    }
}

bool ChemistryRepairGame::validateFormula(const string& built, const string& target)
{
    // Simple string comparison - formulas must match exactly
    return built == target;
}
