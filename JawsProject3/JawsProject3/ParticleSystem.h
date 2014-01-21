#include <d3d11_1.h>
#include "MathHelper.h"
#include "GraphicHelper.h"
#include "Camera.h"

#pragma once


class ParticleSystem
{
public:
	ParticleSystem();
	~ParticleSystem(void);
	HRESULT Initialize( ID3D11Device* device, ID3D11DeviceContext* deviceContext);
	HRESULT CreateRandomTexture1DSRV(ID3D11Device* device);
	HRESULT CreateBuffer( ID3D11Device* device, ID3D11DeviceContext* deviceContext);
	HRESULT CreateVertexInputBuffer(ID3D11Device* device);
	HRESULT CreateGeometryShaders(ID3D11DeviceContext* g_pImmidiateContext, ID3D11Device* device);
	HRESULT CompileVertexShaders(ID3D11Device* device,ID3D11DeviceContext* deviceContext);
	HRESULT CompilePixelShaders(ID3D11Device* device);
	HRESULT CreateTextureSamplerState(ID3D11Device* device);
	void Cleanup(ID3D11Device* device);
	void Reset();
	void Draw(ID3D11DeviceContext* g_pImmidiateContext, ID3D11Device* device, float dt, float gt);

	ID3D11ShaderResourceView*	randomTexSRV;
	ID3D11SamplerState*			textureSamplerState;
private:
	struct CParticleBuffer
	{
		// for when the emit position/direction is varying
		XMFLOAT3 gEmitPosW;
		float gGameTime;
		XMFLOAT3 gEmitDirW;
		float gTimeStep;
	};
	struct CBFixed
	{
		XMFLOAT3 gAccelW;
		float padding2;
	};

	GraphicHelper				graphicHelper;
	ID3D11GeometryShader*		geometryUpdateShader;
	ID3D11VertexShader*			vertexUpdateShader;
	ID3D11GeometryShader*		geometryDrawShader;
	ID3D11VertexShader*			vertexDrawShader;
	ID3D11PixelShader*			pixelDrawShader;

	ID3D11InputLayout*			vertexUpdateLayout;
	ID3D11InputLayout*			vertexDrawLayout;

	ID3D11Buffer*				outputVertexBuffer;
	ID3D11Buffer*				initVertexBuffer;
	ID3D11Buffer*				drawVertexBuffer;

	ID3D11Buffer*				particleBuffer;
	ID3D11Buffer*				fixedBuffer;

	bool mFirstRun;

};

