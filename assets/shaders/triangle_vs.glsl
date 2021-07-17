#version 460 core

layout(location = 0) in vec3 a_PositionOS;
layout(location = 1) in vec3 a_NormalOS;

out vec3 outColor;

uniform mat4x4 _ObjectToWorld;
uniform mat4x4 _MatrixP;
uniform mat4x4 _MatrixV;


void main()
{
    vec4 positionWS = _ObjectToWorld * vec4(a_PositionOS, 1.0f);
    vec3 normalWS = normalize(mat3x3(_ObjectToWorld) * a_NormalOS).xyz;

    gl_Position = _MatrixP * _MatrixV * positionWS;
    outColor = normalWS.xyz;
}
