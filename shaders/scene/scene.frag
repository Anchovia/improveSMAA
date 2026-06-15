#version 330 core

in vec3 v_normal;
in vec2 v_texcoord;

layout(location = 0) out vec4 outColor;

uniform float u_exposure;
uniform vec4 u_materialColor;
uniform sampler2D u_diffuseTex;
uniform int u_useDiffuseTexture;

void main() {
    vec3 n = normalize(v_normal);
    vec3 key = normalize(vec3(-0.45, 0.75, 0.35));
    vec3 fill = normalize(vec3(0.35, 0.25, -0.6));

    vec3 surfaceColor = u_materialColor.rgb;
    if (u_useDiffuseTexture != 0) {
        vec4 texel = texture(u_diffuseTex, v_texcoord);
        if (texel.a < 0.35) {
            discard;
        }
        surfaceColor *= texel.rgb;
    }

    float keyLight = max(dot(n, key), 0.0);
    float fillLight = max(dot(n, fill), 0.0) * 0.35;
    vec3 color = surfaceColor * (0.20 + keyLight * 0.85 + fillLight);
    color = vec3(1.0) - exp(-color * max(u_exposure, 0.001));
    outColor = vec4(color, 1.0);
}
