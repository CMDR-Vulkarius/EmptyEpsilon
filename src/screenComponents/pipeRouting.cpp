#include "pipeRouting.h"
#include "i18n.h"
#include "gui/gui2_panel.h"
#include "gui/gui2_button.h"
#include "gui/gui2_label.h"
#include "gui/gui2_element.h"
#include "graphics/opengl.h"
#include <random>

bool PipeTile::hasConnection(int direction) const
{
    if (type == PipeType::Empty) return false;
    
    // Normalize direction to 0-3
    int dir = direction % 4;
    
    switch (type)
    {
        case PipeType::Straight:
            // Connects at rotation 0 and 180 (horizontal) OR 90 and 270 (vertical)
            if (rotation % 2 == 0) // Horizontal
                return (dir == 0 || dir == 2); // right or left
            else // Vertical
                return (dir == 1 || dir == 3); // down or up
            
        case PipeType::Curved:
            // 90-degree elbow: rotation 0 = right+down, 1 = down+left, 2 = left+up, 3 = up+right
            return (dir == rotation || dir == (rotation + 1) % 4);
            
        case PipeType::TJunction:
            // T-junction: rotation determines which direction is NOT connected
            // rotation 0 = missing up (has right, down, left)
            // rotation 1 = missing right (has down, left, up)
            // rotation 2 = missing down (has left, up, right)
            // rotation 3 = missing left (has up, right, down)
            return dir != ((rotation + 3) % 4);
            
        case PipeType::Cross:
            return true; // All directions
            
        default:
            return false;
    }
}

PipeRoutingGame::PipeRoutingGame(GuiContainer* owner, CompletionCallback callback)
: GuiOverlay(owner, "PIPE_ROUTING_GAME", colorConfig.background),
  completion_callback(callback),
  puzzle_solved(false)
{
    // Main panel
    main_panel = new GuiPanel(this, "PIPE_GAME_PANEL");
    main_panel->setPosition(0, 0, sp::Alignment::Center)->setSize(700, 700);
    
    // Title
    (new GuiLabel(main_panel, "PIPE_GAME_TITLE", tr("ROUTE THE REACTOR COOLANT"), 25))
        ->setSize(GuiElement::GuiSizeMax, 50)->setPosition(0, 10, sp::Alignment::TopCenter);
    
    // Status label
    status_label = new GuiLabel(main_panel, "PIPE_GAME_STATUS", tr("Click tiles to rotate pipes"), 20);
    status_label->setSize(GuiElement::GuiSizeMax, 30)->setPosition(0, 60, sp::Alignment::TopCenter);
    
    // Grid container
    grid_container = new GuiElement(main_panel, "PIPE_GRID");
    grid_container->setPosition(0, 100, sp::Alignment::TopCenter)->setSize(GRID_SIZE * TILE_SIZE, GRID_SIZE * TILE_SIZE);
    
    // Create tile buttons
    for (int row = 0; row < GRID_SIZE; row++)
    {
        for (int col = 0; col < GRID_SIZE; col++)
        {
            auto button = new GuiButton(grid_container, "", "", [this, row, col]() {
                onTileClicked(row, col);
            });
            button->setPosition(col * TILE_SIZE, row * TILE_SIZE, sp::Alignment::TopLeft);
            button->setSize(TILE_SIZE - 2, TILE_SIZE - 2);
            tile_buttons[row][col] = button;
        }
    }
    
    // Close button
    close_button = new GuiButton(main_panel, "PIPE_CLOSE", tr("CLOSE"), [this]() {
        if (completion_callback)
            completion_callback(puzzle_solved);
        destroy();
    });
    close_button->setPosition(0, -20, sp::Alignment::BottomCenter)->setSize(200, 50);
    
    initializePuzzle();
}

void PipeRoutingGame::initializePuzzle()
{
    // Use random device for seeding
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> type_dist(1, 4); // 1-4 for PipeType (skip Empty)
    std::uniform_int_distribution<> rot_dist(0, 3);
    std::uniform_int_distribution<> row_dist(0, GRID_SIZE - 1);
    
    // Choose random source and destination rows (left and right edges)
    source_row = row_dist(gen);
    dest_row = row_dist(gen);
    
    // Initialize with random pipes
    for (int row = 0; row < GRID_SIZE; row++)
    {
        for (int col = 0; col < GRID_SIZE; col++)
        {
            // Create random pipe type and rotation
            PipeType type = static_cast<PipeType>(type_dist(gen));
            int rotation = rot_dist(gen);
            
            grid[row][col] = PipeTile(type, rotation);
        }
    }
    
    // Ensure source and dest have appropriate connections
    // Source (left edge): must connect to the right
    grid[source_row][0] = PipeTile(PipeType::Straight, 0); // Horizontal
    
    // Dest (right edge): must connect to the left
    grid[dest_row][GRID_SIZE - 1] = PipeTile(PipeType::Straight, 0); // Horizontal
}

void PipeRoutingGame::onTileClicked(int row, int col)
{
    if (puzzle_solved) return;
    
    // Don't allow rotating source or destination tiles
    if ((row == source_row && col == 0) || (row == dest_row && col == GRID_SIZE - 1))
        return;
    
    // Rotate the tile
    grid[row][col].rotation = (grid[row][col].rotation + 1) % 4;
    
    // Check if puzzle is solved
    checkSolution();
}

void PipeRoutingGame::checkSolution()
{
    if (hasPathFromSourceToDest())
    {
        puzzle_solved = true;
        status_label->setText(tr("SUCCESS! Coolant flow restored!"));
    }
    else
    {
        status_label->setText(tr("Click tiles to rotate pipes"));
    }
}

bool PipeRoutingGame::hasPathFromSourceToDest()
{
    // BFS to find path from source to destination
    std::array<std::array<bool, GRID_SIZE>, GRID_SIZE> visited{};
    std::vector<std::pair<int, int>> queue;
    
    queue.push_back({source_row, 0});
    visited[source_row][0] = true;
    
    // Direction vectors: right, down, left, up
    const int dr[] = {0, 1, 0, -1};
    const int dc[] = {1, 0, -1, 0};
    
    while (!queue.empty())
    {
        auto [row, col] = queue.front();
        queue.erase(queue.begin());
        
        // Check if we reached the destination
        if (row == dest_row && col == GRID_SIZE - 1)
            return true;
        
        // Check all 4 directions
        for (int dir = 0; dir < 4; dir++)
        {
            // Check if current tile connects in this direction
            if (!grid[row][col].hasConnection(dir))
                continue;
            
            int new_row = row + dr[dir];
            int new_col = col + dc[dir];
            
            // Check bounds
            if (new_row < 0 || new_row >= GRID_SIZE || new_col < 0 || new_col >= GRID_SIZE)
                continue;
            
            // Check if already visited
            if (visited[new_row][new_col])
                continue;
            
            // Check if the neighboring tile connects back (opposite direction)
            int opposite_dir = (dir + 2) % 4;
            if (!grid[new_row][new_col].hasConnection(opposite_dir))
                continue;
            
            // Add to queue
            visited[new_row][new_col] = true;
            queue.push_back({new_row, new_col});
        }
    }
    
    return false;
}

void PipeRoutingGame::onDraw(sp::RenderTarget& target)
{
    GuiOverlay::onDraw(target);
    
    // Draw pipes on each tile
    // Calculate base position: main panel is centered, grid_container is at (0, 100) TopCenter
    glm::vec2 screen_center = glm::vec2(target.getVirtualSize()) / 2.0f;
    glm::vec2 main_panel_size(700, 700);
    glm::vec2 main_panel_top_left = screen_center - main_panel_size / 2.0f;
    
    // grid_container is at (0, 100) with TopCenter alignment relative to main_panel
    // TopCenter means position.x is center-aligned, position.y is top-aligned
    float grid_width = GRID_SIZE * TILE_SIZE;  // 400
    float grid_container_left = main_panel_top_left.x + (main_panel_size.x - grid_width) / 2.0f;
    float grid_container_top = main_panel_top_left.y + 100;
    
    for (int row = 0; row < GRID_SIZE; row++)
    {
        for (int col = 0; col < GRID_SIZE; col++)
        {
            // Each button is at (col * TILE_SIZE, row * TILE_SIZE) with size (TILE_SIZE-2, TILE_SIZE-2)
            glm::vec2 tile_center = glm::vec2(
                grid_container_left + col * TILE_SIZE + TILE_SIZE / 2.0f,
                grid_container_top + row * TILE_SIZE + TILE_SIZE / 2.0f
            );
            drawPipe(target, tile_center, grid[row][col]);
        }
    }
    
    // Draw source indicator (left edge - entry point)
    glm::u8vec4 source_color(64, 255, 64, 255); // Green
    float source_y = grid_container_top + source_row * TILE_SIZE + TILE_SIZE / 2.0f;
    float source_x = grid_container_left - 30;
    
    // Draw arrow pointing right (triangle)
    target.drawLine(glm::vec2(source_x - 15, source_y), glm::vec2(source_x, source_y), source_color);
    target.drawLine(glm::vec2(source_x, source_y), glm::vec2(source_x - 8, source_y - 8), source_color);
    target.drawLine(glm::vec2(source_x, source_y), glm::vec2(source_x - 8, source_y + 8), source_color);
    
    // Label
    target.drawText(sp::Rect(source_x - 40, source_y - 20, 40, 20), "IN", sp::Alignment::Center, 16, nullptr, source_color);
    
    // Draw destination indicator (right edge - exit point)
    glm::u8vec4 dest_color(255, 200, 64, 255); // Orange/yellow
    float dest_y = grid_container_top + dest_row * TILE_SIZE + TILE_SIZE / 2.0f;
    float dest_x = grid_container_left + grid_width + 30;
    
    // Draw arrow pointing right (triangle)
    target.drawLine(glm::vec2(dest_x - 15, dest_y), glm::vec2(dest_x, dest_y), dest_color);
    target.drawLine(glm::vec2(dest_x, dest_y), glm::vec2(dest_x - 8, dest_y - 8), dest_color);
    target.drawLine(glm::vec2(dest_x, dest_y), glm::vec2(dest_x - 8, dest_y + 8), dest_color);
    
    // Label
    target.drawText(sp::Rect(dest_x, dest_y - 20, 50, 20), "OUT", sp::Alignment::Center, 16, nullptr, dest_color);
}

void PipeRoutingGame::drawPipe(sp::RenderTarget& target, glm::vec2 position, const PipeTile& tile)
{
    if (tile.type == PipeType::Empty) return;
    
    // Pipe color
    glm::u8vec4 color(100, 200, 255, 255); // Cyan for pipes
    float half_tile = TILE_SIZE / 2.0f - 4.0f;
    
    // Draw based on type and rotation
    switch (tile.type)
    {
        case PipeType::Straight:
            if (tile.rotation % 2 == 0) // Horizontal
            {
                // Draw horizontal line
                target.drawLine(position - glm::vec2(half_tile, 0), 
                              position + glm::vec2(half_tile, 0), 
                              color);
            }
            else // Vertical
            {
                // Draw vertical line
                target.drawLine(position - glm::vec2(0, half_tile), 
                              position + glm::vec2(0, half_tile), 
                              color);
            }
            break;
            
        case PipeType::Curved:
            // Draw L-shape based on rotation
            {
                glm::vec2 offsets[] = {
                    {half_tile, 0},   // right
                    {0, half_tile},   // down
                    {-half_tile, 0},  // left
                    {0, -half_tile}   // up
                };
                
                target.drawLine(position, position + offsets[tile.rotation], color);
                target.drawLine(position, position + offsets[(tile.rotation + 1) % 4], color);
            }
            break;
            
        case PipeType::TJunction:
            // Draw T-shape (3 lines, missing one direction)
            {
                glm::vec2 offsets[] = {
                    {half_tile, 0},   // right
                    {0, half_tile},   // down
                    {-half_tile, 0},  // left
                    {0, -half_tile}   // up
                };
                
                int missing = (tile.rotation + 3) % 4;
                for (int i = 0; i < 4; i++)
                {
                    if (i != missing)
                    {
                        target.drawLine(position, position + offsets[i], color);
                    }
                }
            }
            break;
            
        case PipeType::Cross:
            // Draw + shape (all 4 directions)
            target.drawLine(position - glm::vec2(half_tile, 0), 
                          position + glm::vec2(half_tile, 0), 
                          color);
            target.drawLine(position - glm::vec2(0, half_tile), 
                          position + glm::vec2(0, half_tile), 
                          color);
            break;
            
        default:
            break;
    }
}
