#pragma once
//Resharper Disable All

class PostFog : public PostProcessingMaterial
{
public:
	PostFog();
	~PostFog() override = default;
	PostFog(const PostFog& other) = delete;
	PostFog(PostFog&& other) noexcept = delete;
	PostFog& operator=(const PostFog& other) = delete;
	PostFog& operator=(PostFog&& other) noexcept = delete;
		
protected:
	void Initialize(const GameContext& /*gameContext*/) override {}
	void UpdateBaseEffectVariables(const SceneContext& sceneContext, RenderTarget* pSource) override;
};

