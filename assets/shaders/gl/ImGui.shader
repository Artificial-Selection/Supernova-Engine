Shader "Engine/ImGui"
{
    State
    {
        Cull Off
        DepthTest Off
        DepthWrite On
        DepthCompare Always
        BlendOp Add, Add
        Blend SrcAlpha OneMinusSrcAlpha, One OneMinusSrcAlpha
    }
    Vertex
    {
        #version 460 core

        layout(location = 0) in vec2 in_PositionOS;
        layout(location = 1) in vec2 in_TexCoord0;
        layout(location = 2) in vec4 in_Color;

        layout(location = 0) out vec2 out_TexCoord0;
        layout(location = 1) out vec4 out_Color;

        uniform mat4x4 ProjMtx;


        void main()
        {
            gl_Position   = ProjMtx * vec4(in_PositionOS.xy, 0, 1);
            out_TexCoord0 = in_TexCoord0;
            out_Color     = in_Color;
        }
    }
    Fragment
    {
        #version 460 core

        layout(location = 0) in vec2 in_TexCoord0;
        layout(location = 1) in vec4 in_Color;

        layout(location = 0) out vec4 out_FragColor;

        layout(binding = 0) uniform sampler2D Texture;


        void main()
        {
            out_FragColor = in_Color * texture(Texture, in_TexCoord0);
        }
    }
}
