#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxmath.h>


using namespace DirectX;

#pragma once

class GraphicHelper
{
public:
	GraphicHelper();
	~GraphicHelper();
	HRESULT CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut );
	HRESULT CreateDepthStencilSatates(ID3D11Device* device, ID3D11DeviceContext* deviceContext);

	void TurnOffDepth(ID3D11DeviceContext* deviceContext);
	void TurnOnDepth(ID3D11DeviceContext* deviceContext);
	void Cleanup(ID3D11Device* device);
	struct Terrain
	{
		XMFLOAT3 Pos;
		XMFLOAT2 Tex;
		XMFLOAT2 BoundsY;
	};

	struct Particle
	{
		XMFLOAT3 InitialPosW;
		XMFLOAT3 InitialVelW;
		XMFLOAT2 SizeW;
		float Age;
		UINT Type;

	};


private:
	ID3D11DepthStencilState * onState;
	ID3D11DepthStencilState * offState;
};

