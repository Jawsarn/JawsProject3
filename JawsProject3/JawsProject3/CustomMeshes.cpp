#include "CustomMeshes.h"


CustomMeshes::CustomMeshes(void)
{
}


CustomMeshes::~CustomMeshes(void)
{
}

HRESULT CustomMeshes::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
	HRESULT hr = S_OK;
	std::vector<std::vector<GraphicHelper::SimpleVertex>> vertices;


	graphicHelper.CreateDepthStencilSatates(device,deviceContext);

	fileReader = FileReader();
	fileReader.ReadObjFile("BTH.obj",vertexBuffer,device,deviceContext, vertices,materials,textureSRVs);
	hr = CreateSimpleShaders(device,deviceContext);
	if( FAILED( hr ) )
        return hr;

	hr = CreateSamplerState(device,deviceContext);
	if( FAILED( hr ) )
        return hr;

	hr = CreateBuffers(device, deviceContext);
	if( FAILED( hr ) )
        return hr;

	
	
	std::vector<GraphicHelper::SimpleVertex> tempVert = vertices[0];
	vertCount.push_back(tempVert.size());


	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(GraphicHelper::SimpleVertex) * tempVert.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &tempVert[0];
	hr = (device->CreateBuffer(&vbd, &vinitData, &vertexBuffer));

	return hr;
}
HRESULT CustomMeshes::CreateSimpleShaders(ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
	HRESULT hr = S_OK;
	
	//CREATE SHADER 1
	ID3DBlob* pVSBlob = nullptr;
    hr = graphicHelper.CompileShaderFromFile( L"TestVertexShader.hlsl", "VS", "vs_5_0", &pVSBlob );
	
    if( FAILED( hr ) )
    {
        MessageBox( nullptr,
                    L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }

	// Create the vertex shader
	hr = device->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &vertexShader );
	if( FAILED( hr ) )
	{	
		pVSBlob->Release();
        return hr;
	}

	//input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE( layout );

     // Create the input layout
	hr = device->CreateInputLayout( layout, numElements, pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(), &vertexLayout );

	pVSBlob->Release();
	if( FAILED( hr ) )
        return hr;


	//create pixel shader
	hr = graphicHelper.CompileShaderFromFile( L"TestPixelShader.hlsl", "PS", "ps_5_0", &pVSBlob );
	
    if( FAILED( hr ) )
    {
        MessageBox( nullptr,
                    L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }

	// Create the vertex shader
	hr = device->CreatePixelShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &pixelShader );
	if( FAILED( hr ) )
	{	
		pVSBlob->Release();
        return hr;
	}
	return hr;

}

HRESULT CustomMeshes::CreateSamplerState(ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
	HRESULT hr = S_OK;

	D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory( &sampDesc, sizeof(sampDesc) );
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = device->CreateSamplerState( &sampDesc, &samplerState);

	return hr;
}

HRESULT CustomMeshes::CreateBuffers(ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
	HRESULT hr = S_OK;

	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(MeshBuffer);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;

	hr = device->CreateBuffer( &bd, nullptr, &meshBuffer );
    if( FAILED( hr ) )
        return hr;
}

void CustomMeshes::Draw(ID3D11DeviceContext* deviceContext)
{
	MeshBuffer mesh;
	mesh.Material = materials[0];
	deviceContext->UpdateSubresource(meshBuffer,0, nullptr,&mesh,0,0);
	graphicHelper.TurnOnDepth(deviceContext);
	

	//set input right
	UINT stride = sizeof( GraphicHelper::SimpleVertex );
	UINT offset = 0;
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetInputLayout( vertexLayout );

	
	deviceContext->IASetVertexBuffers( 0, 1, &vertexBuffer, &stride, &offset );

	//draw
	//VS
	deviceContext->VSSetShader( vertexShader, nullptr, 0 );


	//HS
	deviceContext->HSSetShader( nullptr, nullptr, 0 );

	//DS
	deviceContext->DSSetShader( nullptr, nullptr, 0 );

	//GS
	deviceContext->GSSetShader( nullptr, nullptr, 0 );

	//PS
	deviceContext->PSSetShader( pixelShader, nullptr, 0);
	deviceContext->PSSetConstantBuffers(2,1,&meshBuffer);
	deviceContext->PSSetSamplers(0, 1, &samplerState);
	deviceContext->PSSetShaderResources(0,1,&textureSRVs[0]);

	deviceContext->Draw( vertCount[0],0);
}
