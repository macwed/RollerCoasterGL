//
// Created by maciej on 25.07.25.
//
#include "SaveHeightmap.hpp"

#include <fstream>

#include "math/Array_2D.hpp"


void savePGM(const Array_2D<float>& arr, const std::string& filename) {
    std::ofstream file(filename);
    file << "P2\n" << arr.width() << " " << arr.height() << "\n255\n";
    for (int y = 0; y < arr.height(); y++) {
        for (int x = 0; x < arr.width(); x++) {
            int value = static_cast<int>(arr(x, y) * 255.0f);
            file << value << " ";
        }
        file << "\n";
    }
    file.close();
}
