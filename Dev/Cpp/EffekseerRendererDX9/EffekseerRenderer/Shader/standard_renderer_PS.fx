
Texture2D	g_texture		: register( t0 );
sampler2D	g_sampler		: register( s0 );


struct PS_Input
{
	float4 Color		: COLOR;
	float2 UV		: TEXCOORD0;
};


float4 PS( const PS_Input Input ) : COLOR
{
	float4 Output = Input.Color * tex2D(g_sampler, Input.UV);
	return Output;
}
