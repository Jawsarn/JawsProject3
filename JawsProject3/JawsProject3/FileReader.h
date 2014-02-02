#include <d3d11_1.h>
#include <DirectXMath.h>
#include "GraphicHelper.h"
#include "LightHelper.h"
#include "DDSTextureLoader.h"

#include <vector>
#include <istream>
#include <fstream> 
#include <string> 


#pragma once

using namespace DirectX;

class FileReader
{
public:

	FileReader(void);
	~FileReader(void);
	HRESULT ReadObjFile(std::string filename,		//.obj filename
	ID3D11Buffer* vertBuff,			//mesh vertex buffer
	ID3D11Device* device,
	ID3D11DeviceContext* deviceContext,
	std::vector<std::vector<GraphicHelper::SimpleVertex>> &vertices,
	std::vector<Material> &outMaterials,
	std::vector<ID3D11ShaderResourceView*> &textureViews
	);

	HRESULT ReadMaterial(ID3D11Device* device,
		std::string filename, 
		std::vector<std::string> materialNames, 
		std::vector<Material> &outMaterials, 
		std::vector<ID3D11ShaderResourceView*> &textureViews);

	//things I will need move elsewhere
	ID3D11BlendState* transparency;
	ID3D11Buffer*	meshVertBuff;
	ID3D11Buffer*	meshIndexBuffer;
	
	XMMATRIX meshWorld;

	std::vector<int> meshSubsetIndexStart;
	std::vector<int> meshSubsetTexture;
	
	void Cleanup();

	struct Group
	{
		std::string groupName;
		std::string materialName;
		std::vector<unsigned int> vertexIndices;
		std::vector<unsigned int> uvIndices;
		std::vector<unsigned int> normalIndices;
	};
	struct MatGroup
	{
		std::string name;
		Material mMaterial;
		ID3D11ShaderResourceView* tempResource;
	};
};

