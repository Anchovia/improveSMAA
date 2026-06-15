#version 330 core

in vec3 v_normal;
in vec3 v_color;

layout(location = 0) out vec4 outColor;

uniform float u_exposure;

void main() {
    vec3 n = normalize(v_normal);
    vec3 key = normalize(vec3(-0.45, 0.75, 0.35));
    vec3 fill = normalize(vec3(0.35, 0.25, -0.6));

    float keyLight = max(dot(n, key), 0.0);
    float fillLight = max(dot(n, fill), 0.0) * 0.35;
    vec3 color = v_color * (0.20 + keyLight * 0.85 + fillLight);
    color = vec3(1.0) - exp(-color * max(u_exposure, 0.001));
    outColor = vec4(color, 1.0);
}
