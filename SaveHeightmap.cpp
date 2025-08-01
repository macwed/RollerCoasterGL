//
// Created by maciej on 25.07.25.
//
#include <fstream>

#include "Array_2D.hpp"
#include "SaveHeightmap.hpp"


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