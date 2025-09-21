#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNrm;
layout(location=2) in vec2 aUV;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out VS {
    vec3 PosWS;
    vec3 NrmWS;
    vec2 uv;
} v;

void main() {
    vec4 posWS = model * vec4(aPos, 1.0);
    v.PosWS = posWS.xyz;

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    v.NrmWS = normalize(normalMatrix * aNrm);
    v.uv = aUV;

    gl_Position = projection * view * posWS;
}
