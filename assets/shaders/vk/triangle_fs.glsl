#version 460 core

// layout(binding = 0) uniform sampler2D _DiffuseTexture;

layout(location = 0) in vec3 in_PositionWS;
layout(location = 1) in vec3 in_NormalWS;
layout(location = 2) in vec2 in_TexCoord0;

layout(location = 0) out vec4 out_FragColor;


void main()
{
    vec3 normalWS = normalize(in_NormalWS);

    out_FragColor = vec4(normalWS, 1);
}
