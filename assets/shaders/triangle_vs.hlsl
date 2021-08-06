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
    float3 normalOS   : NORMAL;
    float3 texCoord0  : TEXCOORD0;
};

struct Varyings
{
    float4 positionCS : SV_POSITION;
    float3 positionWS : VAR_POSITION;
    float3 normalWS   : VAR_NORMAL;
    float2 texCoord0  : VAR_TEXCOORD0;
};


Varyings main(Attributes input)
{
    Varyings output;

    float4 positionWS = mul(_ObjectToWorld, float4(input.positionOS, 1.0f));
    output.positionCS = mul(_CameraProjection, mul(_CameraView, positionWS));
    output.positionWS = positionWS.xyz;
    output.normalWS   = mul((float3x3)_ObjectToWorld, input.normalOS);
    output.texCoord0  = input.texCoord0.xy;

    return output;
}
