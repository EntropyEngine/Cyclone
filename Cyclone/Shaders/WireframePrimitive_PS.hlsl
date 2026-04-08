struct PSInput
{
    float4 PositionPS : SV_Position;
    float3 Color : COLOR;
    uint EntityID : TEXCOORD0;
};

struct PSOutput
{
    float4 Color : SV_Target0;
    uint EntityID : SV_Target1;
};

PSOutput main( PSInput input )
{
    PSOutput output;
    output.Color = float4( input.Color, 1.0f );
    output.EntityID = input.EntityID;
    return output;
}