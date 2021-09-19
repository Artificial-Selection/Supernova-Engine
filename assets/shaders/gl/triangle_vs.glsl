#version 460 core

layout(location = 0) in vec3 in_PositionOS;
layout(location = 1) in vec3 in_NormalOS;
layout(location = 2) in vec3 in_TexCoord0;

layout(location = 0) out vec3 out_Color;
layout(location = 1) out vec2 out_TexCoord0;

uniform mat4x4 _ObjectToWorld;
uniform mat4x4 _MatrixP;
uniform mat4x4 _MatrixV;


void main()
{
    vec4 positionWS = _ObjectToWorld * vec4(in_PositionOS, 1.0f);
    vec3 normalWS = normalize(mat3x3(_ObjectToWorld) * in_NormalOS).xyz;

    gl_Position = _MatrixP * _MatrixV * positionWS;
    out_Color = normalWS.xyz;
    out_TexCoord0 = in_TexCoord0.xy;
}
