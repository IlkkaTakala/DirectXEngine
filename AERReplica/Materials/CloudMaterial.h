#pragma once
class CloudMaterial : public Material<CloudMaterial>
{
public:
	CloudMaterial();
	~CloudMaterial() override = default;

	CloudMaterial(const CloudMaterial& other) = delete;
	CloudMaterial(CloudMaterial&& other) noexcept = delete;
	CloudMaterial& operator=(const CloudMaterial& other) = delete;
	CloudMaterial& operator=(CloudMaterial&& other) noexcept = delete;

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

