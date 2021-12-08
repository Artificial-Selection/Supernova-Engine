Shader "Engine/Main"
{
    Vertex
    {
        #version 460 core

        layout(set = 0, binding = 0) uniform PerFrame
        {
            mat4x4 View;
            mat4x4 Projection;
        }
        ub_Camera;

        layout(set = 0, binding = 1) uniform PerDraw { mat4x4 ObjectToWorld; }
        ub_Model;


        layout(location = 0) in vec3 in_PositionOS;
        layout(location = 1) in vec3 in_NormalOS;
        layout(location = 2) in vec3 in_TexCoord0;

        layout(location = 0) out vec3 out_PositionWS;
        layout(location = 1) out vec3 out_NormalWS;
        layout(location = 2) out vec2 out_TexCoord0;


        void main()
        {
            vec4 positionWS = ub_Model.ObjectToWorld * vec4(in_PositionOS, 1.0);

            gl_Position   = ub_Camera.Projection * ub_Camera.View * positionWS;
            gl_Position.y = -gl_Position.y;

            out_PositionWS = positionWS.xyz;
            out_NormalWS   = mat3x3(ub_Model.ObjectToWorld) * in_NormalOS;
            out_TexCoord0  = in_TexCoord0.xy;
        }
    }
    Fragment
    {
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

            out_FragColor = baseColor; // vec4(normalWS, 1);
        }

    }
}
