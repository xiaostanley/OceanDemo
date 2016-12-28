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

#define NUM_NOISE_OCTAVES 5
float hash(float n) 
{ 
	return frac(sin(n) * 1e4); 
}
float hash(float2 p) 
{ 
	return frac(1e4 * sin(17.0 * p.x + p.y * 0.1) * (0.1 + abs(sin(p.y * 13.0 + p.x)))); 
}

float noisePos(float2 x) 
{
    float2 i = floor(x);
    float2 f = frac(x);

	// Four corners in 2D of a tile
	float a = hash(i);
    float b = hash(i + float2(1.0, 0.0));
    float c = hash(i + float2(0.0, 1.0));
    float d = hash(i + float2(1.0, 1.0));

	// Same code, with the clamps in smoothstep and common subexpressions
	// optimized away.
    float2 u = f * f * (3.0 - 2.0 * f);
	return lerp(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

float fbm(float2 x) 
{
	float v = 0.0;
	float a = 0.5;
	float2 shift = float2(100, 100);

	// Rotate to reduce axial bias
    float2x2 rot = float2x2(cos(0.5), sin(0.5), -sin(0.5), cos(0.50));
	for (int i = 0; i < NUM_NOISE_OCTAVES; ++i) {
		v += a * noisePos(x);
		x = mul(rot, x) * 2.0 + shift;
		a *= 0.5;
	}
	return v;
}

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

	#define NSWWAVES 2
	Wave waveShallow[NSWWAVES] = 
	{
		{ waveFreqShallow, waveAmpShallow, 0.0, float2(-1, 0) },
		{ waveFreqShallow, waveAmpShallow, 1.2, float2(0, 1) }
	};
	#define NDPWAVES 3
	Wave waveDeep[NDPWAVES] =
	{
		{ waveFreqDeep, waveAmpDeep, 0.5, float2(-1, 0) },
		{ waveFreqDeep * 0.7, waveAmpDeep * 0.8, 1.7, float2(-0.7, 0.7) },
		{ waveFreqDeep * 0.3, waveAmpDeep * 0.3, 0.9, float2(0, -1)}
	};

    //float4 P = IN.ObjPos;
	float4 P = mul(World, IN.ObjPos);

	// sum waves
	float ddx = 0.0, ddy = 0.0;
	float derivShallow, derivDeep;
	float angleShallow, angleDeep;
	float weight = IN.Weight.x;

	for(int i = 0; i<NSWWAVES; ++i)
	{
		angleShallow = dot(waveShallow[i].dir, P.xz) * waveShallow[i].freq + time * waveShallow[i].phase;
		P.y += waveShallow[i].amp * sin(angleShallow) * (1.0 - weight);
		derivShallow = waveShallow[i].freq * waveShallow[i].amp * cos(angleShallow);
		ddx -= derivShallow * waveShallow[i].dir.x * (1.0 - weight);
		ddy -= derivShallow * waveShallow[i].dir.y * (1.0 - weight);
	}

	for(int i = 0; i<NDPWAVES; ++i)
	{
		// 2016-12-11 14:55:52 随机高度
		float rd = fbm(P.xz - float2(time * 0.5, time * 0.5));

		angleDeep = dot(waveDeep[i].dir, P.xz) * waveDeep[i].freq + time * waveDeep[i].phase;
		P.y += (waveDeep[i].amp * sin(angleDeep) + 0.25 * rd )* weight;

		derivDeep = waveDeep[i].freq * waveDeep[i].amp * cos(angleDeep);
		ddx -= derivDeep * waveDeep[i].dir.x * weight;
		ddy -= derivDeep * waveDeep[i].dir.y * weight;
	}

	// compute the 3x3 transform from tangent space to object space
	// first rows are the tangent and binormal scaled by the bump scale

	OUT.rotMatrix1.xyz = BumpScale * normalize(float3(1, ddy, 0)); // Binormal
	OUT.rotMatrix2.xyz = BumpScale * normalize(float3(0, ddx, 1)); // Tangent
	OUT.rotMatrix3.xyz = normalize(float3(ddx, 1, ddy)); // Normal

	OUT.ClipPos = mul(ViewProj, P);

	// calculate texture coordinates for normal map lookup
	OUT.bumpCoord0.xy = IN.TexCoord*textureScale + time * bumpSpeed;
	OUT.bumpCoord1.xy = IN.TexCoord*textureScale * 2.0 + time * bumpSpeed * 4.0;
	OUT.bumpCoord2.xy = IN.TexCoord*textureScale * 4.0 + time * bumpSpeed * 8.0;

	OUT.eyeVector = IN.ObjPos.xyz - eyePosition; // eye position in vertex space
	return OUT;
}
