#version 330

layout (location = 0) in vec3 in_origin;
layout (location = 1) in vec3 in_vtan;
layout (location = 2) in vec3 in_vbitan;
layout (location = 3) in vec4 in_uvRect;  // vec2 lowerLeft and vec2 upperRight in glyph texture (uv)

out vec3 v_tangent;
out vec3 v_bitangent;
out vec4 v_uvRect;

void main()
{
    gl_Position = vec4(in_origin, 1.0);

    v_tangent   = in_vtan;
    v_bitangent = in_vbitan;
    v_uvRect    = in_uvRect;
}
