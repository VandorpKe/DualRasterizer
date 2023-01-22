#include "pch.h"
#include "Texture.h"
#include "Vector2.h"
#include <SDL_image.h>

namespace dae
{
	Texture::Texture(const std::string& path, ID3D11Device* pDevice)
	{
		m_pSurface = IMG_Load(path.c_str());

		const DXGI_FORMAT format{ DXGI_FORMAT_R8G8B8A8_UNORM };
		D3D11_TEXTURE2D_DESC desc{};
		desc.Width = m_pSurface->w;
		desc.Height = m_pSurface->h;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = format;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA initData{};
		initData.pSysMem = m_pSurface->pixels;
		initData.SysMemPitch = static_cast<UINT>(m_pSurface->pitch);
		initData.SysMemSlicePitch = static_cast<UINT>(m_pSurface->h * m_pSurface->pitch);

		HRESULT hr = pDevice->CreateTexture2D(&desc, &initData, &m_pResource);

		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
		SRVDesc.Format = format;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = 1;

		hr = pDevice->CreateShaderResourceView(m_pResource, &SRVDesc, &m_pSRV);

		m_pSurfacePixels = (uint32_t*)m_pSurface->pixels;
	}

	Texture::~Texture()
	{
		if (m_pSRV) m_pSRV->Release();
		if (m_pResource) m_pResource->Release();

		SDL_FreeSurface(m_pSurface);
	}

	ID3D11ShaderResourceView* Texture::GetSRV() const
	{
		return m_pSRV;
	}

	ColorRGB Texture::Sample(const Vector2& uv) const
	{
		//Sample the correct pixel for the given uv

		//Convert UV from [0, 1] range to [0, width/height] range
		const size_t x{ size_t(uv.x * m_pSurface->w) };
		const size_t y{ size_t(uv.y * m_pSurface->h) };

		//Get the index from the UV coordinates
		const Uint32 pixel{ m_pSurfacePixels[x + y * m_pSurface->w] };

		Uint8 r, g, b;
		SDL_GetRGB(pixel, m_pSurface->format, &r, &g, &b);

		//Remap the color from [0,255] to [0, 1]
		return ColorRGB{ r / 255.f, g / 255.f, b / 255.f };
	}

}