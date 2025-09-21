//
// Created by maciej on 20.09.25.
//

#ifndef SUPPORTGEOMETRYBUILDER_HPP
#define SUPPORTGEOMETRYBUILDER_HPP

#include <glm/glm.hpp>

#include "RailGeometryBuilder.hpp"

namespace rc::gfx::geometry {

    class SupportGeometryBuilder {
    public:
        explicit SupportGeometryBuilder(MeshOut& mesh) : mesh_(mesh) {}

        // belka między A i B
        void addBeamBox(const glm::vec3& A, const glm::vec3& B, float thick, float height);

        // cylinder między A i B
        void addSupportCylinder(const glm::vec3& top, const glm::vec3& bottom,
                                float radius, int ringSides=12);

        // stopka na ziemi
        void addFootDisk(const glm::vec3& center, float r, int sides=16);

    private:
        MeshOut& mesh_;

        void addOrientedBox(const glm::vec3& center, const glm::vec3& axisDir,
                            float length, float sx, float sy);
        void addOrientedCylinder(const glm::vec3& A, const glm::vec3& B,
                                 float r, int sides);
    };


} // support

#endif //SUPPORTGEOMETRYBUILDER_HPP
