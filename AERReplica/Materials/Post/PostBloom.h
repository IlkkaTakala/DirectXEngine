#pragma once
//Resharper Disable All

	class PostBloom : public PostProcessingMaterial
	{
	public:
		PostBloom();
		~PostBloom() override;
		PostBloom(const PostBloom& other) = delete;
		PostBloom(PostBloom&& other) noexcept = delete;
		PostBloom& operator=(const PostBloom& other) = delete;
		PostBloom& operator=(PostBloom&& other) noexcept = delete;
		
	protected:
		void Initialize(const GameContext& /*gameContext*/) override;
		void Draw(const SceneContext& sceneContext, RenderTarget* pSource) override;

	private:

		RenderTarget* BlurTarget1;
		RenderTarget* BlurTarget2;
	};

