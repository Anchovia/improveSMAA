#version 330 core

uniform vec4 u_rtMetrics;
uniform sampler2D u_edgesTex;
uniform sampler2D u_areaTex;
uniform sampler2D u_searchTex;

#include "SMAA.hlsl"

in vec2 v_texcoord;
in vec2 v_pixcoord;
in vec4 v_offset[3];

layout(location = 0) out vec4 outColor;

void main() {
    outColor = SMAABlendingWeightCalculationPS(
        v_texcoord,
        v_pixcoord,
        v_offset,
        u_edgesTex,
        u_areaTex,
        u_searchTex,
        vec4(0.0));
}
