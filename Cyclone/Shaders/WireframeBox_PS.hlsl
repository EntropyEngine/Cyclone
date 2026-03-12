cbuffer cInstance : register( b1 )
{
    float4x4 gWorld;
    float4 gColor;
}

struct PSInput
{
    float4 PositionPS : SV_Position;
    float3 PositionWS : TEXCOORD0;
};

float4 main( PSInput input ) : SV_Target
{
    return gColor;
}