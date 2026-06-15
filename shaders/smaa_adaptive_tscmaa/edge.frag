#version 330 core

uniform vec4 u_rtMetrics;
uniform sampler2D u_colorTex;

#include "SMAA.hlsl"

in vec2 v_texcoord;
in vec4 v_offset[3];

layout(location = 0) out vec4 outColor;

void main() {
    vec2 edges = SMAALumaEdgeDetectionPS(v_texcoord, v_offset, u_colorTex);
    outColor = vec4(edges, 0.0, 0.0);
}
