#version 330 core

uniform vec4 u_rtMetrics;
uniform sampler2D u_colorTex;
uniform sampler2D u_blendTex;

#include "SMAA.hlsl"

in vec2 v_texcoord;
in vec4 v_offset;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = SMAANeighborhoodBlendingPS(v_texcoord, v_offset, u_colorTex, u_blendTex);
}
