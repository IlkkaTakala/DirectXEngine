#pragma once
class CharacterMaterial : public Material<CharacterMaterial>
{
public:
	CharacterMaterial();
	~CharacterMaterial() override = default;

	CharacterMaterial(const CharacterMaterial& other) = delete;
	CharacterMaterial(CharacterMaterial&& other) noexcept = delete;
	CharacterMaterial& operator=(const CharacterMaterial& other) = delete;
	CharacterMaterial& operator=(CharacterMaterial&& other) noexcept = delete;

	void SetDiffuseMap(const std::wstring& assetFile);
	void SetDiffuseMap(TextureData* pTextureData);

protected:
	void InitializeEffectVariables() override;
	void OnUpdateModelVariables(const SceneContext& /*sceneContext*/, const ModelComponent* /*pModel*/) const;
};

