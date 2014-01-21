
struct Material
{
	float4 Ambient;
	float4 Diffuse;
	float4 Specular; // w = SpecPower
	float4 Reflect;
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

	// Ambient term.
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



Texture2D txDiffuse : register( t0 );
SamplerState samLinear : register( s0 );

cbuffer CBPerFrame : register( b0 )
{
	PointLight gPointLight;

	float3 gEyePosW;
	float padding;
};

cbuffer CBPerObject : register( b1 )
{
	matrix World;
	matrix View;
	matrix Projection;
	
	Material gMaterial;
	float4 vOutputColor;
};



struct VS_INPUT
{
    float4 Pos : POSITION;
    float3 Norm : NORMAL;
	float2 Tex : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 Pos : SV_POSITION;
	float3 Norm : NORMAL;
	float2 Tex : TEXCOORD0;
};




//vertex shader
VS_OUTPUT VS( VS_INPUT input )
{
    VS_OUTPUT output = (VS_OUTPUT)0;

	output.Pos = input.Pos;
	output.Norm = input.Norm;
	output.Tex = input.Tex;

    return output;
}



//pixel shader
float4 PS( VS_OUTPUT input ) : SV_Target
{
	float4 finalColor = 0;
	
	input.Norm = normalize(input.Norm);

	float3 toEyeW = normalize(gEyePosW - input.Pos);

	float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

	float4 A, D, S;

	
	ComputePointLight(gMaterial, gPointLight, input.Pos, input.Norm, toEyeW, A, D, S);
	ambient += A;
	diffuse += D;
	spec += S;
	

	finalColor = ambient + diffuse + spec;


	return txDiffuse.Sample( samLinear, input.Tex )*finalColor;
}

//solid pixel shader
float4 PSSolid( VS_OUTPUT input) : SV_Target
{
    return float4(1,1,1,0);
}

[maxvertexcount(12)]
void GS( triangle VS_OUTPUT input[3], inout TriangleStream<VS_OUTPUT> triStream)
{
	VS_OUTPUT output = (VS_OUTPUT)0;
	matrix WorldViewProj = mul(World,View);
	WorldViewProj = mul(WorldViewProj,Projection);
	//calculate normal
	
	for (int i = 0; i < 3; i++)
	{
		output.Pos = input[i].Pos;
		output.Pos = float4(output.Pos.x , output.Pos.y + 3, output.Pos.z , output.Pos.w);
		output.Pos = mul(output.Pos,WorldViewProj);
		output.Tex = input[i].Tex;
		output.Norm = mul( float4 ( input[i].Norm, 1), World).xyz;
		triStream.Append(output);
	}

	triStream.RestartStrip();

	for (int i = 0; i < 3; i++)
	{
		output.Pos = input[i].Pos;
		output.Pos = float4(output.Pos.x , output.Pos.y - 3, output.Pos.z , output.Pos.w);
		output.Pos = mul(output.Pos,WorldViewProj);
		output.Tex = input[i].Tex;
		output.Norm = mul( float4 ( input[i].Norm, 1), World).xyz;
		triStream.Append(output);
	}

	triStream.RestartStrip();

	for (int i = 0; i < 3; i++)
	{
		output.Pos = input[i].Pos;
		output.Pos = float4(output.Pos.x + 3, output.Pos.y , output.Pos.z , output.Pos.w);
		output.Pos = mul(output.Pos, WorldViewProj);
		output.Pos = float4(output.Pos.x, output.Pos.y, output.Pos.z, output.Pos.w);
		output.Tex = input[i].Tex;
		output.Norm = mul( float4 ( input[i].Norm, 1), World).xyz;
		triStream.Append(output);
	}
	triStream.RestartStrip();

	for (int i = 0; i < 3; i++)
	{
		output.Pos = input[i].Pos;
		output.Pos = float4(output.Pos.x - 3, output.Pos.y , output.Pos.z, output.Pos.w);
		output.Pos = mul(output.Pos, WorldViewProj);
		output.Pos = float4(output.Pos.x, output.Pos.y, output.Pos.z, output.Pos.w);
		output.Tex = input[i].Tex;
		output.Norm = mul( float4 ( input[i].Norm, 1), World).xyz;
		triStream.Append(output);
	}
	triStream.RestartStrip();
}

