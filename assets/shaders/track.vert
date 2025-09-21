#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;

uniform mat4 model, view, projection;

out VS {
  vec3 posWS;
  vec3 nWS;
  vec2 uv;
} v;

void main(){
  vec4 pWS = model * vec4(aPos,1.0);
  v.posWS = pWS.xyz;
  // macierz normalnych:
  v.nWS = mat3(transpose(inverse(model))) * aNormal;
  v.uv = aUV;

  gl_Position = projection * view * pWS;
}
