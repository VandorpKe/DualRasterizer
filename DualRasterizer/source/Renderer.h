#pragma once
#include "Camera.h"
#include "Texture.h"

struct SDL_Window;
struct SDL_Surface;
struct Vertex_Out;
struct Mesh;
class MeshRepresentation;

namespace dae
{
	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(const Timer* pTimer);
		void Render() const;

		void RenderHardwareRasterizer() const;
		void RenderSoftwareRasterizer() const;

		ColorRGB PixelShading(const Vertex_Out& v);

		// KEYS
		void StateRasterizer(); // F1
		void StateTechnique();
		void StateRotation();
	private:
		SDL_Window* m_pWindow{};

		int m_Width{};
		int m_Height{};

		bool m_IsInitialized{ false };

		Camera m_Camera{};

		// SELECTION STATE
		bool m_EnableNormalMap = { true };
		bool m_EnableRotation = { true };
		bool m_DirectXEnabled = { true };
		enum class LightingMode
		{
			ObservedArea,	//Lambert Cosine Law
			Diffuse,		//Lambert material
			Specular,		//Glossines
			Combined		//ObservedArea * Diffuse * Specular
		};
		LightingMode m_CurrentLightingMode = LightingMode::Combined;

		// -----------------------------------
		// X DIRECTX
		// -----------------------------------

		HRESULT InitializeDirectX();
		ID3D11Device* m_pDevice;
		ID3D11DeviceContext* m_pDeviceContext;
		IDXGISwapChain* m_pSwapChain;
		ID3D11Texture2D* m_pDepthStencilBuffer;
		ID3D11DepthStencilView* m_pDepthStencilView;
		ID3D11Resource* m_pRenderTargetBuffer;
		ID3D11RenderTargetView* m_pRenderTargetView;

		// MESH
		std::vector<MeshRepresentation*> m_pHardwareMeshes;
		float m_CurrentAngle = { 0.f };

		Texture* m_pDiffuseTexture;
		Texture* m_pGlossTexture;
		Texture* m_pNormalTexture;
		Texture* m_pSpecularTexture;
		Texture* m_pFireTexture;

		// -----------------------------------
		// X SOFTWARE RASTERIZER
		// -----------------------------------

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		float* m_pDepthBufferPixels{};

		std::vector<Mesh> m_SoftwareMeshes;
	};
}
