#include <d3d11_1.h>
#include <string>
#include <vector>
#include <fstream>
#include <array>
#include <algorithm>
#include "GraphicHelper.h"
#include "DDSTextureLoader.h"

class Terrain
{
public:
	//functions
	Terrain();
	~Terrain();

	HRESULT Initialize(ID3D11Device * device, ID3D11DeviceContext* deviceContext);  //initialize
	
	void Draw(ID3D11DeviceContext* deviceContext);
	void Cleanup(ID3D11Device* device);
	
	
private:
	struct InitInfo
	{
		//height map file name
		std::wstring HeightmapFileName;

		//texture file names
		std::wstring LayerMapFilename1;
		std::wstring LayerMapFilename2;
		std::wstring LayerMapFilename3;
		std::wstring LayerMapFilename4;
		std::wstring BlenMapFileName;

		//scale to add on the height (365 bit parts to when they've been loaded)
		float HeightScale;
		
		//dimensions
		UINT HeightmapWidth;
		UINT HeightmapHeight;

		//spacing
		float CellSpacing;
	};



	struct CBOnButtonPress
	{
		XMMATRIX World;

		float gMinDist;
		float gMaxDist;

		float gMinTess;
		float gMaxTess;

		XMFLOAT2 gTexScale; // = 50.0f;
		float paddin1;
		float padding2;
	};

	InitInfo mInfo;
	GraphicHelper graphicHelper;

	UINT mNumPatchVertices;
	UINT mNumPatchQuadFaces;
	UINT mNumPatchVerticesRows;
	UINT mNumPatchVerticesCols;
	static const int CellsPerPatch = 64;

	std::vector<float> mHeightmap;
	std::vector<XMFLOAT4> mBlendMap;
	std::vector<XMFLOAT3> mNormalmap;
	std::vector<XMFLOAT2> mPatchBoundsY;

	ID3D11Buffer*				g_pQuadPatchVertexBuffer;
	ID3D11Buffer*				g_pQuadPatchIndexBuffer;
	ID3D11SamplerState*			g_pQuadSamplerState;
	ID3D11SamplerState*			g_pTextureSamplerState;

	ID3D11ShaderResourceView* mHeightMapSRV;
	ID3D11ShaderResourceView* mNormalMapSRV;
	ID3D11ShaderResourceView* mBlendMapSRV;
	ID3D11ShaderResourceView* mTexture1SRV;
	ID3D11ShaderResourceView* mTexture2SRV;
	ID3D11ShaderResourceView* mTexture3SRV;
	ID3D11ShaderResourceView* mTexture4SRV;

	////FUNCTIONS

	HRESULT LoadTextures(ID3D11Device * device);

	HRESULT BuildBlendMapShaderResourceView(std::vector<XMFLOAT4>& inblendMap, ID3D11Device * device);
	HRESULT BuildHeightmapShaderResourceView(ID3D11Device * device); //create heightmap shader resource view
	HRESULT BuildNormalmapShaderResourceView(ID3D11Device * device);
	HRESULT BuildQuadPatchVertexBuffer(ID3D11Device *device, ID3D11DeviceContext* deviceContext);	//create vertex buffer for quad patch
	HRESULT BuildQuadPatchIndexBuffer(ID3D11Device *device, ID3D11DeviceContext* deviceContext);	//create index buffer for quad patch
	HRESULT CreateHeightmapSamplerState(ID3D11Device* device);	//create sampler state for the heightmap
	HRESULT CreateTextureSamplerState(ID3D11Device* device);

	XMFLOAT3 CalculateNormal(float i, float j, float k , float l, float m, float n);

	void CalculateNormalMap(void);
	void CalcAllPatchBoundsY(void);
	void CalcPatchBoundsY(UINT i, UINT j);
	HRESULT InitializeCBuffers(ID3D11Device * device,ID3D11DeviceContext* deviceContext);
	
	void LoadBlendMap(void);

	void InitializeInfo();
	void LoadHeightmap(void); //loads a raw heightmap
	
	void Smooth(void); //smooth the heightmaps heights
	bool InBounds(int m, int n);  //check if vertice is inbounds of array
	float Average(float i, float j); //average the heights to Smooth function

	float GetWidth(void)const;
	float GetDepth(void)const;

	ID3D11Buffer* g_CBOnButtonPress;
	XMMATRIX g_World;

	ID3D11VertexShader*			g_pVertexShader;
	ID3D11PixelShader*			g_pPixelShader;
	ID3D11GeometryShader*		g_pGeometryShader;
	ID3D11HullShader*			g_pHullShader;
	ID3D11DomainShader*			g_pDomainShader;
	ID3D11InputLayout*			g_pVertexLayout;

	HRESULT CompileVertexShaders(ID3D11Device* device,ID3D11DeviceContext* deviceContext);
	HRESULT CompilePixelShaders(ID3D11Device* device);
	HRESULT CompileHullShaders(ID3D11Device* device);
	HRESULT CompileDomainShaders(ID3D11Device* device);
	HRESULT CompileGeometryShaders(ID3D11Device* device);
};

