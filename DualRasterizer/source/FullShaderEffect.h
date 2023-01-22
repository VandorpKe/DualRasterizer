#pragma once
#include "Effect.h"

namespace dae
{
	class FullShaderEffect final : public Effect
	{
	public:
		FullShaderEffect(ID3D11Device* pDevice, const std::wstring& assetFile);
		virtual ~FullShaderEffect();

		FullShaderEffect(const FullShaderEffect&)					= delete;
		FullShaderEffect(FullShaderEffect&&) noexcept				= delete;
		FullShaderEffect& operator=(const FullShaderEffect&)		= delete;
		FullShaderEffect& operator=(FullShaderEffect&&) noexcept	= delete;

		void SetNormalMap(Texture* pNormalTexture);
		void SetSpecularMap(Texture* pSpecularTexture);
		void SetGlossinessMap(Texture* pGlossinessTexture);
	private:
		// Only needed for the full shading of the vehicle
		// Not the transparent fire
		ID3DX11EffectShaderResourceVariable* m_pNormalMap{};
		ID3DX11EffectShaderResourceVariable* m_pSpecularMapVariable{};
		ID3DX11EffectShaderResourceVariable* m_pGlossinessMapVariable{};
	};
}