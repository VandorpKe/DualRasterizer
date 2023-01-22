#pragma once

namespace dae
{
	class Texture;
	class Effect
	{
	public:
		Effect(ID3D11Device* pDevice, const std::wstring& assetFile);
		virtual ~Effect();

		Effect(const Effect&) = delete;
		Effect(Effect&&) noexcept = delete;
		Effect& operator=(const Effect&) = delete;
		Effect& operator=(Effect&&) noexcept = delete;

		ID3DX11Effect* GetEffect();
		ID3DX11EffectTechnique* GetTechnique();

		void SetWorldViewProjMatrix(const Matrix& matrix);
		void SetWorldMatrix(const Matrix& matrix);
		void SetViewInvertMatrix(const Matrix& matrix);
		void SetDiffuseMap(Texture* pDiffuseTexture);

		// Didn't understand the cycle ways on the ppt
		void CycleTechnique();
	protected:
		enum class FilterState
		{
			point,
			linear,
			anisotropic,
		};
		FilterState m_FilterMethod = FilterState::point;

		static ID3DX11Effect* LoadEffect(ID3D11Device* pDevice, const std::wstring& assetFile);

		ID3DX11Effect* m_pEffect;
		ID3DX11EffectTechnique* m_pTechnique;

		// MATRICES
		ID3DX11EffectMatrixVariable* m_pMatWorldViewProjVariable;
		ID3DX11EffectMatrixVariable* m_pViewInverseVariable{};
		ID3DX11EffectMatrixVariable* m_pWorldVariable{};

		// DIFFUSE
		ID3DX11EffectShaderResourceVariable* m_pDiffuseMapVariable;
	};
}

