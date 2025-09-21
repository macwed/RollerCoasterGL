//
// Created by maciej on 17.08.25.
//

#ifndef TRACKTYPES_HPP
#define TRACKTYPES_HPP

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

namespace rc::common {
    enum class EdgeType { CatmullRom, Linear, Circular, Helix };
    struct EdgeMeta {
        EdgeType type{EdgeType::CatmullRom};
        // Parametry łuku kołowego (gdy type==Circular)
        // Łuk leży w płaszczyźnie o normalnej 'circleNormal' i środku 'circleCenter'.
        // Jeśli circleRadius <= 0, to bierzemy promień z punktu startowego.
        // 'circleTurns' pozwala wymusić dodatkowe okrążenia (plus/minus) poza najkrótszym łukiem.
        glm::vec3 circleCenter{0.0f};
        glm::vec3 circleNormal{0.0f, 1.0f, 0.0f};
        float     circleRadius{0.0f};
        float     circleTurns{0.0f};
        bool      circleShortest{true};

        // Parametry helisy (gdy type==Helix)
        // Helisa wzdłuż osi przechodzącej przez 'helixAxisPoint' o kierunku 'helixAxisDir' (znormalizowany),
        // promień 'helixRadius', skok 'helixPitch' (przesunięcie na oś na jeden obrót),
        // oraz łączna liczba obrotów 'helixTurns' na danym segmencie.
        glm::vec3 helixAxisPoint{0.0f};
        glm::vec3 helixAxisDir{0.0f, 1.0f, 0.0f};
        float     helixRadius{1.0f};
        float     helixPitch{1.0f};
        float     helixTurns{1.0f};

        // (Loop usunięty)
    };
    struct NodeMeta {
        bool stationStart = false, stationEnd = false;
        float length = 0.f;
    };
    struct RollKey {
        float s = 0.f, roll = 0.f;
    };
    struct Frame {
        glm::vec3 pos, T, N, B;
        float s = 0.f;
        glm::quat q;
    };
} // namespace rc::common

#endif // TRACKTYPES_HPP
