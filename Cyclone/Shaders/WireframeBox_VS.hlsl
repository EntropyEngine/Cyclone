struct VSInput
{
    float3 Position : SV_Position;
    float4 Center: InstCenter;
    float4 Extent: InstExtent;
    float4 Color: InstColor;
};

struct VSOutput
{
    float4 PositionPS : SV_Position;
    float4 Color : COLOR;
};

cbuffer cViewProj : register( b0 )
{
    float4x4 gViewProj;
}

VSOutput main( VSInput input )
{
    float4x4 world = float4x4(
        input.Extent.x, 0, 0, 0,
        0, input.Extent.y, 0, 0,
        0, 0, input.Extent.z, 0,
        input.Center.x, input.Center.y, input.Center.z, 1
    );
    float4 PositionWS = mul( float4( input.Position, 1.0f ), world );
    float4 PositionPS = mul( PositionWS, gViewProj );
    
    VSOutput output;
    output.PositionPS = PositionPS;
    output.Color = input.Color;
    
    return output;
}