#version 330 core

in vec3 fragPos;    // data interpolated from vertex shader
out vec4 FragColor; // result color

uniform float minH;
uniform float maxH;

void main() {
	float h = clamp((fragPos.y - minH) / (maxH - minH), 0.0, 1.0);
	FragColor = mix(vec4(0.2,0.5,0.1,1), vec4(0.8,0.8,0.8,1), h);
}