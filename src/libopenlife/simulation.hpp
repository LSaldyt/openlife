#include "cell.hpp"
#include <iostream>
#include <vector>
#include <memory>

struct rectangleShape {
    double x = 0;
    double y = 0;
    double width;
    double height;
};


using tools::print;
using tools::getTime;

// Code for simulation backend only
struct Simulation
{
    const static int csv_save_period;

    Simulation();
    Simulation(int nCells, double width, double height);
    ~Simulation();

    void update();
    void updateInteractions();
    
    std::vector<std::shared_ptr<Cell>> cells;

    int getCellCount();
    float getArea();
    
    position center_marker;

private:
    // Functions for generating random cells
    void _genCells(int nCells);
    std::shared_ptr<Cell> _generateRandomCell();
    
    bool _inBounds(Cell& cell);
    void _updateInteractions();
    position _getAverageLocation();
    
    rectangleShape border;

    unsigned long long last_update;
    int update_count;
};
