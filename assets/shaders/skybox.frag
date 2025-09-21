#version 330 core
in vec3 vDir;
out vec4 FragColor;

//kolory gradientu nieba
uniform vec3 uTopColor;
uniform vec3 uHorizonColor;
uniform vec3 uBottomColor;

void main(){
    vec3 d = normalize(vDir);
    float tUp = clamp(d.y*0.5 + 0.5, 0.0, 1.0);
    // mieszanie:
    vec3 mid = mix(uBottomColor, uHorizonColor, smoothstep(0.0, 1.0, tUp));
    vec3 col = mix(mid, uTopColor, smoothstep(0.5, 1.0, tUp));
    FragColor = vec4(col, 1.0);
}

