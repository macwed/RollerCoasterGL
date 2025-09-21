#version 330 core
in VS { vec3 posWS; vec3 nWS; vec2 uv; } v;
out vec4 FragColor;

uniform sampler2D texAlbedo;
uniform sampler2D texSpec;

uniform vec3 uCamPos;
uniform vec3 dirLightDir;
uniform vec3 dirLightColor;
uniform vec3 pointPos;
uniform vec3 pointColor;
uniform float pointRange;

void main() {
    vec3 albedo = texture(texAlbedo, v.uv).rgb;
    float specMask = texture(texSpec, v.uv).r;

    vec3 N = normalize(v.nWS);
    vec3 V = normalize(uCamPos - v.posWS);

    // światło kierunkowe (Blinn–Phong)
    vec3 Ld = normalize(-dirLightDir);
    vec3 Hd = normalize(Ld + V);
    float ndl = max(dot(N, Ld), 0.0);
    float ndh = max(dot(N, Hd), 0.0);
    float specD = pow(ndh, 64.0) * specMask;

    vec3 color = 0.03 * albedo;               // ambient (tło)
    color += albedo * dirLightColor * ndl;    // dyfuza
    color += dirLightColor * specD;           // spekular

    // światło punktowe (wagonik)
    vec3 Lp = pointPos - v.posWS;
    float d = length(Lp);
    float att = clamp(1.0 - d / pointRange, 0.0, 1.0);
    if (att > 0.0) {
        Lp /= max(d, 1e-6);
        vec3 Hp = normalize(Lp + V);
        float ndlp = max(dot(N, Lp), 0.0);
        float ndhp = max(dot(N, Hp), 0.0);
        float specP = pow(ndhp, 64.0) * specMask;
        color += (albedo * pointColor * ndlp + pointColor * specP) * att;
    }

    FragColor = vec4(color, 1.0);
}
