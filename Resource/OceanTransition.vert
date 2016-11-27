// 2016-11-27 12:56:25
// Stanley Xiao
// 过渡区域海面顶点着色器

struct VS_INPUT 
{
	float4 ObjPos	: POSITION;		// in object space 
	float4 Weight	: TANGENT;		// 以切向量存储权重
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
		uniform float waveFreqShallow,
		uniform float waveFreqDeep,
		uniform float waveAmpShallow,
		uniform float waveAmpDeep
        )
{
	VS_OUTPUT OUT;

	#define NWAVES 2
	Wave waveShallow[NWAVES] = 
	{
		{ waveFreqShallow, waveAmpShallow, 0.0, float2(-1, 0) },
		{ waveFreqShallow, waveAmpShallow, 1.2, float2(0, 1) }
	};
	Wave waveDeep[NWAVES] =
	{
		{ waveFreqDeep, waveAmpDeep * 1.5, 0.5, float2(-1, 0) },
		{ waveFreqDeep * 2, waveAmpDeep, 1.7, float2(-0.7, 0.7) }
	};

    //float4 P = IN.ObjPos;
	float4 P = mul(World, IN.ObjPos);

	// sum waves
	float ddx = 0.0, ddy = 0.0;
	float derivShallow, derivDeep;
	float angleShallow, angleDeep;
	float weight = IN.Weight.x;

	// wave synthesis using two sine waves at different frequencies and phase shift
	for(int i = 0; i<NWAVES; ++i)
	{
		angleShallow = dot(waveShallow[i].dir, P.xz) * waveShallow[i].freq + time * waveShallow[i].phase;
		P.y += waveShallow[i].amp * sin(angleShallow) * (1.0 - weight);
		angleDeep = dot(waveDeep[i].dir, P.xz) * waveDeep[i].freq + time * waveDeep[i].phase;
		P.y += waveDeep[i].amp * sin(angleDeep) * weight;

		// calculate derivate of wave function
		derivShallow = waveShallow[i].freq * waveShallow[i].amp * cos(angleShallow);
		ddx -= derivShallow * waveShallow[i].dir.x * (1.0 - weight);
		ddy -= derivShallow * waveShallow[i].dir.y * (1.0 - weight);
		derivDeep = waveDeep[i].freq * waveDeep[i].amp * cos(angleDeep);
		ddx -= derivDeep * waveDeep[i].dir.x * weight;
		ddy -= derivDeep * waveDeep[i].dir.y * weight;
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
