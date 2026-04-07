cbuffer ScreenData : register( b0 )
{
    uint gMouseX;
    uint gMouseY;
    uint gScreenW;
    uint gScreenH;
};

#if MSAA_LEVEL == 1
Texture2D<uint> gEntityBuffer : register( t0 );
#else
Texture2DMS<uint, MSAA_LEVEL> gEntityBuffer : register( t0 );
#endif

#define MOUSE_WIDTH 8

#define TOTAL_THREADS (MOUSE_WIDTH * MOUSE_WIDTH * MSAA_LEVEL)

RWStructuredBuffer<uint> gOutputBuffer : register( u0 );

[numthreads( MOUSE_WIDTH, MOUSE_WIDTH, MSAA_LEVEL )]
void main( uint groupIndex : SV_GroupIndex )
{
    uint pixelOffsetX = groupIndex % MOUSE_WIDTH;
    uint pixelOffsetY = ( groupIndex / MOUSE_WIDTH ) % MOUSE_WIDTH;
    uint pixelSample = groupIndex / MOUSE_WIDTH / MOUSE_WIDTH;

    int pixelX = gMouseX;
    pixelX -= MOUSE_WIDTH / 2;
    pixelX += pixelOffsetX;

    int pixelY = gMouseY;
    pixelY -= MOUSE_WIDTH / 2;
    pixelY += pixelOffsetY;
    
    pixelX = clamp( pixelX, 0, gScreenW - 1 );
    pixelY = clamp( pixelY, 0, gScreenH - 1 );
    
#if MSAA_LEVEL == 1
    gOutputBuffer[groupIndex] = gEntityBuffer.Load( int3( pixelX, pixelY, 0 ) );
#else
    gOutputBuffer[groupIndex] = gEntityBuffer.Load( int2( pixelX, pixelY ), pixelSample );
#endif

}