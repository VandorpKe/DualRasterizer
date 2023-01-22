#include "pch.h"
#include "FullShaderEffect.h"
#include "Effect.h"
#include "Texture.h"

namespace dae
{
	FullShaderEffect::FullShaderEffect(ID3D11Device* pDevice, const std::wstring& assetFile)
		:Effect(pDevice, assetFile)
	{
		m_pNormalMap = m_pEffect->GetVariableByName("gNormalMap")->AsShaderResource();
		if (!m_pNormalMap->IsValid())
			std::wcout << L"m_pNormalMapVariable not valid!\n";

		m_pSpecularMapVariable = m_pEffect->GetVariableByName("gSpecularMap")->AsShaderResource();
		if (!m_pSpecularMapVariable->IsValid())
			std::wcout << L"m_pSpecularMapVariable not valid!\n";

		m_pGlossinessMapVariable = m_pEffect->GetVariableByName("gGlossinessMap")->AsShaderResource();
		if (!m_pGlossinessMapVariable->IsValid())
			std::wcout << L"m_pGlossinessMapVariable not valid!\n";
	}

	FullShaderEffect::~FullShaderEffect()
	{
	}

	void FullShaderEffect::SetNormalMap(Texture* pNormalTexture)
	{
		if (m_pNormalMap)
			m_pNormalMap->SetResource(pNormalTexture->GetSRV());
	}

	void FullShaderEffect::SetSpecularMap(Texture* pSpecularTexture)
	{
		if (m_pSpecularMapVariable)
			m_pSpecularMapVariable->SetResource(pSpecularTexture->GetSRV());
	}

	void FullShaderEffect::SetGlossinessMap(Texture* pGlossinessTexture)
	{
		if (m_pGlossinessMapVariable)
			m_pGlossinessMapVariable->SetResource(pGlossinessTexture->GetSRV());
	}

}
