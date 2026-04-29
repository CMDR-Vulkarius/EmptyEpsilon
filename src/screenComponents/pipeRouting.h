#ifndef PIPE_ROUTING_H
#define PIPE_ROUTING_H

#include "gui/gui2_overlay.h"
#include <array>
#include <functional>

class GuiPanel;
class GuiButton;
class GuiLabel;

// Pipe tile types
enum class PipeType {
    Empty,       // No pipe
    Straight,    // Straight pipe (horizontal or vertical based on rotation)
    Curved,      // 90-degree elbow
    TJunction,   // T-junction
    Cross        // Cross/intersection
};

// Each tile has a type and rotation (0, 90, 180, 270 degrees)
struct PipeTile {
    PipeType type;
    int rotation;  // 0, 1, 2, 3 (each = 90 degrees)
    
    PipeTile() : type(PipeType::Empty), rotation(0) {}
    PipeTile(PipeType t, int r) : type(t), rotation(r) {}
    
    // Check if this tile connects in a given direction (0=right, 1=down, 2=left, 3=up)
    bool hasConnection(int direction) const;
};

class PipeRoutingGame : public GuiOverlay
{
public:
    using CompletionCallback = std::function<void(bool success)>;
    
    PipeRoutingGame(GuiContainer* owner, CompletionCallback callback);
    
    void onDraw(sp::RenderTarget& target) override;
    
private:
    static constexpr int GRID_SIZE = 5;
    static constexpr int TILE_SIZE = 80;
    
    GuiPanel* main_panel;
    GuiElement* grid_container;
    GuiLabel* status_label;
    GuiButton* close_button;
    
    std::array<std::array<PipeTile, GRID_SIZE>, GRID_SIZE> grid;
    std::array<std::array<GuiButton*, GRID_SIZE>, GRID_SIZE> tile_buttons;
    
    CompletionCallback completion_callback;
    bool puzzle_solved;
    int source_row;
    int dest_row;
    
    void initializePuzzle();
    void onTileClicked(int row, int col);
    void checkSolution();
    bool hasPathFromSourceToDest();
    void drawPipe(sp::RenderTarget& target, glm::vec2 position, const PipeTile& tile);
};

#endif // PIPE_ROUTING_H
