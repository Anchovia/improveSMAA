#version 330 core

in vec2 v_texcoord;

layout(location = 0) out vec4 outColor;

uniform sampler2D u_leftTex;
uniform sampler2D u_rightTex;
uniform int u_viewMode;
uniform float u_split;
uniform float u_diffScale;

void main() {
    vec4 leftColor = texture(u_leftTex, v_texcoord);
    vec4 rightColor = texture(u_rightTex, v_texcoord);

    if (u_viewMode == 1) {
        float line = smoothstep(0.0025, 0.0, abs(v_texcoord.x - u_split));
        vec4 splitColor = v_texcoord.x < u_split ? leftColor : rightColor;
        outColor = mix(splitColor, vec4(1.0, 0.88, 0.12, 1.0), line);
        return;
    }

    if (u_viewMode == 2) {
        outColor = vec4(abs(rightColor.rgb - leftColor.rgb) * u_diffScale, 1.0);
        return;
    }

    outColor = rightColor;
}
