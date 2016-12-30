#include "cell.hpp"

const float Cell::mate_radius      = 1.0f;
const float Cell::neighbor_radius  = 10.0f;
const float Cell::move_modifier    = 3.0f;
const float Cell::standard_radius  = 3.0f;
const float Cell::minimum_radius   = 10.0f;

const int Cell::underpopulation_limit = 2;
const int Cell::overpopulation_limit  = 5;
const int Cell::max_neighbors = 5;

const double Cell::underpopulation_damage = 0.001;
const double Cell::overpopulation_damage  = 0.2;
const double Cell::affection_threshold = 1000.;
const double Cell::turn_rate = 1.0; // Degrees
const double Cell::max_life = 10.0;

// Create initial cell
Cell::Cell() :
    Entity(Cell::standard_radius, Cell::max_life),
    genome()
{
    auto bounds = getLocalBounds();
    setOrigin(bounds.width / 2, bounds.height / 2);
    displayAttributes();
}

// Create child cell
Cell::Cell(Cell& a, Cell& b) :
    Cell::Cell()
{
    using tools::avg;

    genome = Genome(a.genome, b.genome);

    setRotation(a.getRotation() + 90);

    auto posA = a.getPosition();
    auto posB = b.getPosition();

    // Spawn location for child cells
    auto dx = avg(posA.x, posB.x);
    auto dy = avg(posA.y, posB.y);

    setPosition(dx, dy);
    displayAttributes();
}

void Cell::displayAttributes()
{
    setFillColor(genome.representation());
    setRadius(Cell::standard_radius * genome.gene("size") + Cell::minimum_radius);
}

void Cell::bounce(sf::Vector2f bounds)
{
    auto rotation = getRotation();
    auto radius   = getRadius();
    auto pos      = getPosition();

    float normal = 0; // Angle normal to surface
    float reflected = 0; // Angle after reflection off of surface

    bool left   = pos.x - radius < 0;
    bool right  = pos.x + radius > bounds.x;
    bool top    = pos.y - radius < 0;
    bool bottom = pos.y + radius > bounds.y;

    // Corners don't use a normal angle because it doesn't make sense (collision is with two surfaces)
    // Single-plane collisions use the law of reflection

    // Corner cases -> Turn 180 degrees (Other methods, like facing the center of the board, resulted in bugs)
    if (left && top)
    {
        reflected = (int)(rotation + 180) % 360;
        move(1.0, 1.0);
    }
    else if (left && bottom)
    {
        move(1.0, -1.0);
        reflected = (int)(rotation + 180) % 360;
    }
    else if (right && top)
    {
        move(-1.0, 1.0);
        reflected = (int)(rotation + 180) % 360;
    }
    else if (right && bottom)
    {
        move(-1.0, -1.0);
        reflected = (int)(rotation + 180) % 360;
    }

    // Edge cases
    else
    {
        if (left)       
        {
            normal = 0;
            move(1.0, 0);
        }
        else if (right) 
        {
            normal = 180;
            move(-1.0, 0);
        }
        else if (top) 
        {
            normal = 90;
            move(0, 1.0);
        }
        else if (bottom) 
        {
            normal = 270;
            move(0, -1.0);
        }

        float inverse_normal = (int)(normal + 180) % 360;
        float incidence      = rotation - inverse_normal;

        reflected = normal - incidence;
    }

    setRotation(reflected);
}


// Build a list/count of neighbors and mates
void Cell::interact(const std::vector<std::shared_ptr<Cell>>& cells)
{
    for (auto& cell : cells)
    {
        auto dist   = distance(*this, *cell);
        auto radius = getRadius();

        // Mate with closer cells, treat further ones as neighbors, and ignore the rest
        if (dist < radius * Cell::mate_radius)
        {
            mates.push_back(cell);
        }

        if (dist < radius * Cell::neighbor_radius)
        {
            addNeighbor(cell);
            cell->addNeighbor(std::make_shared<Cell>(*this));
        }
    }
}

double Cell::calculateIdealAngle(sf::Vector2f neighborLoc, double currentAngle)
{
    auto angle = atan2(getPosition().y - neighborLoc.y, 
                       getPosition().x - neighborLoc.x);
    return degrees(angle) - 90;
}

double Cell::calculateNextAngle(double currentAngle, bool isOverpopulated)
{
    if (neighbors.empty())
    {
        return currentAngle;
    }

    double idealAngle = calculateIdealAngle(getAverageLocation(neighbors), currentAngle);
    if (isOverpopulated) idealAngle = fmod((idealAngle + 180), 360);

    if (idealAngle > currentAngle + turn_rate) {
        return currentAngle + turn_rate;
    } else if (idealAngle < currentAngle - turn_rate) {
        return currentAngle - turn_rate;
    } else {
        return idealAngle;
    }
}

sf::Vector2f getAverageLocation(std::vector<std::shared_ptr<Cell>> cells)
{
    sf::Vector2f averagePoint (0, 0);
    if (cells.size() == 0)
    {
        return averagePoint;
    }
    for (auto& cell : cells) 
    {
        averagePoint.x += cell->getPosition().x;
        averagePoint.y += cell->getPosition().y;
    }

    averagePoint.x /= cells.size();
    averagePoint.y /= cells.size();

    return averagePoint;
}

void Cell::addNeighbor(std::shared_ptr<Cell> cell)
{
    neighbors.push_back(cell);
}

void Cell::update()
{
    auto radius = getRadius();
    moveVec(*this, Cell::move_modifier * (Cell::standard_radius / radius)); // As radius increases, speed decreases

    if (neighbors.size() < Cell::underpopulation_limit)     // Underpopulation
    {
        setRotation(calculateNextAngle(this->getRotation(), false));
        damage(Cell::underpopulation_damage);
    }
    else if (neighbors.size() > Cell::overpopulation_limit) // Overpopulation
    {
        setRotation(calculateNextAngle(this->getRotation(), true));
        damage(Cell::overpopulation_damage);
    } else {
        setRotation(calculateNextAngle(this->getRotation(), false));
        affection += genome.gene("affection_prime");
    }

    const sf::Color *cellColor = &getFillColor();
    // Fade on death
    setFillColor(sf::Color(cellColor->r, cellColor->g, cellColor->b, 255 * lifePercent()));
}

std::string Cell::csv()
{
    auto position = getPosition();
    return std::to_string(position.x) + "," + std::to_string(position.y);
}

std::vector<std::shared_ptr<Cell>> Cell::mate()
{
    std::vector<std::shared_ptr<Cell>> children;

    if (mates.size() > 0 and neighbors.size() < Cell::max_neighbors)
    {
        for (auto& mate : mates)
        {
            //The mating threshold stops 2 parents from falling too far in love.
            if (affection + mate->affection > Cell::affection_threshold)
            {
                affection = 0;
                mate->affection = 0;
                children.push_back(std::make_shared<Cell>(Cell(*this, *mate)));
            }
        }
    }

    // Interaction finished
    mates.clear();
    neighbors.clear();

    return children;
}
