#version 330 core
// wejście sześcian
layout(location=0) in vec3 aPos;

uniform mat4 uProjection;
uniform mat4 uViewNoTrans; // widok bez translacji (tylko rotacja kamery)

out vec3 vDir; // kierunek z kamery

void main(){
    // kierunek „na zewnątrz” – po prostu pos sześcianu po obrocie kamery
    vDir = mat3(uViewNoTrans) * aPos;
    // rzutowanie bez translacji – sześcian „przyklejony” do kamery
    vec4 pos = uProjection * uViewNoTrans * vec4(aPos, 1.0);
    gl_Position = pos;
}

