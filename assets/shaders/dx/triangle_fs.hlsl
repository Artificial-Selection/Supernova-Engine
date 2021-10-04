Texture2D _BaseColorMap        : register(t0);
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
