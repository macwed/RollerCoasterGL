#include "Array_2D.hpp"
#include "SimplexNoise.hpp"

constexpr int MAP_WIDTH = 2048;
constexpr int MAP_HEIGHT = 2048;
constexpr unsigned SEED = 81232256;
constexpr float SCALE = 0.02f;
constexpr float OFFSET = 263.0f;

int main()
{
    Array_2D<float> heightmap(MAP_WIDTH, MAP_HEIGHT);
    SimplexNoise simplexnoise = SimplexNoise(SEED);
    for (int y = 0; y < heightmap.height(); y++)
    {
        for (int x = 0; x < heightmap.width(); x++)
        {
            heightmap(x, y) = simplexnoise.noise(x * SCALE + OFFSET, y * SCALE + OFFSET);
        }
    }

    heightmap.normalize();

    for (int y = 0; y < heightmap.height(); y++)
    {
        for (int x = 0; x < heightmap.width(); x++) {
            //if (heightmap(x, y) >= 1.0f || heightmap(x, y) <= 0.0f)
            {
                std::cout << heightmap(x, y) << " ";
            }
        }
        std::cout << std::endl;
    }

    std::cout << "Height(0,0): " << heightmap(0, 0) << std::endl;

    return 0;
}