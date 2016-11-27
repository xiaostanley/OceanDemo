// 2016-11-27 13:11:52
// Stanley Xiao
// 浅水区域海面顶点着色器

struct VS_INPUT 
{
	float4 ObjPos	: POSITION;   // in object space 
	float2 TexCoord : TEXCOORD0;
};

struct VS_OUTPUT 
{
	float4 ClipPos    : POSITION;	// in clip space 
	float3 rotMatrix1 : TEXCOORD0;	// first row of the 3x3 transform from tangent to obj space
	float3 rotMatrix2 : TEXCOORD1;	// second row of the 3x3 transform from tangent to obj space
	float3 rotMatrix3 : TEXCOORD2;	// third row of the 3x3 transform from tangent to obj space
	float2 bumpCoord0 : TEXCOORD3;
	float2 bumpCoord1 : TEXCOORD4;
	float2 bumpCoord2 : TEXCOORD5;
	float3 eyeVector  : TEXCOORD6;
};

// wave functions
struct Wave 
{
	float freq;  // 2*PI / wavelength
	float amp;   // amplitude
	float phase; // speed * 2*PI / wavelength
	float2 dir;
};

VS_OUTPUT main(VS_INPUT IN,
		//uniform float4x4 WorldViewProj,
		uniform float4x4 World,
		uniform float4x4 ViewProj,
		uniform float3 eyePosition,
		uniform float BumpScale,
		uniform float2 textureScale,
		uniform float2 bumpSpeed,
		uniform float time,
		uniform float waveFreq,
		uniform float waveAmp
        )
{
	VS_OUTPUT OUT;

	#define NWAVES 2
	Wave wave[NWAVES] = 
	{
		{ waveFreq, waveAmp, 0.0, float2(-1, 0) },
		{ waveFreq, waveAmp, 1.2, float2(0, 1) }
	};

    //float4 P = IN.ObjPos;
	float4 P = mul(World, IN.ObjPos);

	// sum waves
	float ddx = 0.0, ddy = 0.0;
	float deriv;
	float angle;

	// wave synthesis using two sine waves at different frequencies and phase shift
	for(int i = 0; i<NWAVES; ++i)
	{
		angle = dot(wave[i].dir, P.xz) * wave[i].freq + time * wave[i].phase;
		P.y += wave[i].amp * sin( angle );

		// calculate derivate of wave function
		deriv = wave[i].freq * wave[i].amp * cos(angle);
		ddx -= deriv * wave[i].dir.x;
		ddy -= deriv * wave[i].dir.y;
	}

	// compute the 3x3 transform from tangent space to object space
	// first rows are the tangent and binormal scaled by the bump scale

	OUT.rotMatrix1.xyz = BumpScale * normalize(float3(1, ddy, 0)); // Binormal
	OUT.rotMatrix2.xyz = BumpScale * normalize(float3(0, ddx, 1)); // Tangent
	OUT.rotMatrix3.xyz = normalize(float3(ddx, 1, ddy)); // Normal

	//OUT.ClipPos = mul(WorldViewProj, P);
	OUT.ClipPos = mul(ViewProj, P);

	// calculate texture coordinates for normal map lookup
	OUT.bumpCoord0.xy = IN.TexCoord*textureScale + time * bumpSpeed;
	OUT.bumpCoord1.xy = IN.TexCoord*textureScale * 2.0 + time * bumpSpeed * 4.0;
	OUT.bumpCoord2.xy = IN.TexCoord*textureScale * 4.0 + time * bumpSpeed * 8.0;

	OUT.eyeVector = P.xyz - eyePosition; // eye position in vertex space
	return OUT;
}
