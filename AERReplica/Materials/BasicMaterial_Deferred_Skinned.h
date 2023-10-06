#pragma once
class BasicMaterial_Deferred_Skinned : public Material<BasicMaterial_Deferred_Skinned>
{
public:
	BasicMaterial_Deferred_Skinned();
	~BasicMaterial_Deferred_Skinned() override = default;

	BasicMaterial_Deferred_Skinned(const BasicMaterial_Deferred_Skinned& other) = delete;
	BasicMaterial_Deferred_Skinned(BasicMaterial_Deferred_Skinned&& other) noexcept = delete;
	BasicMaterial_Deferred_Skinned& operator=(const BasicMaterial_Deferred_Skinned& other) = delete;
	BasicMaterial_Deferred_Skinned& operator=(BasicMaterial_Deferred_Skinned&& other) noexcept = delete;

	void SetDiffuseMap(const std::wstring& assetFile);
	void SetDiffuseMap(TextureData* pTextureData);

	void SetNormalMap(const std::wstring& assetFile);
	void SetNormalMap(TextureData* pTextureData);

	void SetSpecularMap(const std::wstring& assetFile);
	void SetSpecularMap(TextureData* pTextureData);

	void UseTransparency(bool enable);

protected:
	void InitializeEffectVariables() override;
	void OnUpdateModelVariables(const SceneContext& /*sceneContext*/, const ModelComponent* /*pModel*/) const;
};

