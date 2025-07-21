#include <fstream>
#include <random>
#include "Array_2D.hpp"
#include "SimplexNoise.hpp"

constexpr int MAP_WIDTH = 2048;
constexpr int MAP_HEIGHT = 2048;
constexpr unsigned SEED = 56;
constexpr float SCALE = 0.0012f;
constexpr float OFFSET = 131.0f;

void savePGM(const Array_2D<float>& arr, const std::string& filename)
{
    std::ofstream file (filename);
    file << "P2\n" << arr.width() << " " << arr.height() << "\n255\n";
    for (int y = 0; y < arr.height(); y++)
    {
        for (int x = 0; x < arr.width(); x++)
        {
            int value = static_cast<int>(std::clamp(arr(x, y), 0.0f, 1.0f) * 255.0f);
            file << value << " ";
        }
        file << "\n";
    }
    file.close();
}

int main()
{
    Array_2D<float> heightmap(MAP_WIDTH, MAP_HEIGHT);
    SimplexNoise simplexnoise = SimplexNoise(SEED);
    for (int y = 0; y < heightmap.height(); y++)
    {
        for (int x = 0; x < heightmap.width(); x++)
        {
            heightmap(y, x) = simplexnoise.noise(static_cast<float>(x) * SCALE + OFFSET, static_cast<float>(y) * SCALE + OFFSET);
        }
    }

    //heightmap.normalize();

    /*for (int y = 0; y < heightmap.height(); y++)
    {
        for (int x = 0; x < heightmap.width(); x++) {
            //if (heightmap(x, y) >= 1.0f || heightmap(x, y) <= 0.0f)
            {
                std::cout << heightmap(x, y) << " ";
            }
        }
        std::cout << std::endl;
    }*/

    std::cout << "Height(0,0): " << heightmap(0, 0) << std::endl;
    savePGM(heightmap, "heightmap.pnm");

    return 0;
}
