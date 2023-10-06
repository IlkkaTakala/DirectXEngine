#include "stdafx.h"
#include "ShadowMapRenderer.h"
#include "Misc/ShadowMapMaterial.h"

ShadowMapRenderer::~ShadowMapRenderer()
{
	SafeDelete(m_pShadowRenderTarget);
	SafeDelete(m_pFarShadowRenderTarget)
}

void ShadowMapRenderer::Initialize()
{
	m_pShadowRenderTarget = new RenderTarget(m_GameContext.d3dContext);
	m_pFarShadowRenderTarget = new RenderTarget(m_GameContext.d3dContext);

	m_Width = 2048;
	m_Height = 2048;
	m_pAspect = (float)m_Width / (float)m_Height;

	RENDERTARGET_DESC rtDesc;
	rtDesc.enableColorBuffer = false;
	rtDesc.enableDepthSRV = true;
	rtDesc.width = m_Width;
	rtDesc.height = m_Height;
	HANDLE_ERROR(m_pShadowRenderTarget->Create(rtDesc));
	HANDLE_ERROR(m_pFarShadowRenderTarget->Create(rtDesc));

	m_pShadowMapGenerator = MaterialManager::Get()->CreateMaterial<ShadowMapMaterial>();

	m_GeneratorTechniqueContexts[(int)ShadowGeneratorType::Static] = m_pShadowMapGenerator->GetTechniqueContext((int)ShadowGeneratorType::Static);
	m_GeneratorTechniqueContexts[(int)ShadowGeneratorType::Skinned] = m_pShadowMapGenerator->GetTechniqueContext((int)ShadowGeneratorType::Skinned);
}

void ShadowMapRenderer::UpdateMeshFilter(const SceneContext& sceneContext, MeshFilter* pMeshFilter) const
{
	ShadowGeneratorType Type = ShadowGeneratorType::Static;
	if (pMeshFilter->HasAnimations()) Type = ShadowGeneratorType::Skinned;

	const auto& Mat = m_GeneratorTechniqueContexts[(int)Type];
	pMeshFilter->BuildVertexBuffer(sceneContext, Mat.inputLayoutID, Mat.inputLayoutSize, Mat.pInputLayoutDescriptions);
}

void ShadowMapRenderer::Begin(const SceneContext& sceneContext)
{
	constexpr ID3D11ShaderResourceView* const pSRV[] = { nullptr };
	D3D11_VIEWPORT m_Viewport;
	m_Viewport.Width = static_cast<FLOAT>(m_Width);
	m_Viewport.Height = static_cast<FLOAT>(m_Height);
	m_Viewport.TopLeftX = 0;
	m_Viewport.TopLeftY = 0;
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;
	sceneContext.d3dContext.pDeviceContext->RSSetViewports(1, &m_Viewport);
	sceneContext.d3dContext.pDeviceContext->PSSetShaderResources(1, 1, pSRV);

	const auto Proj = XMMatrixOrthographicLH(25.f * m_pAspect, 25.f, 0.1f, 200.f);
	const auto& Light = sceneContext.pLights->GetDirectionalLight();
	const auto& Cam = sceneContext.pCamera;
	const auto World = XMLoadFloat4x4(&Cam->GetView());
	const auto Dir = XMLoadFloat4(&Light.direction);
	auto forw = Cam->GetTransform()->GetForward();
	forw.y = 0.f;
	const auto Pos = XMLoadFloat3(&Cam->GetTransform()->GetWorldPosition()) + XMLoadFloat3(&forw) * 15.f - Dir * 100.f;
	const auto View = XMMatrixLookAtLH(Pos, Pos + Dir, {0, 1, 0});

	const auto FarProj = XMMatrixOrthographicLH(300.f * m_pAspect, 300.f, 0.1f, 1000.f);
	const auto FarPos = XMLoadFloat3(&Cam->GetTransform()->GetWorldPosition()) + XMLoadFloat3(&forw) * 150.f - Dir * 200.f;
	const auto FarView = XMMatrixLookAtLH(FarPos, FarPos + Dir, {0, 1, 0});

	XMStoreFloat4x4(&m_LightVP, View * Proj);
	XMStoreFloat4x4(&m_FarLightVP, FarView * FarProj);

	m_pShadowMapGenerator->SetVariable_Matrix(L"gLightViewProj", m_LightVP);
	m_pShadowMapGenerator->SetVariable_Matrix(L"gFarLightViewProj", m_FarLightVP);

	m_GameContext.pGame->SetRenderTarget(m_pShadowRenderTarget);
	m_pShadowRenderTarget->Clear();
	m_pFarShadowRenderTarget->Clear();
}

void ShadowMapRenderer::DrawMesh(const SceneContext& sceneContext, MeshFilter* pMeshFilter, const XMFLOAT4X4& meshWorld, const std::vector<XMFLOAT4X4>& meshBones)
{
	//This function is called for every mesh that needs to be rendered on the shadowmap (= cast shadows)

	ShadowGeneratorType Type = pMeshFilter->HasAnimations() ? ShadowGeneratorType::Skinned : ShadowGeneratorType::Static;
	const auto& Tech = m_GeneratorTechniqueContexts[(int)Type];
	m_pShadowMapGenerator->SetVariable_Matrix(L"gWorld", meshWorld);
	if (Type == ShadowGeneratorType::Skinned) {
		m_pShadowMapGenerator->SetVariable_MatrixArray(L"gBones", (float*)meshBones.data(), (UINT)meshBones.size());
	}
	const auto pDeviceContext = sceneContext.d3dContext.pDeviceContext;

	pDeviceContext->IASetInputLayout(Tech.pInputLayout);

	pDeviceContext->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	for (const auto& subMesh : pMeshFilter->GetMeshes())
	{
		const UINT offset = 0;
		m_pShadowMapGenerator->SetTechnique((int)Type);
		const auto& vertexBufferData = pMeshFilter->GetVertexBufferData(sceneContext, m_pShadowMapGenerator, subMesh.id);
		pDeviceContext->IASetVertexBuffers(0, 1, &vertexBufferData.pVertexBuffer, &vertexBufferData.VertexStride,
			&offset);

		pDeviceContext->IASetIndexBuffer(subMesh.buffers.pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

		D3DX11_TECHNIQUE_DESC techDesc{};

		Tech.pTechnique->GetDesc(&techDesc);
		m_GameContext.pGame->SetRenderTarget(m_pShadowRenderTarget);
		for (UINT p = 0; p < techDesc.Passes; ++p)
		{
			if (p > 0)
				m_GameContext.pGame->SetRenderTarget(m_pFarShadowRenderTarget);
			Tech.pTechnique->GetPassByIndex(p)->Apply(0, pDeviceContext);
			pDeviceContext->DrawIndexed(subMesh.indexCount, 0, 0);
		}
	}
}

void ShadowMapRenderer::End(const SceneContext&) const
{
	m_GameContext.pGame->SetRenderTarget(nullptr);
	m_GameContext.pGame->ResetViewport();
}

ID3D11ShaderResourceView* ShadowMapRenderer::GetShadowMap() const
{
	return m_pShadowRenderTarget->GetDepthShaderResourceView();
}

ID3D11ShaderResourceView* ShadowMapRenderer::GetFarShadowMap() const
{
	return m_pFarShadowRenderTarget->GetDepthShaderResourceView();;
}

void ShadowMapRenderer::Debug_DrawDepthSRV(const XMFLOAT2& position, const XMFLOAT2& scale, const XMFLOAT2& pivot) const
{
	if (m_pFarShadowRenderTarget->HasDepthSRV())
	{
		SpriteRenderer::Get()->DrawImmediate(m_GameContext.d3dContext, m_pFarShadowRenderTarget->GetDepthShaderResourceView(), position, XMFLOAT4{ Colors::White }, pivot, scale);

		//Remove from Pipeline
		constexpr ID3D11ShaderResourceView* const pSRV[] = { nullptr };
		m_GameContext.d3dContext.pDeviceContext->PSSetShaderResources(0, 1, pSRV);
	}
}
