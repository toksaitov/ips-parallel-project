#version 110

uniform sampler2D texture_sampler;

varying vec4 fragment_color;
varying vec2 fragment_texture_coordinates;

void main()
{
    gl_FragColor = texture2D(texture_sampler, fragment_texture_coordinates);
    gl_FragColor.a = 1.0;
}

