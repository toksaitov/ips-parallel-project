#version 110

attribute vec4 position;
attribute vec3 normal;
attribute vec4 color;
attribute vec2 texture_coordinates;

uniform mat4 model_view_projection_matrix;

varying vec4 fragment_color;
varying vec2 fragment_texture_coordinates;

void main()
{
    fragment_color = color;
    fragment_texture_coordinates = texture_coordinates;

    gl_Position = model_view_projection_matrix * position;
}

