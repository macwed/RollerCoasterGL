#version 330 core

in vec3 fragPos;    // data interpolated from vertex shader
out vec4 FragColor; // result color
void main() {
    FragColor = vec4(0.5, 0.8, 0.3, 1.0); // light green
}
