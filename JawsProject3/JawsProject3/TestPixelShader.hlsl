
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


void ComputePointLight(Material mat,
					   PointLight L, 
					   float3 pos,
					   float3 normal,
					   float3 toEye,
					   out float4 ambient,
					   out float4 diffuse, 
					   out float4 spec)
{
	// Initialize outputs.
	ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
	diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
	spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

	


	// The vector from the surface to the light.
	float3 lightVec = L.Position - pos;

	// The distance from surface to light.
	float d = length(lightVec);

	// Range test.
	if(d > L.Range)
		return;

	// Normalize the light vector.
	lightVec /= d;

	// Ambient term.jo

	ambient = mat.Ambient * L.Ambient;

	// Add diffuse and specular term, provided the surface is in
	// the line of site of the light.
	float diffuseFactor = dot(lightVec, normal);

	// Flatten to avoid dynamic branching.
	[flatten]
	if(diffuseFactor > 0.0f)
	{
		float3 v = reflect(-lightVec, normal);  //reflects the light to be pointed towards outwards
		float specFactor = pow(max(dot(v, toEye), 0.0f), mat.Specular.w);  //dot light and eye to get a factor, max it with 0 to get a number >=0, then take the result^(mat spec factor) to get the intensity
		diffuse = diffuseFactor * mat.Diffuse * L.Diffuse;
		spec = specFactor * mat.Specular * L.Specular;
	}

	// Attenuate distance equation
	float att = 1.0f / dot(L.Att, float3(1.0f, d, d*d));
	diffuse *= att;
	spec *= att;
}

SamplerState meshSampler	:register(s0);
Texture2D meshTexture : register (t0);

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

cbuffer MeshBuffer	:register(b2)
{
	Material material;
}


struct PS_INPUT
{
	float4 PosH : SV_POSITION;
	float3 PosW : POSITION;
    float3 Norm : NORMAL;
	float2 Tex : TEXCOORD0;
};



float4 PS(PS_INPUT input) : SV_TARGET
{
	
	
	float3 toEyeW = normalize(gEyePosW - input.PosW);

	float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

	float4 A, D, S;

	ComputePointLight(gMaterial, gPointLight, input.PosW, input.Norm, toEyeW, A, D, S);
	ambient += A;
	diffuse += D;
	spec += S;

	float4 texColor = meshTexture.Sample(meshSampler, input.Tex);
	/*texColor = float4(1,1,1,1);*/

	texColor = texColor * (ambient + diffuse + spec);

	return texColor;
}