struct VSInput
{
    float3 Position : SV_Position;
    float3 Color: COLOR;
    uint EntityID: EntityID;
};

struct VSOutput
{
    float4 PositionPS : SV_Position;
    float3 Color : COLOR;
    uint EntityID : TEXCOORD0;
};

cbuffer cViewProj : register( b0 )
{
    float4x4 gViewProj;
}

VSOutput main( VSInput input )
{
    float4 PositionPS = mul( float4( input.Position, 1.0f ), gViewProj );
    
    VSOutput output;
    output.PositionPS = PositionPS;
    output.Color = input.Color;
    output.EntityID = input.EntityID;
    
    return output;
}