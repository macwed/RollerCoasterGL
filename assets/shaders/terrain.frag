#version 330 core
in VS { vec3 PosWS; vec3 NrmWS; vec2 uv; } v;
out vec4 FragColor;

// Tekstury (zostawiamy te same sloty dla zgodności)
uniform sampler2D texAlbedo;
uniform sampler2D texSpec;   // nieużywane w tym prostym shaderze terenu
uniform sampler2D texNormal; // nieużywane w tym prostym shaderze terenu

// Oświetlenie i mgła
uniform vec3 uCamPos;
uniform vec3 dirLightDir;
uniform vec3 dirLightColor;
uniform vec3 fogColor;
uniform float fogDensity;

void main() {
    vec3 albedo = texture(texAlbedo, v.uv).rgb;

    vec3 N = normalize(v.NrmWS);
    vec3 L = normalize(-dirLightDir);

    float ndl = max(dot(N, L), 0.0);

    // proste ambient + dyfuza
    vec3 color = 0.04 * albedo;
    color += ndl * dirLightColor * albedo;

    // mgła wykładnicza
    float dist = length(uCamPos - v.PosWS);
    float fog = 1.0 - exp(-fogDensity * fogDensity * dist * dist);
    color = mix(color, fogColor, clamp(fog, 0.0, 1.0));

    FragColor = vec4(color, 1.0);
}
