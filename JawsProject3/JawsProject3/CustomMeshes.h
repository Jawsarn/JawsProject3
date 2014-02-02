#include <d3d11_1.h>
#include "FileReader.h"

#pragma once


class CustomMeshes
{
public:
	CustomMeshes(void);
	~CustomMeshes(void);
	HRESULT Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext);
	void Draw(ID3D11DeviceContext* deviceContext);
private:
	GraphicHelper graphicHelper;
	FileReader fileReader;
	ID3D11Buffer* vertexBuffer;
	std::vector<int> vertCount;
	HRESULT CreateSimpleShaders(ID3D11Device* device, ID3D11DeviceContext* deviceContext);
	HRESULT CreateSamplerState(ID3D11Device* device, ID3D11DeviceContext* deviceContext);
	HRESULT CreateBuffers(ID3D11Device* device, ID3D11DeviceContext* deviceContext);

	ID3D11VertexShader*			vertexShader;
	ID3D11PixelShader*			pixelShader;

	ID3D11InputLayout*			vertexLayout;
	ID3D11SamplerState*			samplerState;
	ID3D11Buffer*				meshBuffer;


	std::vector<Material> materials;
	std::vector<ID3D11ShaderResourceView*> textureSRVs;

	struct MeshBuffer
	{
		Material Material;

		//moved frustrums to frame
	};

};

