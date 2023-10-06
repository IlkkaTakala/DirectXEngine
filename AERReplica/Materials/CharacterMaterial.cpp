#include "stdafx.h"
#include "CharacterMaterial.h"

CharacterMaterial::CharacterMaterial() :
	Material(L"Effects/CharacterMaterial.fx")
{
}

void CharacterMaterial::SetDiffuseMap(const std::wstring& assetFile)
{
	SetDiffuseMap(ContentManager::Load<TextureData>(assetFile));
}

void CharacterMaterial::SetDiffuseMap(TextureData* pTextureData)
{
	SetVariable_Texture(L"gDiffuseMap", pTextureData);
}

void CharacterMaterial::InitializeEffectVariables()
{
}

void CharacterMaterial::OnUpdateModelVariables(const SceneContext& /*sceneContext*/, const ModelComponent* pModel) const
{
	if (!pModel->GetAnimator()) return;

	auto& transforms = pModel->GetAnimator()->GetBoneTransforms();
	SetVariable_MatrixArray(L"gBones", (float*)transforms.data(), (UINT)transforms.size());
	GetVariable(L"vertices")->AsShaderResource()->SetResource(pModel->GetShapes());
}
