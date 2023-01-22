#include "pch.h"
#include "Effect.h"
#include "Texture.h"

namespace dae
{
	Effect::Effect(ID3D11Device* pDevice, const std::wstring& assetFile)
	{
		m_pEffect = LoadEffect(pDevice, assetFile);

		m_pTechnique = m_pEffect->GetTechniqueByName("PointFilteringTechnique");
		if (!m_pTechnique->IsValid())
			std::wcout << L"FilterState not valid\n";

		m_pMatWorldViewProjVariable = m_pEffect->GetVariableByName("gWorldViewProj")->AsMatrix();
		if (!m_pMatWorldViewProjVariable->IsValid())
			std::wcout << L"m_pMatWorldViewProjVariable not valid!\n";

		m_pWorldVariable = m_pEffect->GetVariableByName("gWorldMatrix")->AsMatrix();
		if (!m_pWorldVariable->IsValid())
			std::wcout << L"m_pWorldVariable not valid!\n";

		m_pViewInverseVariable = m_pEffect->GetVariableByName("gViewInverseMatrix")->AsMatrix();
		if (!m_pViewInverseVariable->IsValid())
			std::wcout << L"m_pViewInverseVariable not valid!\n";


		m_pDiffuseMapVariable = m_pEffect->GetVariableByName("gDiffuseMap")->AsShaderResource();
		if (!m_pDiffuseMapVariable->IsValid())
			std::wcout << L"m_pDiffuseMapVariable not valid!\n";
	}

	Effect::~Effect()
	{
		if (m_pEffect) m_pEffect->Release();
	}


	ID3DX11Effect* Effect::GetEffect()
	{
		return m_pEffect;
	}

	ID3DX11EffectTechnique* Effect::GetTechnique()
	{
		return m_pTechnique;
	}

	void Effect::SetWorldViewProjMatrix(const Matrix& matrix)
	{
		m_pMatWorldViewProjVariable->SetMatrix(reinterpret_cast<const float*>(&matrix));
	}

	void Effect::SetWorldMatrix(const Matrix& matrix)
	{
		m_pWorldVariable->SetMatrix(reinterpret_cast<const float*>(&matrix));
	}

	void Effect::SetViewInvertMatrix(const Matrix& matrix)
	{
		m_pViewInverseVariable->SetMatrix(reinterpret_cast<const float*>(&matrix));
	}

	void Effect::SetDiffuseMap(Texture* pDiffuseTexture)
	{
		if (m_pDiffuseMapVariable)
			m_pDiffuseMapVariable->SetResource(pDiffuseTexture->GetSRV());
	}

	void Effect::CycleTechnique()
	{
		if (static_cast<int>(m_FilterMethod) == 2)
			m_FilterMethod = FilterState(0);
		else
			m_FilterMethod = FilterState(static_cast<int>(m_FilterMethod) + 1);


		switch (m_FilterMethod)
		{
		case FilterState::point:
			{
				m_pTechnique = m_pEffect->GetTechniqueByName("PointFilteringTechnique");
				if (!m_pTechnique->IsValid())
					std::wcout << L"PointFilteringTechnique not valid\n";
				std::wcout << L"PointFilteringTechnique\n";
			}
			break;
		case FilterState::linear:
			{
				m_pTechnique = m_pEffect->GetTechniqueByName("LinearFilteringTechnique");
				if (!m_pTechnique->IsValid()) 
					std::wcout << L"LinearFilteringTechnique not valid\n";
				std::wcout << L"LinearFilteringTechnique\n";
			}
			break;
		case FilterState::anisotropic:
			{
				m_pTechnique = m_pEffect->GetTechniqueByName("AnisotropicFilteringTechnique");
				if (!m_pTechnique->IsValid()) 
					std::wcout << L"AnisotropicFilteringTechnique not valid\n";
				std::wcout << L"AnisotropicFilteringTechnique\n";
			}
			break;
		}
	}

	ID3DX11Effect* Effect::LoadEffect(ID3D11Device* pDevice, const std::wstring& assetFile)
	{
		HRESULT result;
		ID3D10Blob* pErrorBlob{ nullptr };
		ID3DX11Effect* pEffect;
		DWORD shaderFlags = 0;
#if defined( DEBUG ) || defined( _DEBUG )
		shaderFlags |= D3DCOMPILE_DEBUG;
		shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
		result = D3DX11CompileEffectFromFile(assetFile.c_str(),
			nullptr,
			nullptr,
			shaderFlags,
			0,
			pDevice,
			&pEffect,
			&pErrorBlob);
		if (FAILED(result))
		{
			if (pErrorBlob != nullptr)
			{
				const char* pErrors = static_cast<char*>(pErrorBlob->GetBufferPointer());

				std::wstringstream ss;
				for (unsigned int i = 0; i < pErrorBlob->GetBufferSize(); i++)
					ss << pErrors[i];

				OutputDebugStringW(ss.str().c_str());
				pErrorBlob->Release();
				pErrorBlob = nullptr;

				std::wcout << ss.str() << std::endl;
			}
			else
			{
				std::wstringstream ss;
				ss << "EffectLoader: Failed to CreateEffectFromFile!\nPath: " << assetFile;
				std::wcout << ss.str() << std::endl;
				return nullptr;
			}
		}
		return pEffect;
	}
}


