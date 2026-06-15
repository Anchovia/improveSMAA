#version 330 core

uniform vec4 u_rtMetrics;

#include "SMAA.hlsl"

out vec2 v_texcoord;
out vec2 v_pixcoord;
out vec4 v_offset[3];

void main() {
    vec2 position = vec2(gl_VertexID == 1 ? 3.0 : -1.0,
                         gl_VertexID == 2 ? 3.0 : -1.0);
    v_texcoord = position * 0.5 + 0.5;
    gl_Position = vec4(position, 0.0, 1.0);

    SMAABlendingWeightCalculationVS(v_texcoord, v_pixcoord, v_offset);
}
