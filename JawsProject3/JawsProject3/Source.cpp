#define _XM_NO_INTRINSICS_
#include <windows.h>
#include <Windowsx.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>
#include "DDSTextureLoader.h"

using namespace DirectX;

#include "Terrain.h"
#include "LightHelper.h"
#include "Camera.h"
#include "ParticleSystem.h"
#include "CustomMeshes.h"

#define _DEBUG


struct ConstantBuffer
{
	DirectionalLight gDirLight;
	PointLight gPointLight;
	Material gMaterial;

	XMFLOAT4 gFogColor;
	float gFogStart;
	float gFogRange;
	XMFLOAT2 pad;

	//moved frustrums to frame
};

struct CBPerFrame
{
	XMFLOAT3 gEyePosW;
	float padding;

	XMMATRIX View;
	XMMATRIX Projection;

	XMMATRIX gViewProj;
		
	XMFLOAT4 gWorldFrustumPlanes[6];
};






//global variables
HINSTANCE					g_hInst = nullptr;
HWND						g_hWnd = nullptr;

D3D_DRIVER_TYPE			    g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL			g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*				g_pd3dDevice = nullptr;
ID3D11Device1*				g_pd3dDevice1 = nullptr;
ID3D11DeviceContext*		g_pImmediateContext = nullptr;
ID3D11DeviceContext1*		g_pImmediateContext1 = nullptr;
IDXGISwapChain*				g_pSwapChain = nullptr;
ID3D11RenderTargetView*		g_pRenderTargetView = nullptr;
ID3D11Texture2D*			g_pDepthStencil = nullptr;
ID3D11DepthStencilView*		g_pDepthStencilView = nullptr;
ID3D11RasterizerState*		g_pRasterizerState = nullptr;

ID3D11Buffer*				g_pCBPerFrame = nullptr;
ID3D11Buffer*				g_pConstantBuffer = nullptr;
ID3D11ShaderResourceView*	g_pTextureRV = nullptr;
ID3D11SamplerState*			g_pSamplerLinear = nullptr;

Terrain*					g_Terrain;
ParticleSystem				g_ParticleSystem;
Camera*						g_Camera;
GraphicHelper				g_GraphicHelper;
CustomMeshes*				g_CustomMeshes;

Material					g_Material;
PointLight					g_PointLight;
DirectionalLight			g_DirectionalLight;

XMFLOAT2					g_LastMousePos;

//forward declarations
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow );
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
HRESULT InitDevice();
HRESULT InitDriverAndVersion(UINT width, UINT height);
HRESULT CreateRenderTargetView();
HRESULT CreateDepthStencil(UINT width, UINT height);
HRESULT CreateSamplerState();
void SetViewport(UINT width, UINT height);


void Render();
void CleanupDevice();
HRESULT CreateRastizer();


float dt = 0;
float gt = 0;

//entry point

int WINAPI wWinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow )
{
    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( lpCmdLine );

    if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
        return 0;

    if( FAILED( InitDevice() ) )
    {
        CleanupDevice();
        return 0;
    }
	
	__int64 cntsPerSec = 0;
	QueryPerformanceFrequency((LARGE_INTEGER*)&cntsPerSec);
	float secsPerCnt = 1.0f / (float)cntsPerSec;

	__int64 prevTimeStamp = 0;
	QueryPerformanceCounter((LARGE_INTEGER*)&prevTimeStamp);

    // Main message loop
    MSG msg = {0};
    while( WM_QUIT != msg.message )
    {
        // Update our time
		
		
		if( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
			__int64 currTimeStamp = 0;
			QueryPerformanceCounter((LARGE_INTEGER*)&currTimeStamp);
			dt = (currTimeStamp - prevTimeStamp) * secsPerCnt;
			gt += dt;

			Render();
			prevTimeStamp = currTimeStamp;
        }
    }
    CleanupDevice();

    return ( int )msg.wParam;
}

//create the window
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{
   WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon( hInstance, ( LPCTSTR )107 );
    wcex.hCursor = LoadCursor( nullptr, IDC_ARROW );
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"Lab02";
    wcex.hIconSm = LoadIcon( wcex.hInstance, ( LPCTSTR )107 );
    if( !RegisterClassEx( &wcex ) )
        return E_FAIL;

    // Create window
    g_hInst = hInstance;
    RECT rc = { 0, 0, 1920, 1080 };
    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
    g_hWnd = CreateWindow( L"Lab02", L"Best window ever", WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
                           nullptr );
    if( !g_hWnd )
        return E_FAIL;

    ShowWindow( g_hWnd, nCmdShow );


    return S_OK;
}

void UpdateScene(float dt)
{
	if(GetAsyncKeyState('W')&0x8000)
	{
		g_Camera->Walk(20000.0*dt);
	}
	if(GetAsyncKeyState('S')&0x8000)
	{
		g_Camera->Walk(-20000.0*dt);
	}
	if(GetAsyncKeyState('A')&0x8000)
	{
		g_Camera->Strafe(-20000.0*dt);
	}
	if(GetAsyncKeyState('D')&0x8000)
	{
		g_Camera->Strafe(20000.0*dt);
	}
}

void OnMouseMove(WPARAM btnStae, int x, int y)
{
	if (btnStae & MK_LBUTTON != 0)
	{
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - g_LastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - g_LastMousePos.y));

		g_Camera->Pitch(dy);
		g_Camera->RotateY(dx);
	}

	g_LastMousePos.x = x;
	g_LastMousePos.y = y;
}

//callback function for messages

LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch( message )
    {
        case WM_PAINT:
            hdc = BeginPaint( hWnd, &ps );
            EndPaint( hWnd, &ps );
            break;

        case WM_DESTROY:
            PostQuitMessage( 0 );
            break;

		case WM_KEYDOWN:
			UpdateScene(dt);
			switch(wParam)
			{
				case VK_ESCAPE:
					PostQuitMessage(0);
					break;
			}
			break;
		case WM_MOUSEMOVE:
			OnMouseMove(wParam, GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
			break;

        default:
            return DefWindowProc( hWnd, message, wParam, lParam );
    }

    return 0;
}

//create direct3d device & swap chain

HRESULT InitDevice()
{
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect( g_hWnd, &rc );
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;


	//initialize driver and version
	hr = InitDriverAndVersion( width, height);
	if( FAILED( hr ) )
        return hr;

	// Create a render target view
	hr = CreateRenderTargetView();
	if( FAILED( hr ) )
        return hr;

	//load textuer
	// Load the Texture
    hr = CreateDDSTextureFromFile( g_pd3dDevice, L"Smile.dds", nullptr, &g_pTextureRV );
	

    if( FAILED( hr ) )
        return hr;

	//create sampler state
	hr = CreateSamplerState();
	if( FAILED( hr ) )
        return hr;


	//Create depth stencil texture and viwe
	hr = CreateDepthStencil(width,height);
	if( FAILED(hr) )
		return hr;

	g_pImmediateContext->OMSetRenderTargets( 1, &g_pRenderTargetView, g_pDepthStencilView );



    // Setup the viewport
	SetViewport(width,height);




	////intialize terrain
	//g_Terrain = new Terrain();
	//hr = g_Terrain->Initialize(g_pd3dDevice, g_pImmediateContext);
	//if( FAILED( hr ) )
 //       return hr;

	////initialize particle system
	//g_ParticleSystem  = ParticleSystem();
	//hr = g_ParticleSystem.Initialize(g_pd3dDevice, g_pImmediateContext);
	//if( FAILED( hr ) )
 //       return hr;

	g_CustomMeshes = new CustomMeshes();
	g_CustomMeshes->Initialize(g_pd3dDevice,g_pImmediateContext);
	

	//set camera...
	g_Camera = new Camera(g_Terrain);
	g_Camera->LookAt(XMFLOAT3(0,150, 0),XMFLOAT3(0,0,1),XMFLOAT3(0,1,0));
	g_Camera->SetLens(XM_PIDIV4, width / (FLOAT)height, 0.01f, 100000.0f );
	//not sure if should be the transposed or normal
	
	XMMATRIX viewProject = XMMatrixMultiply(g_Camera->View(),g_Camera->Proj());

	g_Camera->ExtractFrustumPlanes(viewProject);
	XMMATRIX mProj = XMMatrixTranspose(g_Camera->Proj()); 

	
	//set material not used atm
	g_Material = Material();
	g_Material.Ambient = XMFLOAT4(1,1,1,1);
	g_Material.Diffuse = XMFLOAT4(1,1,1,1);
	g_Material.Specular = XMFLOAT4(1,1,1,1);
	g_Material.Transmission = XMFLOAT4(1,1,1,1);

	//set the pointlight
	g_PointLight = PointLight();
	g_PointLight.Ambient =	XMFLOAT4(0.1,		0.1,	0.1,		1);
	g_PointLight.Diffuse =	XMFLOAT4(0.6,		0.6,	0.6,		1);
	g_PointLight.Specular =	XMFLOAT4(0.3,		0.3,	0.3,		1);
	g_PointLight.Pad = 0;
	g_PointLight.Position = XMFLOAT3(200, 250, 100);
	g_PointLight.Att = XMFLOAT3(1.0,0,0);
	g_PointLight.Range = 100000;





	//set directional light
	g_DirectionalLight = DirectionalLight();
	g_DirectionalLight.Ambient = XMFLOAT4(0.1, 0.1, 0.1, 1);
	g_DirectionalLight.Diffuse = XMFLOAT4(0.6 ,0.6, 0.6, 1);
	g_DirectionalLight.Specular = XMFLOAT4(0.3,0.3,0.3,1);
	g_DirectionalLight.Direction = XMFLOAT3(1,-1,0);
	g_DirectionalLight.Pad = 0;
	

	// Set primitive topology
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);
	//g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	//g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	//create and set constant buffesr

	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
	hr = g_pd3dDevice->CreateBuffer( &bd, nullptr, &g_pConstantBuffer );
    if( FAILED( hr ) )
        return hr;
    
	ConstantBuffer cb;
	
	
	cb.gMaterial = g_Material;
	cb.gPointLight = g_PointLight;
	cb.gDirLight = g_DirectionalLight;
	cb.gFogStart = 30;
	cb.gFogRange = 150;
	cb.gFogColor = XMFLOAT4(0,0,0,0);
	g_pImmediateContext->UpdateSubresource( g_pConstantBuffer, 0, nullptr, &cb, 0, 0 );


	///create second
	bd.ByteWidth = sizeof(CBPerFrame);
	hr = g_pd3dDevice->CreateBuffer( &bd, nullptr, &g_pCBPerFrame );
	if( FAILED( hr ) )
        return hr;

	CBPerFrame cbpf;
	

	XMMATRIX view = XMMatrixTranspose(g_Camera->View());
	XMMATRIX proj = XMMatrixTranspose(g_Camera->Proj());
	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	
	cbpf.gViewProj =  XMMatrixMultiply(view, proj);
	cbpf.gEyePosW = g_Camera->GetPosition();
	cbpf.padding = 0.0f;
	cbpf.Projection = proj;
	cbpf.View = view;
	
	for (int i = 0; i < 6; i++)
	{
		cbpf.gWorldFrustumPlanes[i] = g_Camera->GetPlane(i);
	}

	g_pImmediateContext->UpdateSubresource(g_pCBPerFrame,0,nullptr,&cbpf,0,0);
	
	hr = CreateRastizer();
	if( FAILED( hr ) )
        return hr;


	/////////////////////TESTING

	


    return S_OK;
}

HRESULT InitDriverAndVersion(UINT width, UINT height)
{
	HRESULT hr = S_OK;
    
	UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE( driverTypes );

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
	UINT numFeatureLevels = ARRAYSIZE( featureLevels );

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof( sd ) );
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain(
			nullptr, 
			g_driverType, 
			nullptr, 
			createDeviceFlags, 
			featureLevels, 
			numFeatureLevels,
			D3D11_SDK_VERSION, 
			&sd, 
			&g_pSwapChain, 
			&g_pd3dDevice, 
			&g_featureLevel, 
			&g_pImmediateContext );

        if ( hr == E_INVALIDARG )
        {
            // DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
            hr = D3D11CreateDeviceAndSwapChain(
				nullptr, 
				g_driverType, 
				nullptr, 
				createDeviceFlags, 
				&featureLevels[1], 
				numFeatureLevels - 1, 
				D3D11_SDK_VERSION, &sd,
				&g_pSwapChain,
				&g_pd3dDevice, 
				&g_featureLevel, 
				&g_pImmediateContext );
        }

        if( SUCCEEDED( hr ) )
            break;
    }
    if( FAILED( hr ) )
        return hr;

    // Obtain the Direct3D 11.1 versions if available
    hr = g_pd3dDevice->QueryInterface( __uuidof( ID3D11Device1 ), reinterpret_cast<void**>( &g_pd3dDevice1 ) );
    if ( SUCCEEDED(hr) )
    {
        (void)g_pImmediateContext->QueryInterface( __uuidof( ID3D11DeviceContext1 ), reinterpret_cast<void**>( &g_pImmediateContext1 ) );
    }
	return hr;
}

HRESULT CreateSamplerState()
{
	HRESULT hr = S_OK;
	D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory( &sampDesc, sizeof(sampDesc) );
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = g_pd3dDevice->CreateSamplerState( &sampDesc, &g_pSamplerLinear );

	return hr;
}

//compile Shaders


HRESULT CreateRastizer()
{
	HRESULT hr = S_OK;
	D3D11_RASTERIZER_DESC desc;
	//desc.FillMode = D3D11_FILL_WIREFRAME;
	desc.FillMode = D3D11_FILL_SOLID;
	desc.CullMode = D3D11_CULL_BACK;
	desc.FrontCounterClockwise = false;
	desc.DepthBias = 0;
	desc.SlopeScaledDepthBias = 0.0f;
	desc.DepthBiasClamp = 0.0f;
	desc.DepthClipEnable = true;
	desc.ScissorEnable = false;
	desc.MultisampleEnable = false;
	desc.AntialiasedLineEnable = false;

	hr = g_pd3dDevice->CreateRasterizerState(&desc,&g_pRasterizerState);

	g_pImmediateContext->RSSetState(g_pRasterizerState);

	return hr;
}



// Create a render target view
HRESULT CreateRenderTargetView()
{
    HRESULT hr = S_OK;
	ID3D11Texture2D* pBackBuffer = nullptr;
    hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&pBackBuffer );
    if( FAILED( hr ) )
        return hr;

    hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, nullptr, &g_pRenderTargetView );
    pBackBuffer->Release();
    if( FAILED( hr ) )
        return hr;

	return hr;
}

//create depth stencil texture and view
HRESULT CreateDepthStencil(UINT width, UINT height)
{
	HRESULT hr = S_OK;

	// Create depth stencil texture
    D3D11_TEXTURE2D_DESC descDepth;
	ZeroMemory( &descDepth, sizeof(descDepth) );
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
	
    hr = g_pd3dDevice->CreateTexture2D( &descDepth, nullptr, &g_pDepthStencil );
	if( FAILED( hr ) )
        return hr;

	
	// Create the depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory( &descDSV, sizeof(descDSV) );
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    hr = g_pd3dDevice->CreateDepthStencilView( g_pDepthStencil, &descDSV, &g_pDepthStencilView );
	
	return hr;
}



// Setup the viewport
void SetViewport(UINT width, UINT height)
{
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports( 1, &vp );
}




//create vertex and indexes //////NOT USED//////



//render
void Render()
{
	//static float t = 0.0f;
	//if( g_driverType == D3D_DRIVER_TYPE_REFERENCE )
	//{
	//	t += ( float )XM_PI * 0.0125f;
	//}
	//else
	//{
	//	static ULONGLONG timeStart = 0;
	//	ULONGLONG timeCur = GetTickCount64();
	//	if( timeStart == 0 )
	//		timeStart = timeCur;
	//	t = ( timeCur - timeStart ) / 1000.0f;
	//}
	
	
	g_Camera->UpdateViewMatrix();

	XMMATRIX viewProject = XMMatrixMultiply(g_Camera->View(),g_Camera->Proj());
	g_Camera->ExtractFrustumPlanes(viewProject);

	// clear the backbuffer
	g_pImmediateContext->ClearRenderTargetView( g_pRenderTargetView, Colors::DarkGray );
	
	
	//clear the depth buffer to max depth (1.0)
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView,D3D11_CLEAR_DEPTH,1.0f,0);
	//update frame buffers


	//ConstantBuffer cb;
	//
	//g_PointLight.Position = XMFLOAT3( sin(t)* 500, 250, cos(t) * 500);
	//cb.gMaterial = g_Material;
	//cb.gPointLight = g_PointLight;
	//cb.gDirLight = g_DirectionalLight;
	//g_pImmediateContext->UpdateSubresource( g_pConstantBuffer, 0, nullptr, &cb, 0, 0 );

	CBPerFrame cbpf;


	XMMATRIX view = XMMatrixTranspose(g_Camera->View());
	XMMATRIX proj = XMMatrixTranspose(g_Camera->Proj());
	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	

	cbpf.gViewProj =  XMMatrixMultiply(view, proj);
	cbpf.gEyePosW = g_Camera->GetPosition();
	cbpf.padding = 0.0f;
	cbpf.Projection = proj;
	cbpf.View = view;


	for (int i = 0; i < 6; i++)
	{
		cbpf.gWorldFrustumPlanes[i] = g_Camera->GetPlane(i);
	}
	
	g_pImmediateContext->UpdateSubresource(g_pCBPerFrame,0,nullptr,&cbpf,0,0);
	
	//update object buffers
	g_pImmediateContext->VSSetConstantBuffers( 0, 1, &g_pConstantBuffer );
	g_pImmediateContext->VSSetConstantBuffers( 1, 1, &g_pCBPerFrame );

	g_pImmediateContext->HSSetConstantBuffers( 0, 1, &g_pConstantBuffer );
	g_pImmediateContext->HSSetConstantBuffers( 1, 1, &g_pCBPerFrame );

	g_pImmediateContext->DSSetConstantBuffers( 0, 1, &g_pConstantBuffer );
	g_pImmediateContext->DSSetConstantBuffers( 1, 1, &g_pCBPerFrame );

	g_pImmediateContext->GSSetConstantBuffers( 0, 1, &g_pConstantBuffer );
	g_pImmediateContext->GSSetConstantBuffers( 1, 1, &g_pCBPerFrame );
	
	g_pImmediateContext->PSSetConstantBuffers( 0, 1, &g_pConstantBuffer );
	g_pImmediateContext->PSSetConstantBuffers( 1, 1, &g_pCBPerFrame );


	/*g_Terrain->Draw(g_pImmediateContext);
	g_ParticleSystem.Draw(g_pImmediateContext,g_pd3dDevice,dt,gt);*/
	g_CustomMeshes->Draw(g_pImmediateContext);

	//swap the buffer
    g_pSwapChain->Present( 0, 0 );
}

//clean up what we've created
void CleanupDevice()
{
    if( g_pImmediateContext ) g_pImmediateContext->ClearState();

	if( g_pSamplerLinear )g_pSamplerLinear->Release();
	if( g_pTextureRV ) g_pTextureRV->Release();
	if( g_pCBPerFrame ) g_pCBPerFrame->Release();
	if( g_pConstantBuffer) g_pConstantBuffer->Release();
	
	if( g_pDepthStencil ) g_pDepthStencil->Release();
    if( g_pDepthStencilView ) g_pDepthStencilView->Release();
    if( g_pRenderTargetView ) g_pRenderTargetView->Release();
	if( g_pRasterizerState ) g_pRasterizerState->Release();
    if( g_pSwapChain ) g_pSwapChain->Release();
    if( g_pImmediateContext1 ) g_pImmediateContext1->Release();
    if( g_pImmediateContext ) g_pImmediateContext->Release();
    if( g_pd3dDevice1 ) g_pd3dDevice1->Release();
    if( g_pd3dDevice ) g_pd3dDevice->Release();


	/*g_Terrain->Cleanup(g_pd3dDevice);
	g_ParticleSystem.Cleanup(g_pd3dDevice);
	delete(g_Camera);
	delete(g_Terrain);*/
}

