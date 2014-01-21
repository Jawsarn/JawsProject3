//***********************************************
// GLOBALS *
//***********************************************

struct Material
{
	float4 Ambient;
	float4 Diffuse;
	float4 Specular; // w = SpecPower
	//float4 Reflect;
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

cbuffer cbPerFrame  :register(b2)
{
	// for when the emit position/direction is varying
	float3 gEmitPosW;
	float gGameTime;
	float3 gEmitDirW;
	float gTimeStep;
};

cbuffer cbFixed  :register(b3)
{
	// Net constant acceleration used to accerlate the particles.
	float3 gAccelW;
	float padding2;
};



// Random texture used to generate random numbers in shaders.
Texture1D gRandomTex :register(t0);

// Array of textures for texturing the particles.
Texture2DArray gTexArray :register(t1);

SamplerState samLinear	:register(s0);///MOVE TO CPU





//***********************************************
// HELPER FUNCTIONS
//***********************************************

float3 RandUnitVec3(float offset)
{
	// Use game time plus offset to sample random texture.
	float u = (gGameTime + offset);
	// coordinates in [-1,1]
	float3 v = gRandomTex.SampleLevel(samLinear, u, 0).xyz;
	// project onto unit sphere
	return normalize(v);
}
float3 RandVec3(float offset)
{
	// Use game time plus offset to sample random texture.
	float u = (gGameTime + offset);
	// coordinates in [-1,1]
	float3 v = gRandomTex.SampleLevel(samLinear, u, 0).xyz;
	return v;
}
//***********************************************
// STREAM-OUT TECH *
//***********************************************
#define PT_EMITTER 0
#define PT_FLARE 1

struct Particle
{
	float3 InitialPosW : POSITION;
	float3 InitialVelW : VELOCITY;
	float2 SizeW : SIZE;
	float Age : AGE;
	uint Type : TYPE;
};

Particle VSUpdate(Particle vin)
{
	return vin;
}

// The stream-out GS is just responsible for emitting
// new particles and destroying old particles. The logic
// programed here will generally vary from particle system
// to particle system, as the destroy/spawn rules will be
// different.
[maxvertexcount(40)]
void GSUpdate(point Particle gin[1],inout PointStream<Particle> ptStream)
{
	gin[0].Age += gTimeStep;
	if(gin[0].Type == PT_EMITTER)
	{
		// time to emit a new particle?
		if(gin[0].Age > 0.002f)
		{
			for(int i = 0; i < 39; ++i)
			{
			// Spread rain drops out above the camera.

			float3 vRandom = 700.0f*RandVec3((float)i/5.0f);
			vRandom.y = 100.0f;

			Particle p;
			p.InitialPosW = gin[0].InitialPosW + vRandom + gEmitPosW;
			p.InitialVelW = gEmitDirW + float3(0,0,0);
			p.SizeW = float2(1.0f, 1.0f);
			p.Age = 0.0f;
			p.Type = PT_FLARE;

			ptStream.Append(p);
			}

		// reset the time to emit
		gin[0].Age = 0.0f;
		}

		// always keep emitters
		ptStream.Append(gin[0]);
	}
	else
	{
		// Specify conditions to keep particle; this may vary
		// from system to system.
		if( gin[0].Age <= 7.0f )
		ptStream.Append(gin[0]);
	}
}























//***********************************************
// DRAW TECH *
//***********************************************
struct VertexOut
{
	float3 PosW : POSITION;
	uint Type : TYPE;
};


struct GeoOut
{
	float4 PosH : SV_Position;
	float2 Tex : TEXCOORD;
};


//GeoOut VSDraw(Particle vin)
//{
//	GeoOut vout;
//	float t = vin.Age;
//
//	// constant acceleration equation
//	float3 v = 0.5f*t*t*gAccelW + t*vin.InitialVelW + vin.InitialPosW;
//	vout.PosH = mul(float4(v, 1.0f), View);
//	vout.PosH = mul(vout.PosH, Projection);
//	vout.Tex = float2(0,0);
//	//vout.Type = vin.Type;
//
//	return vout;
//}

VertexOut VSDraw(Particle vin)
{
	VertexOut vout;
	float t = vin.Age;

	// constant acceleration equation
	vout.PosW = 0.5f*t*t*gAccelW + t*vin.InitialVelW + vin.InitialPosW;

	vout.Type = vin.Type;
	return vout;
}



// The draw GS just expands points into lines.
[maxvertexcount(2)]
void GSDraw(point VertexOut gin[1], inout LineStream<GeoOut> lineStream)
{
	 //do not draw emitter particles.
	if(gin[0].Type != PT_EMITTER)
	{
		// Slant line in acceleration direction.
		float3 p0 = gin[0].PosW;
		float3 p1 = gin[0].PosW + 0.07f*gAccelW;

		GeoOut v0;
		v0.PosH = mul(float4(p0, 1.0f), View);
		v0.PosH = mul(v0.PosH, Projection);
		v0.Tex = float2(0.0f, 0.0f);
		lineStream.Append(v0);

		GeoOut v1;
		v1.PosH = mul(float4(p1, 1.0f), View);
		v1.PosH = mul(v1.PosH, Projection);
		v1.Tex = float2(1.0f, 1.0f);
		lineStream.Append(v1);
	}

}

float4 PSDraw(GeoOut pin) : SV_TARGET
{
	return float4(0.25,0.25,1,0);
	//return gTexArray.Sample(samLinear, float3(pin.Tex, 0));
}
