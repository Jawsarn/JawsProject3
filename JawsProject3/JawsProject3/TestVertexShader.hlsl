struct Material
{
	float4 Ambient;
	float4 Diffuse;
	float4 Specular; // w = SpecPower
	//float4 Reflect;
	float4 Transmission;
};

struct PointLight
{
	float4 Ambient;
	float4 Diffuse;
	float4 Specular;

	float3 Position;
	float Range;

	float3 Att;
	float pad;
};

struct DirectionalLight
{
	float4 Ambient;
	float4 Diffuse;
	float4 Specular;
	float3 Direction;
	float pad;
};

cbuffer ConstantBuffer	:register(b0)
{
	DirectionalLight gDirLight;
	PointLight gPointLight;
	Material gMaterial;

	float4 gFogColor;
	float gFogStart;
	float gFogRange;
	float2 pad;
};

cbuffer CBPerFrame		:register(b1)
{
	float3 gEyePosW;
	float padding;

	matrix View;
	matrix Projection;

	matrix gViewProj;

	float4 gWorldFrustumPlanes[6];
	//view planes
	
};

struct VS_INPUT
{
	float3 PosW : POSITION;
    float3 Norm : NORMAL;
	float2 Tex : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 PosH : SV_POSITION;
	float3 PosW : POSITION;
    float3 Norm : NORMAL;
	float2 Tex : TEXCOORD0;
};

VS_OUTPUT VS( VS_INPUT input )
{
	VS_OUTPUT output;
	output.PosH = mul(float4(input.PosW, 1.0f), View);
	output.PosH = mul(output.PosH, Projection);
	output.PosW = input.PosW;
	output.Norm = input.Norm;
	output.Tex = input.Tex;
	return output;
}