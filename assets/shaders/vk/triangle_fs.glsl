#version 460 core

layout(set = 0, binding = 2) uniform sampler   s_Sampler;
layout(set = 1, binding = 0) uniform texture2D _BaseColorMap;


layout(location = 0) in vec3 in_PositionWS;
layout(location = 1) in vec3 in_NormalWS;
layout(location = 2) in vec2 in_TexCoord0;

layout(location = 0) out vec4 out_FragColor;


void main()
{
    vec3 normalWS = normalize(in_NormalWS);

    vec4 baseColor = texture(sampler2D(_BaseColorMap, s_Sampler), in_TexCoord0);

    out_FragColor = baseColor; //vec4(normalWS, 1);
}
