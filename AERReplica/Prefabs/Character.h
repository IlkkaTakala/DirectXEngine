#pragma once
struct CharacterDesc
{
	CharacterDesc(
		PxMaterial* pMaterial,
		float radius = .5f,
		float height = 2.f)
	{
		controller.setToDefault();
		controller.radius = radius;
		controller.height = height;
		controller.material = pMaterial;
	}

	float maxMoveSpeed{ 3.f };
	float maxSprintSpeed{ 10.f };
	float maxFallSpeed{ 25.f };

	float JumpSpeed{ 25.f };

	float moveAccelerationTime{ .3f };
	float fallAccelerationTime{ .3f };

	PxCapsuleControllerDesc controller{};

	float rotationSpeed{ 10.f };
	float cameraSpeed{ 35.f };
	float controllerMultiply{ 5.f };

	int actionId_MoveLeft{ -1 };
	int actionId_MoveRight{ -1 };
	int actionId_MoveForward{ -1 };
	int actionId_MoveBackward{ -1 };
	int actionId_Jump{ -1 };
};

struct HumanAnimator;
struct BirdAnimator;

class Character : public GameObject
{
public:
	Character(const CharacterDesc& characterDesc);
	~Character() override;

	Character(const Character& other) = delete;
	Character(Character&& other) noexcept = delete;
	Character& operator=(const Character& other) = delete;
	Character& operator=(Character&& other) noexcept = delete;

	void DrawImGui();
	void ShowControls();

	void GiveScore();
	void ClearScore(int max) { Score = 0; MaxScore = max; }
	FollowCamera* GetCamera() const { return m_pCameraComponent; }

	float GetSpeed() const;
	XMFLOAT3 GetDirection() const { return m_LocalDirection; }
	bool IsFalling() const { return isJumping; }
	GameObject* m_pbirdHolder{};

	void Restart();

protected:
	void Initialize(const SceneContext&) override;
	void Update(const SceneContext&) override;
	void PostDraw(const SceneContext&) override;

private:

	int Score;
	int MaxScore;

	void Move(XMFLOAT2 value);
	void Pause();
	void Jump();
	void ToHuman();
	void ToBird();

	void ShowPauseMenu(bool show);
	bool PauseMenu{false};
	bool GameEnd{false};

	bool isJumping{ false };
	bool hasJumped{ false };

	float fading;

	bool isBird{ false };

	FMOD::Sound* transformSound;
	FMOD::Sound* windSound;
	FMOD::Channel* windChannel;

	FollowCamera* m_pCameraComponent{};
	ControllerComponent* m_pControllerComponent{};
	ModelComponent* m_pMesh{};
	ModelComponent* m_pMeshBird{};
	InputComponent* m_pInput{};

	GameObject* Controls;

	HumanAnimator* hanim{};
	BirdAnimator* banim{};

	AnimationInstance inst;

	SpriteFont* m_pFont;
	SpriteFont* m_pFontBold;

	ParticleSystemComponent* TransformEffect;

	CharacterDesc m_CharacterDesc;
	float m_TotalPitch{}, m_TotalYaw{};				//Total camera Pitch(X) and Yaw(Y) rotation
	float m_FlightPitch{}, m_FlightYaw{};				//Total camera Pitch(X) and Yaw(Y) rotation
	float m_MoveAcceleration{},						//Acceleration required to reach maxMoveVelocity after 1 second (maxMoveVelocity / moveAccelerationTime)
		m_FallAcceleration{},						//Acceleration required to reach maxFallVelocity after 1 second (maxFallVelocity / fallAccelerationTime)
		m_MoveSpeed{};								//MoveSpeed > Horizontal Velocity = MoveDirection * MoveVelocity (= TotalVelocity.xz)
	bool m_Sprinting{};

	XMFLOAT3 m_TotalVelocity{};						//TotalVelocity with X/Z for Horizontal Movement AND Y for Vertical Movement (fall/jump)
	float m_TotalFallVelocity{};						//TotalVelocity with X/Z for Horizontal Movement AND Y for Vertical Movement (fall/jump)
	XMFLOAT3 m_CurrentDirection{};					//Current/Last Direction based on Camera forward/right (Stored for deacceleration)
	XMFLOAT3 m_LocalDirection{};
	XMFLOAT3 m_InputDirection{};
	XMFLOAT2 m_Look{};
};


struct HumanAnimator
{
	AnimationStateMachine state;
	AnimationInstance idle;
	AnimationInstance walk;
	AnimationInstance run;
	AnimationInstance jump;

	Character* owner;

	FMOD::Sound* footsteps[3];

	void operator()(float delta, BoneArray bones) {
		/*idle.Update(delta, 1.f);
		walk.Update(delta, 1.f);
		run.Update(delta, 1.f);
		jump.Update(delta, 1.f);*/
		state.Evaluate(delta, bones);
	}
	void PlayFootstep();

	HumanAnimator(ModelAnimator* anima, Character* c);
	~HumanAnimator();
};
struct BirdAnimator
{
	AnimationStateMachine state;
	AnimationInstance flap;
	AnimationInstance dive;
	AnimationInstance turnL;
	AnimationInstance turnR;

	Character* owner;
	FMOD::Sound* flaps[3];
	void PlayFlaps();

	void operator()(float delta, BoneArray bones) {
		state.Evaluate(delta, bones );
	}

	BirdAnimator(ModelAnimator* anima, Character* c);
	~BirdAnimator();
};
