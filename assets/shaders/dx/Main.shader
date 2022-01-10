Shader "Engine/Main"
{
    Vertex
    {
        cbuffer PerFrame : register(b0)
        {
            float4x4 _CameraView;
            float4x4 _CameraProjection;
        };
        cbuffer PerDraw : register(b1)
        {
            float4x4 _ObjectToWorld;
        };


        struct Attributes
        {
            float3 positionOS : POSITION;
            float3 normalOS : NORMAL;
            float3 texCoord0 : TEXCOORD0;
        };
        struct Varyings
        {
            float4 positionCS : SV_POSITION;
            float3 positionWS : VAR_POSITION;
            float3 normalWS   : VAR_NORMAL;
            float2 texCoord0  : VAR_TEXCOORD0;
        };


        Varyings main(Attributes IN)
        {
            Varyings OUT;

            float4 positionWS = mul(_ObjectToWorld, float4(IN.positionOS, 1.0f));
            OUT.positionCS    = mul(_CameraProjection, mul(_CameraView, positionWS));
            OUT.positionWS    = positionWS.xyz;
            OUT.normalWS      = mul((float3x3) _ObjectToWorld, IN.normalOS);
            OUT.texCoord0     = IN.texCoord0.xy;

            return OUT;
        }
    }
    Fragment
    {
        Texture2D _BaseColorMap : register(t0);
        sampler   sampler_BaseColorMap : register(s0);


        struct Varyings
        {
            float4 positionCS : SV_POSITION;
            float3 positionWS : VAR_POSITION;
            float3 normalWS   : VAR_NORMAL;
            float2 texCoord0  : VAR_TEXCOORD0;
        };


        float4 main(Varyings input) : SV_TARGET
        {
            float3 normalWS = normalize(input.normalWS);

            float4 baseColor = _BaseColorMap.Sample(sampler_BaseColorMap, input.texCoord0);

            return baseColor;
            // return float4(normalWS, 1);
        }
    }
}
