#include "pch.h"
#include "Renderer.h"
#include "MeshRepresentation.h"
#include "Utils.h"
#include "FullShaderEffect.h"
#include "Texture.h"

namespace dae {

	Renderer::Renderer(SDL_Window* pWindow) :
		m_pWindow(pWindow)
	{
		//Initialize
		SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

		//Initialize DirectX pipeline
		const HRESULT result = InitializeDirectX();
		if (result == S_OK)
		{
			m_IsInitialized = true;
			std::cout << "DirectX is initialized and ready!\n";
		}
		else
		{
			std::cout << "DirectX initialization failed!\n";
		}

		// CAMERA
		m_Camera.Initialize(45.f, { 0.f, 0.f, 0.f }, (float)m_Width / m_Height);

		// VEHICLE
		m_pDiffuseTexture = new Texture{ "Resources/vehicle_diffuse.png", m_pDevice };
		m_pGlossTexture = new Texture{ "Resources/vehicle_gloss.png", m_pDevice };
		m_pNormalTexture = new Texture{ "Resources/vehicle_normal.png", m_pDevice };
		m_pSpecularTexture = new Texture{ "Resources/vehicle_specular.png", m_pDevice };

		std::vector<Vertex> verticesVehicle;
		std::vector<uint32_t> indicesVehicle;
		if (!Utils::ParseOBJ("Resources/vehicle.obj", verticesVehicle, indicesVehicle))
			std::wcout << L"Invalid filepath\n";

		FullShaderEffect* pFullShaderEffect{ new FullShaderEffect(m_pDevice, L"Resources/PosCol3D.fx") };
		pFullShaderEffect->SetNormalMap(m_pNormalTexture);
		pFullShaderEffect->SetGlossinessMap(m_pGlossTexture);
		pFullShaderEffect->SetSpecularMap(m_pSpecularTexture);
		pFullShaderEffect->SetDiffuseMap(m_pDiffuseTexture);

		auto* pMeshVehicle = new MeshRepresentation{ m_pDevice, verticesVehicle, indicesVehicle, pFullShaderEffect };
		m_pMeshes.push_back(pMeshVehicle);

		// FIRE
		Effect* pFireEffect{ new Effect(m_pDevice, L"Resources/Transparent3D.fx") };
		m_pFireTexture = new Texture{ "Resources/fireFX_diffuse.png", m_pDevice };
		pFireEffect->SetDiffuseMap(m_pFireTexture);

		std::vector<Vertex> verticesFire;
		std::vector<uint32_t> indicesFire;
		if (!Utils::ParseOBJ("Resources/fireFX.obj", verticesFire, indicesFire))
			std::wcout << L"Invalid filepath\n";

		auto* pMeshFire = new MeshRepresentation{ m_pDevice, verticesFire, indicesFire, pFireEffect };
		m_pMeshes.push_back(pMeshFire);
	}

	Renderer::~Renderer()
	{
		for(auto& mesh : m_pMeshes)
		{
			delete mesh;
		}
		m_pMeshes.clear();

		if (m_pRenderTargetView) m_pRenderTargetView->Release();
		if (m_pRenderTargetBuffer) m_pRenderTargetBuffer->Release();
		if (m_pDepthStencilView) m_pDepthStencilView->Release();
		if (m_pDepthStencilBuffer) m_pDepthStencilBuffer->Release();
		if (m_pSwapChain) m_pSwapChain->Release();
		if (m_pDeviceContext)
		{
			m_pDeviceContext->ClearState();
			m_pDeviceContext->Flush();
			m_pDeviceContext->Release();
		}
		if (m_pDevice) m_pDevice->Release();
	}

	void Renderer::Update(const Timer* pTimer)
	{
		m_Camera.Update(pTimer);

		const float rotationSpeed = { float(M_PI) / 4.f * pTimer->GetElapsed() };
		if(m_EnableRotation)
		{
			m_CurrentAngle += rotationSpeed;
			std::cout << "Updating rotation\n";
		}

		for(auto& mesh : m_pMeshes)
		{
			mesh->RotateY(m_CurrentAngle);
			mesh->Translation(0, 0, 50);
			mesh->Update(m_Camera.GetWorldViewProjection(), m_Camera.GetInverseViewMatrix());
		}
	}



	void Renderer::Render() const
	{
		if (!m_IsInitialized)
			return;

		//1. CLEAR RTV & DSV
		ColorRGB clearColor = ColorRGB{0.f, 0.f, 0.3f}; 
		m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView,&clearColor.r);
		m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

		//2. SET PIPELINE + INVOKE DRAWCALLS (= RENDER)
		for (auto& mesh : m_pMeshes)
		{
			mesh->Render(m_pDeviceContext);
		}
		
		//3. PRESENT BACKBUFFER (SWAP)
		m_pSwapChain->Present(0, 0);
	}

	HRESULT Renderer::InitializeDirectX()
	{
		//1. Create Device & DeviceContext
		//=====
		D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;
		uint32_t createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif 
		HRESULT result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, createDeviceFlags, &featureLevel,
			1, D3D11_SDK_VERSION, &m_pDevice, nullptr, &m_pDeviceContext);
		if (FAILED(result))
			return result;

		//Create DXGI Factory
		IDXGIFactory1* pDxgiFactory{};
		result = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&pDxgiFactory));
		if (FAILED(result))
			return result;

		//2. Create Swapchain
		//=====
		DXGI_SWAP_CHAIN_DESC swapChainDesc{};
		swapChainDesc.BufferDesc.Width = m_Width;
		swapChainDesc.BufferDesc.Height = m_Height;
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 1; 
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 60;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED; 
		swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 1;
		swapChainDesc.Windowed = true;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.Flags = 0;

		//Get the handle (HWND) from the SDL Backbuffer
		SDL_SysWMinfo sysWMInfo{}; 
		SDL_VERSION(&sysWMInfo.version)
		SDL_GetWindowWMInfo(m_pWindow, &sysWMInfo);
		swapChainDesc.OutputWindow = sysWMInfo.info.win.window;
		//Create SwapChain
		result = pDxgiFactory->CreateSwapChain(m_pDevice, &swapChainDesc, &m_pSwapChain);
		if (FAILED(result))
			return result;

		//3. Create DepthStencil (DS) & DepthStencilView (DSV)
		//Resource
		D3D11_TEXTURE2D_DESC depthStencilDesc{};
		depthStencilDesc.Width = m_Width;
		depthStencilDesc.Height = m_Height;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.ArraySize = 1;
		depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthStencilDesc.CPUAccessFlags = 0;
		depthStencilDesc.MiscFlags = 0;

		//View
		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{}; 
		depthStencilViewDesc.Format = depthStencilDesc.Format;
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Texture2D.MipSlice = 0;

		result = m_pDevice->CreateTexture2D(&depthStencilDesc, nullptr, &m_pDepthStencilBuffer); 
		if (FAILED(result)) 
			return result;
		result = m_pDevice->CreateDepthStencilView(m_pDepthStencilBuffer, &depthStencilViewDesc, &m_pDepthStencilView);
		if (FAILED(result))
			return result;

		//4. Create RenderTarget (RT) & RenderTargetView (RTV)
		//===== 
		// 
		//Resource
		result = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&m_pRenderTargetBuffer)); if (FAILED(result))
			return result;

		//View
		result = m_pDevice->CreateRenderTargetView(m_pRenderTargetBuffer, nullptr, &m_pRenderTargetView);
		if (FAILED(result))
			return result;

		//5. Bind RTV & DSV to Output Merger Stage
		//=====
		m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);

		//6. Set Viewport
		//=====
		D3D11_VIEWPORT viewport{};
		viewport.Width = static_cast<float>(m_Width);
		viewport.Height = static_cast<float>(m_Height);
		viewport.TopLeftX = 0.f;
		viewport.TopLeftY = 0.f;
		viewport.MinDepth = 0.f;
		viewport.MaxDepth = 1.f;
		m_pDeviceContext->RSSetViewports(1, &viewport);

		return result;
	}


	// From software rasterizer
	ColorRGB Renderer::PixelShading(const Vertex_Out& v)
	{
		const Vector3 lightDirection = { .577f, -.577f, .577f };
		// Diffuse Reflection Coefficient
		const float lightIntensity = { 7.f };
		const float shininess = { 25.f };
		const ColorRGB ambient = { .025f, .025f, .025f };

		const ColorRGB diffuse{ m_pDiffuseTexture->Sample(v.uv) };
		const ColorRGB lambertDiffuseColor{ (lightIntensity * diffuse) / PI };

		//-------------------------
		// NORMAL MAPS
		// Calculate tangentSpaceAxis
		const Vector3 binormal = { Vector3::Cross(v.normal, v.tangent) };
		const Matrix tangentSpaceAxis = Matrix{ v.tangent, binormal, v.normal, Vector3::Zero };

		// When sampling our normal is in [0, 255], but normalized vectors have [-1, 1]
		ColorRGB sampledNormal = { m_pNormalTexture->Sample(v.uv) };
		// no need to divide by 255 - [0, 255] -> [0, 1] - is already done in the sample function
		const Vector3 sampledNormalRemap = { 2.f * sampledNormal.r - 1.f, 2.f * sampledNormal.g - 1.f, 2.f * sampledNormal.b - 1.f }; // [0, 1] -> [-1, 1]

		// Calculate sampled normal to tanget space
		const Vector3 sampleNrmlTangentSpace{ tangentSpaceAxis.TransformVector(sampledNormalRemap.Normalized()).Normalized() };

		//-------------------------
		// NORMAL MAP ENABLED
		Vector3 selectedNormal{};
		if (m_EnableNormalMap)
			selectedNormal = sampleNrmlTangentSpace;
		else
			selectedNormal = v.normal;

		//-------------------------
		// LAMBERT'S COSINE LAW
		// Calculate observed area (Lambert's cosine law)
		float observedArea = { Vector3::Dot(selectedNormal, -lightDirection) };

		// Return nothing if observed area is negative
		if (observedArea < 0)
			return ColorRGB{ 0, 0, 0 };

		//-------------------------
		// PHONG
		// Calculate the phong
		const Vector3 reflect = { lightDirection - 2.f * Vector3::Dot(selectedNormal, lightDirection) * selectedNormal };
		const float cosAlpha = { std::max(0.f, Vector3::Dot(reflect, v.viewDirection)) };
		// r, g & b are the same so you can use either one
		const ColorRGB specReflectance = { m_pSpecularTexture->Sample(v.uv) * powf(cosAlpha, m_pGlossTexture->Sample(v.uv).r * shininess) };

		//-------------------------
		// RETURN
		ColorRGB finalColor{ observedArea, observedArea, observedArea };
		switch (m_CurrentLightingMode)
		{
		case LightingMode::ObservedArea:
			return finalColor;
		case LightingMode::Diffuse:
			return finalColor *= lambertDiffuseColor;
		case LightingMode::Specular:
			return finalColor = specReflectance;
		case LightingMode::Combined:
			return finalColor *= lambertDiffuseColor + specReflectance + ambient;
		}
	}

	void Renderer::StateTechnique()
	{
		for (const auto& mesh : m_pMeshes)
		{
			mesh->CycleTechnique();
		}
	}

	void Renderer::StateRotation()
	{
		if (m_EnableRotation)
			m_EnableRotation = false;
		else
			m_EnableRotation = true;

		std::wcout << L"Rotation: " << m_EnableRotation << std::endl;
	}
}
