Shader "Engine/ImGui"
{
    State
    {
        // NOTE(v.matushkin): I think some of this is redundant
        Cull Off
        DepthTest Off
        DepthWrite On
        DepthCompare Always
        BlendOp Add, Add
        Blend SrcAlpha OneMinusSrcAlpha, One OneMinusSrcAlpha
    }
    Vertex
    {
        cbuffer vertexBuffer : register(b0)
        {
            float4x4 ProjectionMatrix;
        };


        struct Attributes
        {
            float2 pos : POSITION;
            float2 uv  : TEXCOORD0;
            float4 col : COLOR0;
        };
        struct Varyings
        {
            float4 pos : SV_POSITION;
            float4 col : COLOR0;
            float2 uv  : TEXCOORD0;
        };


        Varyings main(Attributes IN)
        {
            Varyings OUT;

            OUT.pos = mul(ProjectionMatrix, float4(IN.pos.xy, 0.f, 1.f));
            OUT.col = IN.col;
            OUT.uv  = IN.uv;

            return OUT;
        }
    }
    Fragment
    {
        sampler sampler0;
        Texture2D texture0;


        struct Varyings
        {
            float4 pos : SV_POSITION;
            float4 col : COLOR0;
            float2 uv  : TEXCOORD0;
        };


        float4 main(Varyings IN) : SV_Target
        {
            float4 out_col = IN.col * texture0.Sample(sampler0, IN.uv);
            return out_col;
        }
    }
}
