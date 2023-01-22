#pragma once
#include <SDL_surface.h>
#include <string>
#include "ColorRGB.h"

namespace dae
{
	struct Vector2;

	class Texture
	{
	public:
		// HARDWARE RASTERIZER
		Texture(const std::string& path, ID3D11Device* pDevice);

		// SOFTWARE RASTERIZER
		Texture(SDL_Surface* pSurface);

		~Texture();

		// HARDWARE RASTERIZER
		ID3D11ShaderResourceView* GetSRV() const;

		// SOFTWARE RASTERIZER
		ColorRGB Sample(const Vector2& uv) const;
		static Texture* LoadFromFile(const std::string& path);
	private:
		ID3D11Texture2D* m_pResource{};
		ID3D11ShaderResourceView* m_pSRV{};

		// From Software Rasterizer
		SDL_Surface* m_pSurface{ nullptr };
		uint32_t* m_pSurfacePixels{ nullptr };
	};
}
