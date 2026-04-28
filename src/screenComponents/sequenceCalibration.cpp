#include "sequenceCalibration.h"
#include "preferenceManager.h"
#include "playerInfo.h"
#include "spaceObjects/playerSpaceship.h"

SequenceCalibrationGame::SequenceCalibrationGame(GuiContainer* owner, CompletionCallback callback)
: GuiOverlay(owner, "SEQUENCE_CALIBRATION", colorConfig.background), completion_callback(callback)
{
    current_sequence_index = 0;
    current_input = "";
    puzzle_complete = false;
    
    // Main panel
    main_panel = new GuiPanel(this, "SEQ_PANEL");
    main_panel->setPosition(0, 0, sp::Alignment::Center)->setSize(1000, 550);
    
    // Title
    title_label = new GuiLabel(main_panel, "TITLE", "DRIVE SYSTEM CALIBRATION", 30);
    title_label->setPosition(0, 20, sp::Alignment::TopCenter)->setSize(0, 40);
    
    // Progress indicator
    progress_label = new GuiLabel(main_panel, "PROGRESS", "", 18);
    progress_label->setPosition(0, 70, sp::Alignment::TopCenter)->setSize(0, 25);
    
    // Instructions/technical description
    instruction_label = new GuiLabel(main_panel, "INSTRUCTION", "", 14);
    instruction_label->setPosition(0, 105, sp::Alignment::TopCenter)->setSize(900, 40);
    
    // Sequence display
    sequence_display = new GuiLabel(main_panel, "SEQUENCE", "",32);
    sequence_display->setPosition(0, 170, sp::Alignment::TopCenter)->setSize(0, 50);
    
    // Input display
    input_display = new GuiLabel(main_panel, "INPUT", "", 24);
    input_display->setPosition(0, 240, sp::Alignment::TopCenter)->setSize(0, 35);
    
    // Status message
    status_label = new GuiLabel(main_panel, "STATUS", "", 18);
    status_label->setPosition(0, 290, sp::Alignment::TopCenter)->setSize(0, 30);
    
    // Numpad panel
    numpad_panel = new GuiPanel(main_panel, "NUMPAD_PANEL");
    numpad_panel->setPosition(0, 340, sp::Alignment::TopCenter)->setSize(300, 130);
    
    // Create numpad buttons (3x3 grid + 0 and -)
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            int digit = row * 3 + col + 1;
            int x_pos = 10 + col * 70;
            int y_pos = 10 + row * 38;
            
            GuiButton* btn = new GuiButton(numpad_panel, "NUM_" + string(digit), string(digit), [this, digit]() {
                onNumpadDigitClicked(digit);
            });
            btn->setPosition(x_pos, y_pos, sp::Alignment::TopLeft)->setSize(60, 33);
        }
    }
    
    // 0 button
    GuiButton* zero_btn = new GuiButton(numpad_panel, "NUM_0", "0", [this]() {
        onNumpadDigitClicked(0);
    });
    zero_btn->setPosition(10, 124, sp::Alignment::TopLeft)->setSize(60, 33);
    
    // Minus button
    GuiButton* minus_btn = new GuiButton(numpad_panel, "NUM_MINUS", "-", [this]() {
        if (current_input.empty() || current_input == "0") {
            current_input = "-";
        }
        updateDisplay();
    });
    minus_btn->setPosition(80, 124, sp::Alignment::TopLeft)->setSize(60, 33);
    
    // Control buttons
    clear_button = new GuiButton(main_panel, "CLEAR", "CLEAR", [this]() { onClearClicked(); });
    clear_button->setPosition(-160, 485, sp::Alignment::TopCenter)->setSize(130, 45);
    
    submit_button = new GuiButton(main_panel, "SUBMIT", "SUBMIT", [this]() { onSubmitClicked(); });
    submit_button->setPosition(0, 485, sp::Alignment::TopCenter)->setSize(130, 45);
    
    close_button = new GuiButton(main_panel, "CLOSE", "CANCEL", [this]() {
        if (puzzle_complete && completion_callback) {
            completion_callback(true);
        } else if (completion_callback) {
            completion_callback(false);
        }
        destroy();
    });
    close_button->setPosition(160, 485, sp::Alignment::TopCenter)->setSize(130, 45);
    
    initializePuzzle();
    updateDisplay();
}

void SequenceCalibrationGame::initializePuzzle()
{
    // Create 5 random sequences
    std::vector<SequenceType> types = {
        SequenceType::Arithmetic,
        SequenceType::Geometric,
        SequenceType::Fibonacci,
        SequenceType::Squares,
        SequenceType::Cubes,
        SequenceType::Primes
    };
    
    // Shuffle and pick 5 (or use all 6 - one gets picked twice randomly)
    for (int i = 0; i < 5; i++) {
        Sequence seq;
        int type_index = rand() % types.size();
        seq.type = types[type_index];
        
        seq.technical_description = getSequenceTypeDescription(seq.type);
        seq.blank_index = 2 + (rand() % 2); // Position 2 or 3
        
        generateSequence(seq, seq.type);
        sequences.push_back(seq);
    }
}

void SequenceCalibrationGame::generateSequence(Sequence& seq, SequenceType type)
{
    switch (type) {
        case SequenceType::Arithmetic: {
            int start = rand() % 20 + 1;
            int step = rand() % 9 + 1;
            for (int i = 0; i < 5; i++) {
                seq.values[i] = start + i * step;
            }
            break;
        }
        case SequenceType::Geometric: {
            int start = rand() % 5 + 2;
            int ratio = rand() % 3 + 2;
            for (int i = 0; i < 5; i++) {
                seq.values[i] = start * (int)pow(ratio, i);
            }
            break;
        }
        case SequenceType::Fibonacci: {
            int a = rand() % 5 + 1;
            int b = rand() % 5 + 1;
            seq.values[0] = a;
            seq.values[1] = b;
            for (int i = 2; i < 5; i++) {
                seq.values[i] = seq.values[i-1] + seq.values[i-2];
            }
            break;
        }
        case SequenceType::Squares: {
            int start = rand() % 8 + 2;
            for (int i = 0; i < 5; i++) {
                int n = start + i;
                seq.values[i] = n * n;
            }
            break;
        }
        case SequenceType::Cubes: {
            int start = rand() % 5 + 2;
            for (int i = 0; i < 5; i++) {
                int n = start + i;
                seq.values[i] = n * n * n;
            }
            break;
        }
        case SequenceType::Primes: {
            int primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47};
            int start_index = rand() % 10;
            for (int i = 0; i < 5; i++) {
                seq.values[i] = primes[start_index + i];
            }
            break;
        }
    }
    
    seq.correct_answer = seq.values[seq.blank_index];
}

std::string SequenceCalibrationGame::getSequenceTypeDescription(SequenceType type)
{
    std::vector<std::string> descriptions;
    
    switch (type) {
        case SequenceType::Arithmetic:
            descriptions = {
                "Recalibrate linear phase shift compensators",
                "Adjust stepped-frequency oscillator timing",
                "Balance sequential power distribution nodes"
            };
            break;
        case SequenceType::Geometric:
            descriptions = {
                "Tune exponential energy decay curves",
                "Synchronize multiplicative field harmonics",
                "Calibrate logarithmic sensor array spacing"
            };
            break;
        case SequenceType::Fibonacci:
            descriptions = {
                "Optimize recursive waveform interference patterns",
                "Balance natural resonance frequency cascade",
                "Align golden-ratio spatial fold geometry"
            };
            break;
        case SequenceType::Squares:
            descriptions = {
                "Calculate quadratic field strength distribution",
                "Verify inverse-square force field gradients",
                "Tune area-based power scaling matrices"
            };
            break;
        case SequenceType::Cubes:
            descriptions = {
                "Calibrate volumetric subspace compression ratios",
                "Adjust three-dimensional energy density functions",
                "Balance cubic lattice crystal alignment"
            };
            break;
        case SequenceType::Primes:
            descriptions = {
                "Synchronize indivisible quantum state frequencies",
                "Tune prime-factorization encryption buffers",
                "Calibrate fundamental harmonic resonators"
            };
            break;
    }
    
    return descriptions[rand() % descriptions.size()];
}

void SequenceCalibrationGame::onNumpadDigitClicked(int digit)
{
    if (puzzle_complete) return;
    current_input += string(digit);
    updateDisplay();
}

void SequenceCalibrationGame::onClearClicked()
{
    current_input = "";
    status_label->setText("");
    updateDisplay();
}

void SequenceCalibrationGame::onSubmitClicked()
{
    if (puzzle_complete) return;
    if (current_input.empty()) return;
    
    const Sequence& seq = sequences[current_sequence_index];
    int user_answer = std::atoi(current_input.c_str());
    
    if (user_answer == seq.correct_answer) {
        // Correct!
        status_label->setText("Correct! Sequence calibrated.");
        current_sequence_index++;
        
        if (current_sequence_index >= (int)sequences.size()) {
            // All sequences complete!
            puzzle_complete = true;
            status_label->setText("SUCCESS! Drive system fully calibrated.");
            submit_button->setText("SUCCESS");
            submit_button->setEnable(false);
            
            // Change close button to activate
            close_button->setText("ACTIVATE REPAIR");
            close_button->setEnable(true);
        } else {
            // Move to next sequence
            current_input = "";
            status_label->setText("");
            updateDisplay();
        }
    } else {
        // Incorrect
        status_label->setText("Incorrect value. Try again!");
    }
}

void SequenceCalibrationGame::updateDisplay()
{
    if (current_sequence_index < (int)sequences.size()) {
        const Sequence& seq = sequences[current_sequence_index];
        
        progress_label->setText("Stage " + string(current_sequence_index + 1) + " of " + string((int)sequences.size()));
        instruction_label->setText(seq.technical_description);
        
        // Build sequence display string
        string seq_text = "";
        for (int i = 0; i < 5; i++) {
            if (i > 0) seq_text += "   ";
            if (i == seq.blank_index) {
                seq_text += "[ ? ]";
            } else {
                seq_text += string(seq.values[i]);
            }
        }
        sequence_display->setText(seq_text);
        
        input_display->setText("Input: " + (current_input.empty() ? "(empty)" : current_input));
    }
}
