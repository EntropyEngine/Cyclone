cbuffer cViewProj : register( b0 )
{
    float4x4 gViewProj;
}

cbuffer cInstance : register( b1 )
{
    float4x4 gWorld;
    float4 gColor;
}

struct VSInput
{
    float3 Position : SV_Position;
};

struct VSOutput
{
    float4 PositionPS : SV_Position;
    float4 Color : COLOR;
};

VSOutput main( VSInput input )
{
    float4 PositionWS = mul( float4( input.Position, 1.0f ), gWorld );
    float4 PositionPS = mul( PositionWS, gViewProj );
    
    VSOutput output;
    output.PositionPS = PositionPS;
    output.Color = gColor;
    
    return output;
}