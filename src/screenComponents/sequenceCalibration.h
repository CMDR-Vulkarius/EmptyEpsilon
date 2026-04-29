#ifndef SEQUENCE_CALIBRATION_H
#define SEQUENCE_CALIBRATION_H

#include "gui/gui2_overlay.h"
#include "gui/gui2_panel.h"
#include "gui/gui2_button.h"
#include "gui/gui2_label.h"
#include <functional>
#include <string>
#include <array>
#include <vector>

class SequenceCalibrationGame : public GuiOverlay
{
public:
    using CompletionCallback = std::function<void(bool success)>;

    SequenceCalibrationGame(GuiContainer* owner, CompletionCallback callback);

private:
    enum class SequenceType {
        Arithmetic,
        Geometric,
        Fibonacci,
        Squares,
        Cubes,
        Primes
    };

    struct Sequence {
        SequenceType type;
        std::string technical_description;
        std::array<int, 5> values;
        int blank_index;
        int correct_answer;
    };

    CompletionCallback completion_callback;
    GuiPanel* main_panel;
    GuiLabel* title_label;
    GuiLabel* instruction_label;
    GuiLabel* progress_label;
    GuiLabel* sequence_display;
    GuiLabel* input_display;
    GuiLabel* status_label;
    
    GuiPanel* numpad_panel;
    GuiButton* clear_button;
    GuiButton* submit_button;
    GuiButton* close_button;
    
    std::vector<Sequence> sequences;
    int current_sequence_index;
    std::string current_input;
    bool puzzle_complete;
    
    void initializePuzzle();
    void generateSequence(Sequence& seq, SequenceType type);
    std::string getSequenceTypeDescription(SequenceType type);
    
    void onNumpadDigitClicked(int digit);
    void onClearClicked();
    void onSubmitClicked();
    void updateDisplay();
};

#endif // SEQUENCE_CALIBRATION_H
