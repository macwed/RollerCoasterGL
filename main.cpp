#include <fstream>
#include <random>
#include "Array_2D.hpp"
#include "SimplexNoise.hpp"

constexpr int MAP_WIDTH = 1024;
constexpr int MAP_HEIGHT = 1024;
constexpr unsigned SEED = 123432;
constexpr float SCALE = 0.001f;
constexpr float OFFSET = 131.0f;

void savePGM(const Array_2D<float>& arr, const std::string& filename)
{
    std::ofstream file (filename);
    file << "P2\n" << arr.width() << " " << arr.height() << "\n255\n";
    for (int y = 0; y < arr.height(); y++)
    {
        for (int x = 0; x < arr.width(); x++)
        {
            int value = static_cast<int>(arr(x, y) * 255.0f);
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
            float height = simplexnoise.fbm(static_cast<float>(x) * SCALE + OFFSET, static_cast<float>(y) * SCALE + OFFSET, 8, 2.50f ,0.4f);
            heightmap(y, x) = height;
        }
    }

    heightmap.normalize();

    for (int i = 0; i < heightmap.height(); i++)
    {
        for (int j = 0; j < heightmap.width(); j++)
        {
            heightmap(i, j) = pow(heightmap(i, j), 1.2);
        }
    }

    std::cout << "Height max: " << *heightmap.maxVal() << " Height min: " << *heightmap.minVal() << std::endl;
    savePGM(heightmap, "heightmap2.pnm");

    return 0;
}
