/*********************************************************************NVMH3****
Comments:
	Simple ocean shader with animated bump map and geometric waves
	Based partly on "Effective Water Simulation From Physical Models", GPU Gems

******************************************************************************/



struct VS_OUTPUT  {
	float4 ClipPos    : POSITION;  // in clip space
	float3 rotMatrix1 : TEXCOORD0; // first row of the 3x3 transform from tangent to obj space
	float3 rotMatrix2 : TEXCOORD1; // second row of the 3x3 transform from tangent to obj space
	float3 rotMatrix3 : TEXCOORD2; // third row of the 3x3 transform from tangent to obj space

	float2 bumpCoord0 : TEXCOORD3;
	float2 bumpCoord1 : TEXCOORD4;
	float2 bumpCoord2 : TEXCOORD5;

	float3 eyeVector  : TEXCOORD6;
};


float4 main(VS_OUTPUT IN,
			uniform sampler2D NormalMap,
			uniform samplerCUBE EnvironmentMap,
			uniform float4 deepColor,
			uniform float4 shallowColor,
			uniform float4 reflectionColor,
			uniform float reflectionAmount,
			uniform float reflectionBlur,
			uniform float waterAmount,
			uniform float fresnelPower,
			uniform float fresnelBias,
			uniform float hdrMultiplier
			) : COLOR
{
	// sum normal maps
	// sample from 3 different points so no texture repetition is noticeable
    float4 t0 = tex2D(NormalMap, IN.bumpCoord0) * 2.0 - 1.0;
    float4 t1 = tex2D(NormalMap, IN.bumpCoord1) * 2.0 - 1.0;
    float4 t2 = tex2D(NormalMap, IN.bumpCoord2) * 2.0 - 1.0;
    float3 N = t0.xyz + t1.xyz + t2.xyz;

    float3x3 m; // tangent to world matrix
    m[0] = IN.rotMatrix1;
    m[1] = IN.rotMatrix2;
    m[2] = IN.rotMatrix3;

    N = normalize( mul( N, m ) );

	// reflection
    float3 E = normalize(IN.eyeVector);
    float4 R;
    R.xyz = reflect(E, N);

    // Ogre conversion for cube map lookup
    R.z = -R.z;
    R.w = reflectionBlur;
    float4 reflection = texCUBEbias(EnvironmentMap, R);

    // cheap hdr effect
    reflection.rgb *= (reflection.r + reflection.g + reflection.b) * hdrMultiplier;

	// fresnel
    float facing = 1.0 - max(dot(-E, N), 0);
    float fresnel = saturate(fresnelBias + pow(facing, fresnelPower));

    float4 waterColor = lerp(shallowColor, deepColor, facing) * waterAmount;

    reflection = lerp(waterColor,  reflection * reflectionColor, fresnel) * reflectionAmount;
    return (waterColor + reflection);
}
