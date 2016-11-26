float4x4         mViewProj;
float4x4         mView;
float4        view_position;
float3         watercolour;
float             LODbias;
float             sun_alfa, sun_theta, sun_shininess, sun_strength;
float             reflrefr_offset;
bool             diffuseSkyRef;

struct VS_INPUT
{
    float3 Pos : POSITION;
    float3 Normal : NORMAL;
    float2 tc : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 Pos : POSITION;
    float2 tc : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 viewvec : TEXCOORD2;
    float3 screenPos : TEXCOORD3;
    float3 sun : TEXCOORD5;
    float3 worldPos : TEXCOORD6;
};

VS_OUTPUT VShaderR300(VS_INPUT i)
{
    VS_OUTPUT o;
    o.worldPos = i.Pos.xyz/4;
    o.Pos = mul(float4(i.Pos.xyz,1), mViewProj);
    o.normal = normalize(i.Normal.xyz);
    o.viewvec = normalize(i.Pos.xyz - view_position.xyz/view_position.w);

    o.tc = i.tc;

    // alt screenpos
    // this is the screenposition of the undisplaced vertices (assuming the plane is y=0)
    // it is used for the reflection/refraction lookup
    float4 tpos = mul(float4(i.Pos.x,0,i.Pos.z,1), mViewProj);
    o.screenPos = tpos.xyz/tpos.w;
    o.screenPos.xy = 0.5 + 0.5*o.screenPos.xy*float2(1,-1);
    o.screenPos.z = reflrefr_offset/o.screenPos.z; // reflrefr_offset controls
                        //the strength of the distortion

    // what am i doing here? (this should really be done on the CPU as it isn¡¯t a per-vertex operation)
    o.sun.x = cos(sun_theta)*sin(sun_alfa);
    o.sun.y = sin(sun_theta);
    o.sun.z = cos(sun_theta)*cos(sun_alfa);
    return o;
}
