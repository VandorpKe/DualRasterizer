#include "pch.h"
#include "Renderer.h"
#include "MeshRepresentation.h"
#include "Utils.h"
#include "FullShaderEffect.h"
#include "Texture.h"

// TEXT COLORS
#define RESET   "\033[0m" 
#define GREEN   "\033[32m"     
#define YELLOW  "\033[33m"       
#define MAGENTA "\033[35m" 

namespace dae {

	Renderer::Renderer(SDL_Window* pWindow) :
		m_pWindow(pWindow)
	{
		// UNI INITIALIZE
		SDL_GetWindowSize(pWindow, &m_Width, &m_Height);
		// CAMERA
		m_Camera.Initialize(45.f, { 0.f, 0.f, 0.f }, (float)m_Width / m_Height);

		// -----------------------------------
		// X INITIALIZE DIRECTX PIPELINE
		// -----------------------------------
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

		// VEHICLE
		Texture pDiffuseTexture = { "Resources/vehicle_diffuse.png", m_pDevice };
		Texture pGlossTexture = { "Resources/vehicle_gloss.png", m_pDevice };
		Texture pNormalTexture = { "Resources/vehicle_normal.png", m_pDevice };
		Texture pSpecularTexture = { "Resources/vehicle_specular.png", m_pDevice };

		std::vector<Vertex> verticesVehicle;
		std::vector<uint32_t> indicesVehicle;
		if (!Utils::ParseOBJ("Resources/vehicle.obj", verticesVehicle, indicesVehicle))
			std::wcout << L"Invalid filepath\n";

		FullShaderEffect* pFullShaderEffect{ new FullShaderEffect(m_pDevice, L"Resources/PosCol3D.fx") };
		pFullShaderEffect->SetNormalMap(&pNormalTexture);
		pFullShaderEffect->SetGlossinessMap(&pGlossTexture);
		pFullShaderEffect->SetSpecularMap(&pSpecularTexture);
		pFullShaderEffect->SetDiffuseMap(&pDiffuseTexture);

		auto* pMeshVehicle = new MeshRepresentation{ m_pDevice, verticesVehicle, indicesVehicle, pFullShaderEffect };
		m_pHardwareMeshes.push_back(pMeshVehicle);

		// FIRE
		Effect* pFireEffect{ new Effect(m_pDevice, L"Resources/Transparent3D.fx") };
		Texture* pFireTexture = new Texture{ "Resources/fireFX_diffuse.png", m_pDevice };
		pFireEffect->SetDiffuseMap(pFireTexture);

		std::vector<Vertex> verticesFire;
		std::vector<uint32_t> indicesFire;
		if (!Utils::ParseOBJ("Resources/fireFX.obj", verticesFire, indicesFire))
			std::wcout << L"Invalid filepath\n";

		auto* pMeshFire = new MeshRepresentation{ m_pDevice, verticesFire, indicesFire, pFireEffect };
		m_pHardwareMeshes.push_back(pMeshFire);

		// -----------------------------------
		// X INITIALIZE SOFTWARE RASTERIZER PIPELINE
		// -----------------------------------

		//Create Buffers
		m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
		m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
		m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

		m_pDepthBufferPixels = new float[m_Width * m_Height];

		m_pDiffuseTexture = Texture::LoadFromFile("Resources/vehicle_diffuse.png");		// Texture diffuse of the vehicle
		m_pNormalTexture = Texture::LoadFromFile("Resources/vehicle_normal.png");			// Normal map of the vehicle
		m_pGlossTexture = Texture::LoadFromFile("Resources/vehicle_gloss.png");			// Gloss map of the vehicle
		m_pSpecularTexture = Texture::LoadFromFile("Resources/vehicle_specular.png");		// Specular map of the vehicle

		Mesh& vehicleMesh = m_SoftwareMeshes.emplace_back(Mesh{});
		vehicleMesh.primitiveTopology = PrimitiveTopology::TriangleList;
		Utils::ParseOBJ("Resources/vehicle.obj", vehicleMesh.vertices, vehicleMesh.indices);
	}

	Renderer::~Renderer()
	{
		// DirectX
		for(auto& mesh : m_pHardwareMeshes)
		{
			delete mesh;
		}
		m_pHardwareMeshes.clear();

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

		// Software Rasterizer
		delete[] m_pDepthBufferPixels;
		delete m_pDiffuseTexture;
		delete m_pNormalTexture;
		delete m_pGlossTexture;
		delete m_pSpecularTexture;
	}

	void Renderer::Update(const Timer* pTimer)
	{
		m_Camera.Update(pTimer);

		const float rotationSpeed = { float(M_PI) / 4.f * pTimer->GetElapsed() };
		if(m_EnableRotation)
			m_CurrentAngle += rotationSpeed;

		if (m_DirectXEnabled)
			UpdateHardwareRasterizer(pTimer);
		else
			UpdateSoftwareRasterizer(pTimer);
	}



	void Renderer::Render()
	{
		if (m_DirectXEnabled)
			RenderHardwareRasterizer();
		else
			RenderSoftwareRasterizer();
	}

	// RENDER
	void Renderer::RenderHardwareRasterizer() const
	{
		if (!m_IsInitialized)
			return;

		//1. CLEAR RTV & DSV
		ColorRGB clearColor = ColorRGB{ 0.f, 0.f, 0.3f };
		m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, &clearColor.r);
		m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

		//2. SET PIPELINE + INVOKE DRAWCALLS (= RENDER)
		for (auto& mesh : m_pHardwareMeshes)
		{
			mesh->Render(m_pDeviceContext);
		}

		//3. PRESENT BACKBUFFER (SWAP)
		m_pSwapChain->Present(0, 0);
	}
	void Renderer::RenderSoftwareRasterizer()
	{
		//@START
		//Lock BackBuffer
		SDL_LockSurface(m_pBackBuffer);

		//Clear the BackBuffer
		ColorRGB clearColor{ 100, 100, 100 };
		uint32_t hexColor = 0xFF000000 | (uint32_t)clearColor.b << 8 | (uint32_t)clearColor.g << 16 | (uint32_t)clearColor.r;
		SDL_FillRect(m_pBackBuffer, NULL, hexColor);

		//std::fill_n EXPLAINED
		//1st parameter: beginning of the range of elements to modify
		//2nd parameter: number of elements to be modified
		//3rd parameter: the value to be assigned
		std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);

		VertexTransformationFunctionW3(m_SoftwareMeshes);

		//Iterates over every mesh
		for (auto& mesh : m_SoftwareMeshes)
		{
			int incr = { 3 };
			if (mesh.primitiveTopology == PrimitiveTopology::TriangleStrip)
				incr = 1;

			//Supports multiple triangles
			//indices.size() - 2 => Otherwise index will go out of bounds in 'idxB' and 'idxC' 
			for (int idx = 0; idx < mesh.indices.size() - 2; idx += incr)
			{
				auto idxA = mesh.indices[idx + 0];
				auto idxB = mesh.indices[idx + 1];
				auto idxC = mesh.indices[idx + 2];

				//Skip degenerate triangles
				if (idxA == idxB || idxB == idxC || idxC == idxA)
					continue;

				//Check if triangle is odd
				//When odd, swap vertices B and C
				if (idx % 2 != 0 && mesh.primitiveTopology == PrimitiveTopology::TriangleStrip)
					std::swap(idxB, idxC);

				//Frustum Culling
				//Vertex 0 - Check if x & y are inside [-1, 1] and z inside [0, 1] => DirectX convention
				if ((mesh.vertices_out[idxA].position.x < -1.f || mesh.vertices_out[idxA].position.x > 1.f) &&
					(mesh.vertices_out[idxB].position.x < -1.f || mesh.vertices_out[idxB].position.x > 1.f) &&
					(mesh.vertices_out[idxC].position.x < -1.f || mesh.vertices_out[idxC].position.x > 1.f))
					continue;
				if ((mesh.vertices_out[idxA].position.y < -1.f || mesh.vertices_out[idxA].position.y > 1.f) &&
					(mesh.vertices_out[idxB].position.y < -1.f || mesh.vertices_out[idxB].position.y > 1.f) &&
					(mesh.vertices_out[idxC].position.y < -1.f || mesh.vertices_out[idxC].position.y > 1.f))
					continue;
				if (mesh.vertices_out[idxA].position.z < 0.f || mesh.vertices_out[idxA].position.z > 1.f ||
					mesh.vertices_out[idxB].position.z < 0.f || mesh.vertices_out[idxB].position.z > 1.f ||
					mesh.vertices_out[idxC].position.z < 0.f || mesh.vertices_out[idxC].position.z > 1.f)
					continue;

				// Move from NDC to Raster Space - not a part of the projection stage
				mesh.vertices_out[idxA].position.x = ((mesh.vertices_out[idxA].position.x + 1) / 2.f) * float(m_Width);
				mesh.vertices_out[idxA].position.y = ((1 - mesh.vertices_out[idxA].position.y) / 2.f) * float(m_Height);
				mesh.vertices_out[idxB].position.x = ((mesh.vertices_out[idxB].position.x + 1) / 2.f) * float(m_Width);
				mesh.vertices_out[idxB].position.y = ((1 - mesh.vertices_out[idxB].position.y) / 2.f) * float(m_Height);
				mesh.vertices_out[idxC].position.x = ((mesh.vertices_out[idxC].position.x + 1) / 2.f) * float(m_Width);
				mesh.vertices_out[idxC].position.y = ((1 - mesh.vertices_out[idxC].position.y) / 2.f) * float(m_Height);

				//Get the bounding box TOP LEFT point
				Vector2 boundingBoxMin{};
				boundingBoxMin.x = std::min(mesh.vertices_out[idxA].position.x, std::min(mesh.vertices_out[idxB].position.x, mesh.vertices_out[idxC].position.x));
				boundingBoxMin.y = std::min(mesh.vertices_out[idxA].position.y, std::min(mesh.vertices_out[idxB].position.y, mesh.vertices_out[idxC].position.y));
				//Clamp is needed otherwise the image will repeat itself
				boundingBoxMin.x = Clamp(boundingBoxMin.x, 0.f, float(m_Width));
				boundingBoxMin.y = Clamp(boundingBoxMin.y, 0.f, float(m_Height));
				//Get the bounding box LOWER RIGHT point
				Vector2 boundingBoxMax{};
				boundingBoxMax.x = std::max(mesh.vertices_out[idxA].position.x, std::max(mesh.vertices_out[idxB].position.x, mesh.vertices_out[idxC].position.x));
				boundingBoxMax.y = std::max(mesh.vertices_out[idxA].position.y, std::max(mesh.vertices_out[idxB].position.y, mesh.vertices_out[idxC].position.y));
				//Clamp is needed otherwise the image will repeat itself
				boundingBoxMax.x = Clamp(boundingBoxMax.x, 0.f, float(m_Width));
				boundingBoxMax.y = Clamp(boundingBoxMax.y, 0.f, float(m_Height));

				//RENDER LOGIC
				for (int px = boundingBoxMin.x; px < boundingBoxMax.x; ++px)
				{
					for (int py = boundingBoxMin.y; py < boundingBoxMax.y; ++py)
					{
						//Make a point of the center of the current pixel
						const Vector2 point = { float(px) + 0.5f, float(py) + 0.5f };

						//Make vectors from the point on the triangle to the point that you want to check
						const Vector2 AP = point - mesh.vertices_out[idxA].position.GetXY();
						const Vector2 BP = point - mesh.vertices_out[idxB].position.GetXY();
						const Vector2 CP = point - mesh.vertices_out[idxC].position.GetXY();

						//Make vectors [AB], [BC] & [CB]
						const Vector2 AB = mesh.vertices_out[idxA].position.GetXY() - mesh.vertices_out[idxB].position.GetXY();
						const Vector2 BC = mesh.vertices_out[idxB].position.GetXY() - mesh.vertices_out[idxC].position.GetXY();
						const Vector2 CA = mesh.vertices_out[idxC].position.GetXY() - mesh.vertices_out[idxA].position.GetXY();

						//Calculate the cross product from each pointOfTriangle to the point
						const float signedParallelogramAB = Vector2::Cross(AP, AB);
						const float signedParallelogramBC = Vector2::Cross(BP, BC);
						const float signedParallelogramCA = Vector2::Cross(CP, CA);

						//Get the total area of the triangle - '-CA' to have [AB] X [AC]
						const float totalAreaTriangle = Vector2::Cross(AB, -CA);

						//If the cross products each have the same sign, then 'point' is in the triangle
						if (signedParallelogramAB > 0 && signedParallelogramBC > 0 && signedParallelogramCA > 0)
						{
							//Value gives back how much of the triangle's area covers the total triangle
							const float weightA = signedParallelogramBC / totalAreaTriangle;
							const float weightB = signedParallelogramCA / totalAreaTriangle;
							const float weightC = signedParallelogramAB / totalAreaTriangle;
							//Total weight should be 1 - otherwise somethings wrong

							//DEPTH TEST
							//ZbufferValue - non-linear
							const float interpolatedDepthZ = { 1 / (
								(1 / mesh.vertices_out[idxA].position.z * weightA) +
								(1 / mesh.vertices_out[idxB].position.z * weightB) +
								(1 / mesh.vertices_out[idxC].position.z * weightC)) };

							if (interpolatedDepthZ > m_pDepthBufferPixels[px + (py * m_Width)])
								continue;

							m_pDepthBufferPixels[px + (py * m_Width)] = interpolatedDepthZ;

							//RASTERIZATION STAGE
							//Transform all necessary attributes accordingly, interpolate and store them in the vertex output

							//WbufferValue - linear
							//When When we want to interpolate vertex attributes with a correct depth (color, uv, normals,
							//etc), we still use the View Space depth Vw
							const float interpolatedDepthW = { 1 / (
								(1 / mesh.vertices_out[idxA].position.w * weightA) +
								(1 / mesh.vertices_out[idxB].position.w * weightB) +
								(1 / mesh.vertices_out[idxC].position.w * weightC)) };

							//Divide each attribute by the original vertex depth and interpolate 
							const Vector2 interpolatedUV = { (
								mesh.vertices_out[idxA].uv / mesh.vertices_out[idxA].position.w * weightA +
								mesh.vertices_out[idxB].uv / mesh.vertices_out[idxB].position.w * weightB +
								mesh.vertices_out[idxC].uv / mesh.vertices_out[idxC].position.w * weightC) * interpolatedDepthW };
							const ColorRGB interpolatedColor = { (
								mesh.vertices_out[idxA].color / mesh.vertices_out[idxA].position.w * weightA +
								mesh.vertices_out[idxB].color / mesh.vertices_out[idxB].position.w * weightB +
								mesh.vertices_out[idxC].color / mesh.vertices_out[idxC].position.w * weightC) * interpolatedDepthW };
							const Vector3 interpolatedNormal = { (
								mesh.vertices_out[idxA].normal / mesh.vertices_out[idxA].position.w * weightA +
								mesh.vertices_out[idxB].normal / mesh.vertices_out[idxB].position.w * weightB +
								mesh.vertices_out[idxC].normal / mesh.vertices_out[idxC].position.w * weightC) * interpolatedDepthW };
							const Vector3 interpolatedTangent = { (
								mesh.vertices_out[idxA].tangent / mesh.vertices_out[idxA].position.w * weightA +
								mesh.vertices_out[idxB].tangent / mesh.vertices_out[idxB].position.w * weightB +
								mesh.vertices_out[idxC].tangent / mesh.vertices_out[idxC].position.w * weightC) * interpolatedDepthW };
							const Vector3 interpolatedViewDir = { (
								mesh.vertices_out[idxA].viewDirection / mesh.vertices_out[idxA].position.w * weightA +
								mesh.vertices_out[idxB].viewDirection / mesh.vertices_out[idxB].position.w * weightB +
								mesh.vertices_out[idxC].viewDirection / mesh.vertices_out[idxC].position.w * weightC) * interpolatedDepthW };

							Vertex_Out vertOut{};
							vertOut.position.x = px;
							vertOut.position.y = py;
							vertOut.position.z = interpolatedDepthZ;
							vertOut.position.w = interpolatedDepthW;
							vertOut.uv = interpolatedUV;
							vertOut.color = interpolatedColor;
							vertOut.normal = interpolatedNormal.Normalized();
							vertOut.tangent = interpolatedTangent.Normalized();
							vertOut.viewDirection = interpolatedViewDir.Normalized();

							// Shade your model with Lambert Diffuse
							ColorRGB finalColor{};
							finalColor = PixelShading(vertOut);

							//Update Color in Buffer
							finalColor.MaxToOne();

							m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
								static_cast<uint8_t>(finalColor.r * 255),
								static_cast<uint8_t>(finalColor.g * 255),
								static_cast<uint8_t>(finalColor.b * 255));
						}
					}
				}
			}
		}


		//@END
		//Update SDL Surface
		SDL_UnlockSurface(m_pBackBuffer);
		SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
		SDL_UpdateWindowSurface(m_pWindow);

	}

	// UPDATE
	void Renderer::UpdateHardwareRasterizer(const Timer* pTimer)
	{
		for (auto& mesh : m_pHardwareMeshes)
		{
			mesh->RotateY(m_CurrentAngle);
			mesh->Translation(0, 0, 50);
			mesh->Update(m_Camera.GetWorldViewProjection(), m_Camera.GetInverseViewMatrix());
		}
	}
	void Renderer::UpdateSoftwareRasterizer(const Timer* pTimer)
	{
		for (auto& mesh : m_SoftwareMeshes)
		{
			mesh.worldMatrix = Matrix::CreateRotationY(m_CurrentAngle) * Matrix::CreateTranslation(0.f, 0.f, 50.f);
		}
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
	ColorRGB Renderer::PixelShading(const Vertex_Out& v) const
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
	void Renderer::VertexTransformationFunctionW3(std::vector<Mesh>& meshes) const
	{
		for (auto& mesh : meshes)
		{
			Matrix worldViewProjectionMatrix = { mesh.worldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix };

			// If you want to change the vertices to for example rotate the mesh,
			// you first have to get out the old values.
			mesh.vertices_out.clear();
			mesh.vertices_out.reserve(mesh.vertices.size());

			for (const Vertex& vertex : mesh.vertices)
			{
				//Multiply every vertex with this matrix, which is the same for all vertices within one mesh!
				Vector4 viewSpaceVertex = worldViewProjectionMatrix.TransformPoint({ vertex.position, 1 });

				// Conversion of the normal and tangent from viewspace to world space
				// This is for the rotation
				const Vector3 normal = { mesh.worldMatrix.TransformVector(vertex.normal).Normalized() };
				const Vector3 tangent = { mesh.worldMatrix.TransformVector(vertex.tangent).Normalized() };

				//Conversion to NDC - Perspective Divide (perspective distortion)
				viewSpaceVertex.x /= viewSpaceVertex.w;
				viewSpaceVertex.y /= viewSpaceVertex.w;
				viewSpaceVertex.z /= viewSpaceVertex.w;
				viewSpaceVertex.w = viewSpaceVertex.w;

				// Calculate the vertex world position to World Space
				const Vector3 vertexWorldPos = { mesh.worldMatrix.TransformPoint(vertex.position) };
				// Calculate the viewDirection
				const Vector3 viewDirection = { m_Camera.origin - vertexWorldPos };

				//Convert to Screen Space
				Vertex_Out NdcSpaceVertex{};
				NdcSpaceVertex.position = viewSpaceVertex;			// Copy the position
				NdcSpaceVertex.uv = vertex.uv;						// Copy the UV
				NdcSpaceVertex.normal = normal;						// Put in the transformed normal
				NdcSpaceVertex.tangent = tangent;					// Put in the transformed tangent
				NdcSpaceVertex.viewDirection = viewDirection;		// Put in the transformed viewDirection

				mesh.vertices_out.emplace_back(NdcSpaceVertex);
			}
		}
	}

	void Renderer::StateRasterizer()
	{
		std::cout << YELLOW << "**(SHARED) Rasterizer Mode = ";
		if (m_DirectXEnabled)
		{
			std::cout << "SOFTWARE\n";
			m_DirectXEnabled = false;
		}
		else
		{
			std::cout << "HARDWARE\n";
			m_DirectXEnabled = true;
		}
		std::cout << RESET;
	}
	void Renderer::StateRotation()
	{
		std::cout << YELLOW << "**(SHARED) Vehicle Rotation ";
		if (m_EnableRotation)
		{
			std::cout << "OFF\n";
			m_EnableRotation = false;
		}
		else
		{
			std::cout << "ON\n";
			m_EnableRotation = true;
		}
		std::wcout << RESET;
	}

	void Renderer::StateTechnique()
	{
		for (const auto& mesh : m_pHardwareMeshes)
		{
			mesh->CycleTechnique();
		}
	}

}
