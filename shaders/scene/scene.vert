#version 330 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texcoord;

uniform mat4 u_mvp;

out vec3 v_normal;
out vec2 v_texcoord;

void main() {
    v_normal = normalize(a_normal);
    v_texcoord = a_texcoord;
    gl_Position = u_mvp * vec4(a_position, 1.0);
}
