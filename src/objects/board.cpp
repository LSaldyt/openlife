#include "board.hpp"

namespace objects 
{

Board::Board(int width, int height) :
    window(sf::VideoMode(width, height), "")
{
    if (!font.loadFromFile("resources/UbuntuMono-R.ttf"))
    {
        std::cout << "Failed to load font: Was the program run from the openlife directory?" << std::endl;
    }
    info.setFont(font);
    info.setCharacterSize(24);
    
    frame_display.setFont(font);
    frame_display.setCharacterSize(24);
    frame_display.setPosition(0., 40.);
}

bool Board::_inBounds(Cell& cell)
{
    auto pos    = cell.getPosition();
    auto radius = cell.getRadius();
    // Addition of radius will make the cells bounce when their edges touch the window's edges
    return pos.x - radius > 0      && 
           pos.x + radius < border_vec.x && 
           pos.y - radius > 0      && 
           pos.y + radius < border_vec.y;
}

// Randomly generate n cells
void Board::_genCells(int nCells)
{
    for (unsigned i = 0; i < nCells; i++)
    {
        // Generates a new cell randomly
        cells.push_back(_generateRandomCell());
    }
}

// Randomly generate a single cell
std::shared_ptr<Cell> Board::_generateRandomCell()
{
    // Explicitly import names
    using tools::dist;
    using tools::randomGenerator;

    std::shared_ptr<Cell> cell = std::make_shared<Cell>(Cell());

    auto radius = cell->getRadius();
    auto size   = window.getSize();

    // Distributions for initial random settings
    auto widthDist  = dist(2 * radius, size.x - (2 * radius));
    auto heightDist = dist(2 * radius, size.y - (2 * radius));
    auto colorDist  = dist(0, 255);
    auto angleDist  = dist(0, 360);

    // An alternative to providing a seed
    auto generator  = randomGenerator();

    int x = widthDist(generator);
    int y = heightDist(generator);

    cell->setPosition(x, y);
    cell->setRotation(angleDist(generator));
    
    // Cells have 16 green so they can always be seen, red and blue go between 0 and 255 and represent turning rate and mating rate.
    
    // Higher mating rates mean more offspring but also more damage.
    cell->setFillColor(sf::Color(colorDist(generator), 16, colorDist(generator)));

    return cell;
}


void Board::_updateInteractions()
{
    std::vector<std::shared_ptr<Cell>> remaining;
    auto size = window.getSize();
    remaining.reserve(cells.size()); // No allocation problems :)
    /* ~Efficiently build the list of neighbors/mates
    
    If you simplify O(n + E(1, n, n-1)), it becomes O((n / 2) * n), which is O(n^2)
    What I actually meant is that this algorithm is more efficient than T = n^2:
    
    Values / Instructions / n^2
    1      / 1            / 1
    2      / 3            / 4
    3      / 6            / 9
    4      / 10           / 16
    ...
    10     / 55           / 100
    100    / 5050         / 10000
    1000   / 500500       / 1000000
    
    http://stackoverflow.com/questions/11032015/how-to-find-time-complexity-of-an-algorithm
    
    I've chosen to use iterators because it makes slicing possible, which is where the efficiency gains come from */
    auto it = cells.begin();
    while (it != cells.end())
    {
        auto cell = *it;
        remaining = std::vector<std::shared_ptr<Cell>>(it + 1, cells.end()); // Slice off the first element of the vector

        cell->interact(remaining);
        if (not _inBounds(**it))
        {
            // Modify the cell to push it into bounds
            cell->bounce(border_vec);
        }
        it++;
    }
}

void Board::_update()
{
    _updateInteractions();

    // Interact with neighbors, mate with mates
    std::vector<std::shared_ptr<Cell>> new_generation;
    for (auto cell : cells)
    {
        cell->update();
        auto children = cell->mate(); // Produce children with current set of mates, then clear list of mates
        new_generation.insert(new_generation.end(), children.begin(), children.end());
    }
    cells.insert(cells.end(), new_generation.begin(), new_generation.end());
    // Efficiently remove dead cells
    cells.erase(
        std::remove_if(cells.begin(), cells.end(),
                    [](const auto & e) { return not e->alive(); }),
            cells.end());

}

void Board::_render()
{
    window.clear();


    for (auto cell : cells)
    {
        window.draw(*cell);
    }

    info.setString("Cells: " + std::to_string(cells.size()));
    window.draw(info);
    window.draw(frame_display);
    window.display(); // For organization
}

void Board::_handle()
{
    sf::Event event;
    while (window.pollEvent(event))
    {
        if (event.type == sf::Event::Closed)
        {
            window.close();
        }
    }
}


// This function should really only call other functions (Similar to how no code goes in int main)
void Board::run(int nCells, int x, int y)
{
    using tools::getTime;

    border_vec = sf::Vector2i(x, y);
    _genCells(nCells);

    while (window.isOpen())
    {
        unsigned long long start_frame = getTime();
        _handle();
        _update();
        _render();
        
        if (cells.size() == 0) break;
        unsigned long long frame_time = getTime() - start_frame;
        frame_display.setString( "Drawtime: " + std::to_string(frame_time) + "us");
    }
}

}
