#include "cell.hpp"

struct rectangleShape {
    double x = 0;
    double y = 0;
    double width;
    double height;
};


namespace objects
{

using tools::print;
using tools::getTime;

// Code for simulation backend only
struct Simulation
{
    const static int csv_save_period;

    Simulation();
    Simulation(int nCells, int width, int height);

    void update();
    void updateInteractions();
    
    //void render(sf::RenderWindow& target);

    int getCellCount();
    float getArea();

private:
    // Functions for generating random cells
    void _genCells(int nCells);
    std::shared_ptr<Cell> _generateRandomCell();
    
    bool _inBounds(Cell& cell);
    void _updateInteractions();
    position _getAverageLocation();

    unsigned long long last_update;
    int update_count;

    // Simulation backend
    std::vector<std::shared_ptr<Cell>> cells;
    
    
    //Graphics BS
    
    rectangleShape border;
    position center_marker;
};

}
