//////Light stuff
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

//////////////CONSTANT BUFFERS/////////////////////////



Texture2D gHeightMap : register (t0);
Texture2D gNormalMap : register (t1);

Texture2D gBlendMap : register (t2);

Texture2D gTexture1 : register (t3);
Texture2D gTexture2 : register (t4);
Texture2D gTexture3 : register (t5);
Texture2D gTexture4 : register (t6);

SamplerState samHeightmap: register( s0 );
SamplerState samTexture: register( s1 );

cbuffer ConstantBuffer	:register(b0)
{
	DirectionalLight gDirLight;
	PointLight gPointLight;
	Material gMaterial;

	float4 gFogColor;
	float gFogStart;
	float gFogRange;
	float2 pad;
}

cbuffer CBPerFrame		:register(b1)
{
	float3 gEyePosW;
	float padding;

	matrix View;
	matrix Projection;

	matrix gViewProj;

	float4 gWorldFrustumPlanes[6];
	//view planes
	
}

cbuffer CBOnButtonPress	:register(b2)
{
	matrix World;

	float gMinDist;
	float gMaxDist;

	float gMinTess;
	float gMaxTess;

	float2 gTexScale; // = 50.0f;
	float paddin1;
	float padding2;
}

/*-----------CONSTANT BUFFES ENDS---------------*/

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


void ComputeDirectionalLight(Material mat,
							 DirectionalLight L,
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

	// The light vector aims opposite the direction the light rays travel.
	float3 lightVec = -L.Direction;

	// Add ambient term.
	ambient = mat.Ambient * L.Ambient;

	// Add diffuse and specular term, provided the surface is in
	// the line of site of the light.
	float diffuseFactor = dot(lightVec, normal);

	// Flatten to avoid dynamic branching.
	[flatten]
	if(diffuseFactor > 0.0f)
	{
		float3 v = reflect(-lightVec, normal);
		float specFactor = pow(max(dot(v, toEye), 0.0f), mat.Specular.w);
		diffuse = diffuseFactor * mat.Diffuse * L.Diffuse;
		spec = specFactor * mat.Specular * L.Specular;
	}
}


/*-----------LIGHT STUFF ENDS----------------------------*/




////////////////FUNCTIONS//////////////////////////////

float CalcTessFactor(float3 p)
{
	float d = distance(p, gEyePosW);

	//Clamps the specified value within the range of 0 to 1.
	float s = saturate((d - gMinDist) / (gMaxDist - gMinDist));

	//lerp() = liner interpolation, takes two points and finds the value between with x returns x + s(y-x) s seems to have to be a value between 0-1 
	return pow(2, (lerp(gMaxTess, gMinTess, s)));
}

bool AabbBehindPlaneTest(float3 center, float3 extents, float4 plane)
{
	float3 n = abs(plane.xyz); // (|n.x|, |n.y|, |n.z|)

	// This is always positive.
	float r = dot(extents, n);

	// signed distance from center point to plane.
	float s = dot(float4(center, 1.0f), plane);

	// If the center point of the box is a distance of r or more behind the
	// plane (in which case s is negative since it is behind the plane),
	// then the box is completely in the negative half space of the plane.
	return (s + r) < 0.0f;
}

// Returns true if the box is completely outside the frustum.
bool AabbOutsideFrustumTest(float3 center, float3 extents, float4 frustumPlanes[6])
{
	for(int i = 0; i < 6; ++i)
	{
		// If the box is completely behind any of the frustum planes
		// then it is outside the frustum.
		if(AabbBehindPlaneTest(center, extents, frustumPlanes[i]))
		{
			return true;
		}
	}
	return false;
}

/*------------FUNCTIONS END----------------------*/
//struct VS_INPUT
//{
//	float3 PosL		: POSITION;
//	float2 Tex		: TEXCOORD0;
//	float2 BoundsY	: BOUNDS;
//};
//
//// Input control point
//struct VS_OUTPUT  //VS_CONTROL_POINT_OUTPUT
//{
//	float3 PosW		: POSITION;  //pre :WORLDPOS
//	float2 Tex		: TEXCOORD0;
//	float2 BoundsY	: BOUNDS;
//
//	// TODO: change/add other stuff
//};
// Output control point
struct HS_CONTROL_POINT_OUTPUT
{
	float3 PosW : WORLDPOS;
	float2 Tex : TEXCOORD0;
	float2 BoundsY : BOUNDS;
};
///////////////////////VERTEX SHADER////////////////////

HS_CONTROL_POINT_OUTPUT VS( HS_CONTROL_POINT_OUTPUT input )
{
	HS_CONTROL_POINT_OUTPUT output;

	// Terrain specified directly in world space.
	output.PosW = input.PosW;

	// Displace the patch corners to world space. This is to make
	// the eye to patch distance calculation more accurate.
	output.PosW.y = gHeightMap.SampleLevel(samHeightmap, input.Tex, 0).r;

	// Output vertex attributes to next stage.
	output.Tex = input.Tex;

	output.BoundsY = input.BoundsY;

	return output;
}

/*-------------------VERTEX SHADER ENDS---------------------*/


///////////////////HULL SHADER/////////////////////////





// Output patch constant data.
struct PatchTess     //pre HS_CONSTANT_DATA_OUTPUT
{
	float EdgeTess[4]			: SV_TessFactor; // e.g. would be [4] for a quad domain
	float InsideTess[2]		: SV_InsideTessFactor; // e.g. would be Inside[2] for a quad domain
	// TODO: change/add other stuff
};


// Patch Constant Function takes in 4 Control points to calculate the patch tesselation factors  (PatchTess), from bounding box kinda like
PatchTess CalcHSPatchConstants(InputPatch<HS_CONTROL_POINT_OUTPUT, 4> patch,
	uint PatchID : SV_PrimitiveID)
{
	PatchTess output;

	// We store the patch BoundsY in the first control point for all patches( done from terrain class)
	float minY = patch[0].BoundsY.x;
	float maxY = patch[0].BoundsY.y;

	// Build axis-aligned bounding box. patch[2] is lower-left corner
	// and patch[1] is upper-right corner.
	float3 vMin = float3(patch[2].PosW.x, minY, patch[2].PosW.z);
	float3 vMax = float3(patch[1].PosW.x, maxY, patch[1].PosW.z);

	// Center/extents representation.
	float3 boxCenter = 0.5f*(vMin + vMax);
	float3 boxExtents = 0.5f*(vMax - vMin);

	//checks if the box is outside of view planes
	if(AabbOutsideFrustumTest(boxCenter, boxExtents, gWorldFrustumPlanes))
	{
		output.EdgeTess[0] = 1.0f;
		output.EdgeTess[1] = 1.0f;
		output.EdgeTess[2] = 1.0f;
		output.EdgeTess[3] = 1.0f;
		output.InsideTess[0] = 1.0f;
		output.InsideTess[1] = 1.0f;
	}
	else        // Do normal tessellation based on distance.
	{
		// It is important to do the tess factor calculation
		// based on the edge properties so that edges shared
		// by more than one patch will have the same
		// tessellation factor. Otherwise, gaps can appear.
		// Compute midpoint on edges, and patch center
		float3 e0 = 0.5f*(patch[0].PosW + patch[2].PosW);
		float3 e1 = 0.5f*(patch[0].PosW + patch[1].PosW);
		float3 e2 = 0.5f*(patch[1].PosW + patch[3].PosW);
		float3 e3 = 0.5f*(patch[2].PosW + patch[3].PosW);
		float3 c = 0.25f*(patch[0].PosW + patch[1].PosW + patch[2].PosW + patch[3].PosW);

		output.EdgeTess[0] = CalcTessFactor(e0);
		output.EdgeTess[1] = CalcTessFactor(e1);
		output.EdgeTess[2] = CalcTessFactor(e2);
		output.EdgeTess[3] = CalcTessFactor(e3);
		output.InsideTess[0] = CalcTessFactor(c);
		output.InsideTess[1] = output.InsideTess[0];
	}



	/*output.EdgeTess[0] = 1.0f;
	output.EdgeTess[1] = 1.0f;
	output.EdgeTess[2] = 1.0f;
	output.EdgeTess[3] = 1.0f;
	output.InsideTess[0] = 1.0f;
	output.InsideTess[1] = 1.0f;*/

	return output;
}

//this is gone thrugh every patch once, to set the values on the control point to next stage
[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("CalcHSPatchConstants")]
HS_CONTROL_POINT_OUTPUT HS( 
	InputPatch<HS_CONTROL_POINT_OUTPUT, 4> ip, 
	uint i : SV_OutputControlPointID,
	uint PatchID : SV_PrimitiveID )
{
	HS_CONTROL_POINT_OUTPUT output;

	// Insert code to compute Output here
	
	output.PosW = ip[i].PosW;
	output.Tex = ip[i].Tex;
	output.BoundsY = ip[i].BoundsY;
	
	

	return output;
}


/*------------HULL SHADER ENDS-----------------------------*/

//////////////////DOMAIN SHENDER/////////////////////////////

struct DS_OUTPUT
{
	float4 PosH : SV_POSITION;
	float3 PosW : POSITION;
	float3 Norm : NORMAL;
	float2 Tex : TEXCOORD0;
	float2 TiledTex : TEXCOORD1;
	float2 uv	: UVSPACE;
	// TODO: change/add other stuff
};

//every quad made from tesselator
[domain("quad")]
DS_OUTPUT DS(	PatchTess input,
	float2 uv : SV_DomainLocation,
	const OutputPatch<HS_CONTROL_POINT_OUTPUT, 4> quad)
{
	DS_OUTPUT output;

	// Bilinear interpolation. lerp() = liner interpolation, takes two points and finds the value between with x returns x + s(y-x) s seems to have to be a value between 0-1 
	output.PosW = lerp(
	lerp(quad[0].PosW, quad[1].PosW, uv.x),
	lerp(quad[2].PosW, quad[3].PosW, uv.x),
	uv.y);

	output.Tex = lerp(
	lerp(quad[0].Tex, quad[1].Tex, uv.x),
	lerp(quad[2].Tex, quad[3].Tex, uv.x),
	uv.y);

	// Tile layer textures over terrain.
	
	output.TiledTex = output.Tex*gTexScale;

	// Displacement mapping

	output.PosW.y = gHeightMap.SampleLevel(samHeightmap, output.Tex, 0).r;


	// Project to homogeneous clip space.
	output.PosH = mul(float4(output.PosW, 1.0f), View);
	output.PosH = mul(output.PosH, Projection);
	output.uv = uv;
	//output.PosH = mul(float4(output.PosW, 1.0f), gViewProj);

	output.Norm = gNormalMap.SampleLevel(samHeightmap, output.Tex, 0).xyz;

	return output;
}


/*-----------------DOMAIN SHENDER ENDs------------------------------------*/


///////////////////////GEOMETRY SHADER/////////////////////////////////////////

struct GSOutput
{
	float4 pos : SV_POSITION;
};

[maxvertexcount(3)]
void GS(
	triangle float4 input[3] : SV_POSITION, 
	inout TriangleStream< GSOutput > output
)
{
	for (uint i = 0; i < 3; i++)
	{
		GSOutput element;
		element.pos = input[i];
		output.Append(element);
	}
}

/*-------------------GEOMETRY SHADER ENDS-------------------------------------------*/

/////////////////////////PIXEL SHADER///////////////////////////////////////////////

float4 PS(DS_OUTPUT input) : SV_TARGET
{
	//get normal

	//float3 normalW = gNormalMap.Sample(samHeightmap, input.Tex).xyz;
	//normalW = gNormalMap.SampleLevel(samHeightmap, input.Tex, 0);
	
	
	//for blendmaps

	// Sample layers in texture array.
	float4 c1 = gTexture1.Sample(samTexture, input.TiledTex);
	float4 c2 = gTexture2.Sample(samTexture, input.TiledTex);
	float4 c3 = gTexture3.Sample(samTexture, input.TiledTex);
	float4 c4 = gTexture4.Sample(samTexture, input.TiledTex);

	// Sample the blend map.
	float4 t = gBlendMap.Sample(samHeightmap, input.Tex);

	// Blend the layers on top of each other.
	
	
	float4 texColor = float4(0,0,1,0);
	texColor = lerp(texColor, c1, t.r);
	texColor = lerp(texColor, c2, t.g);
	texColor = lerp(texColor, c3, t.b);
	texColor = lerp(texColor, c4, t.a);
	

	float3 toEyeW = normalize(gEyePosW - input.PosW);

	float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

	float4 A, D, S;

	
	//ComputeDirectionalLight(gMaterial, gDirLight, input.Norm, toEyeW, A, D, S);
	//ambient += A;
	//diffuse += D;
	//spec += S;

	ComputePointLight(gMaterial, gPointLight, input.PosW, input.Norm, toEyeW, A, D, S);
	ambient += A;
	diffuse += D;
	spec += S;

	texColor = texColor * (ambient + diffuse + spec);

	float distToEye = length(input.PosW - gEyePosW);
	
	float fogLerp = saturate((distToEye - gFogStart) /gFogRange);

	texColor = lerp(texColor, gFogColor, fogLerp);


	return texColor;

}

float4 PSSolid( DS_OUTPUT input) : SV_TARGET
{
	//
	//float4 PosH : SV_POSITION;
	//float3 PosW : POSITION;
	//float2 Tex : TEXCOORD0;
	//float2 TiledTex : TEXCOORD1;
	//// TODO: change/add other stuff
	return float4(1.0f*input.PosW.y/2, 1.0f, 1.0f , 0.5f);
}
/*---------------------PIXEL SHADER ENDS---------------------------------------------*/
