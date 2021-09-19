#version 460 core

layout(location = 0) in vec3 in_Color;
layout(location = 1) in vec2 in_TexCoord0;

layout(location = 0) out vec4 out_FragColor;

layout(binding = 0) uniform sampler2D _DiffuseTexture;


void main()
{
    vec4 textureColor = texture(_DiffuseTexture, in_TexCoord0);
    out_FragColor = textureColor;
    //out_FragColor = vec4(in_Color, 1.0f);
}
