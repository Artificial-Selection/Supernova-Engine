#version 460 core

layout(location = 0) out vec3 out_Color;

vec2 positions[3] = vec2[](
    vec2(-0.5, 0.5),
    vec2(0.5, 0.5),
    vec2(0.0, -0.5)
);

vec3 colors[3] = vec3[](
    vec3(0.0, 0.0, 1.0),
    vec3(0.0, 1.0, 0.0),
    vec3(1.0, 0.0, 0.0)
);

void main()
{
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    out_Color = colors[gl_VertexIndex];
}

// layout(location = 0) in vec3 in_PositionOS;
// layout(location = 1) in vec3 in_NormalOS;
// layout(location = 2) in vec3 in_TexCoord0;

// layout(location = 0) out vec3 out_Color;
// layout(location = 1) out vec2 out_TexCoord0;

// layout(binding = 0) uniform CBPerFrame
// {
//     mat4x4 View;
//     mat4x4 Projection;
// } camera;
// layout(binding = 1) uniform CBPerDraw
// {
//     mat4x4 ObjectToWorld;
// } model;


// void main()
// {
//     vec4 positionWS = model.ObjectToWorld * vec4(in_PositionOS, 1.0f);
//     vec3 normalWS = normalize(mat3x3(model.ObjectToWorld) * in_NormalOS).xyz;

//     gl_Position = camera.Projection * camera.View * positionWS;
//     out_Color = normalWS.xyz;
//     out_TexCoord0 = in_TexCoord0.xy;
// }
