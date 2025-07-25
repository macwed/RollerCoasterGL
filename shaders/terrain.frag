#version 330 core

in vec3 fragPos;    // data interpolated from vertex shader
out vec4 FragColor; // result color
void main() {
    float h = clamp((fragPos.y - 5.0) / 100.0, 0.0, 1.0);
    FragColor = mix(vec4(0.3,0.6,0.2,1), vec4(0.9,0.9,0.95,1), h);

}
