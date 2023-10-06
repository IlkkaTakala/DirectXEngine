#include "stdafx.h"
#include "Character.h"
#include "Materials/CharacterMaterial.h"

Character::Character(const CharacterDesc& characterDesc) :
	m_CharacterDesc{ characterDesc },
	m_MoveAcceleration(characterDesc.maxMoveSpeed / characterDesc.moveAccelerationTime),
	m_FallAcceleration(characterDesc.maxFallSpeed / characterDesc.fallAccelerationTime),
	MaxScore(2), Score(0)
{}

Character::~Character()
{
	delete hanim;
	delete banim;

	transformSound->release();
	windSound->release();
}

void Character::Restart()
{
	isBird = false;
	m_pCameraComponent->SetOffset({ 0, 0, -5 }, 0.2f);
	windChannel->setPaused(true);

	InputManager::CursorVisible(false);

	m_pMesh->Activate();
	m_pMeshBird->Deactivate();

	m_TotalVelocity = XMFLOAT3{ 0.f, 0.f, 0.f };
	m_TotalFallVelocity = 0.f;
	GameEnd = false;
}

void Character::Initialize(const SceneContext& sceneContext)
{
	m_pFont = ContentManager::Load<SpriteFont>(L"SpriteFonts/Goudy_32.fnt");
	m_pFontBold = ContentManager::Load<SpriteFont>(L"SpriteFonts/Goudy_Bold.fnt");

	//Controller
	m_pControllerComponent = AddComponent(new ControllerComponent(m_CharacterDesc.controller));

	//Camera
	m_pCameraComponent = AddChild(new FollowCamera());
	m_pCameraComponent->SetActive(true); //Uncomment to make this camera the active camera

	m_pCameraComponent->GetTransform()->Translate(0.f, m_CharacterDesc.controller.height * .3f, 0.f);
	m_pCameraComponent->SetOffset({ 0, 0, -5 }, 0.2f);

	const auto pPeasantMaterial = MaterialManager::Get()->CreateMaterial<CharacterMaterial>();
	pPeasantMaterial->SetDiffuseMap(L"Textures/player_texture.png");
	m_pMesh = AddChild(new GameObject())->AddComponent(new ModelComponent(L"Meshes/Player.ovm"));
	m_pMesh->SetMaterial(pPeasantMaterial);
	m_pMesh->GetTransform()->Translate(0, -(m_CharacterDesc.controller.height * 0.6f + m_CharacterDesc.controller.radius), 0);
	//m_pMesh->GetTransform()->Scale(.1f);
	if (const auto pAnimator = m_pMesh->GetAnimator())
	{
		hanim = new HumanAnimator(pAnimator, this);
		pAnimator->Updator = [this](float delta, BoneArray bones) { (*hanim)(delta, bones); };
		/*inst = &pAnimator->GetClip(0);
		pAnimator->Updator = [this](float delta, BoneArray bones) { 
			inst.Update(delta);
			inst.MakeTransforms(bones);
		};*/
	}
	m_pMesh->AddBlendShape(L"Meshes/Bird.ovm");

	m_pbirdHolder = AddChild(new GameObject());
	const auto pBirdMaterial = MaterialManager::Get()->CreateMaterial<CharacterMaterial>();
	pBirdMaterial->SetDiffuseMap(L"Textures/bird_texture.png");
	m_pMeshBird = m_pbirdHolder->AddChild(new GameObject())->AddComponent(new ModelComponent(L"Meshes/Bird.ovm"));
	//m_pMeshBird->GetTransform()->Translate(0, -m_CharacterDesc.controller.height * 0.6f, 0);
	m_pMeshBird->GetTransform()->Rotate(0, 180, 0);
	m_pMeshBird->SetMaterial(pBirdMaterial);
	if (const auto pAnimator = m_pMeshBird->GetAnimator())
	{
		banim = new BirdAnimator(pAnimator, this);
		pAnimator->Updator = [this](float delta, BoneArray bones) { (*banim)(delta, bones); };
	}
	m_pMeshBird->Deactivate();
	m_pMeshBird->AddBlendShape(L"Meshes/Player.ovm");

	m_pInput = AddComponent(new InputComponent());

	m_pInput->BindAction("Jump", std::bind(&Character::Jump, this));
	m_pInput->BindAction("Move", std::bind(&Character::Move, this, std::placeholders::_1));
	//m_pInput->BindAction("Mainmenu", [](XMFLOAT2) {
	//	SceneManager::Get()->SetActiveGameScene(L"MainMenu");
	//	});
	m_pInput->BindAction("Sprint", [this](XMFLOAT2) {
		m_Sprinting = true;
		});
	m_pInput->BindAction("Look", [this](XMFLOAT2 value) {
		m_Look = value;
		});
	m_pInput->BindAction("CLook", [this](XMFLOAT2 value) {
		m_Look.x = value.x * m_CharacterDesc.controllerMultiply;
		m_Look.y = -value.y * m_CharacterDesc.controllerMultiply;
		});
	m_pInput->BindAction("Pause", std::bind(&Character::Pause, this));
	m_pInput->BindAction("Hide", [this](XMFLOAT2) {
		Controls->Deactivate();
		InputManager::SetUserMapping(0, "Default");
		});

	InputManager::RegisterInputComponent(m_pInput, 0);

	InputManager::SetUserMapping(0, "Controls");

	Controls = GetScene()->AddChild(new GameObject());
	Controls->AddComponent(new SpriteComponent(L"Textures/Controls.png", { 0.5f, 0.5f }));
	Controls->GetTransform()->Translate(sceneContext.windowWidth * 0.5f, sceneContext.windowHeight * 0.5f, 0.f);

	const auto updator1 = [](ParticleEmitter* system, float delta) {
		ParticleSystemConstruction::Updator u(system);
		u.UpdateLifetime(delta);
		u.UpdateVelocities(delta);
		u.Alpha(delta, CurveData({ {0.f, 0.f}, {0.1f, 1.f}, { 1.f, 0.f } }));
		//u.Color(delta, VectorCurveData({ {0.f, {1.f}} }));
		u.SpriteSize(delta, CurveData({ {0.f, 0.3f}, {1.f, 1.0f} }));
	};

	const auto constructor1 = [](ParticleEmitter* /*sys*/, Particle& p) {
		p.initialSize = MathHelper::randF(.1f, .3f);
		p.max_lifetime = MathHelper::randF(0.1f, 0.5);
		p.isActive = false;
		p.lifetime = 0.f;
		ParticleSystemConstruction::Constructor::SphereLocation(p,
			MathHelper::randF(.1f, 0.5f)
		);
		XMStoreFloat3(&p.velocity, XMVector3Normalize(XMLoadFloat3(&p.vertexInfo.Position)) * MathHelper::randF(.5f, 1.5f));
		p.alpha = 0.f;
		p.initialColour = { 10.f, 10.f, 10.f };
		p.vertexInfo.Color = { 10.f, 10.f, 10.f, 10.f };
	};

	auto pParticle1 = AddChild(new GameObject);
	TransformEffect = pParticle1->AddComponent(new ParticleSystemComponent());
	auto emit = TransformEffect->AddEmitter({ L"Textures/Star.png" , constructor1 , updator1 }, [] { return ParticleSystemConstruction::MakeSpawnBurst(150); });
	emit->SetIsLocalSpace(true);
	TransformEffect->Deactivate();

	auto Fmod = SoundManager::Get()->GetSystem();

	FMOD_RESULT res = Fmod->createStream("Resources/Sounds/cam_wind.Wav", FMOD_DEFAULT | FMOD_LOOP_NORMAL, nullptr, &windSound);
	SoundManager::Get()->ErrorCheck(res);
	res = Fmod->createStream("Resources/Sounds/transform.Wav", FMOD_DEFAULT, nullptr, &transformSound);
	SoundManager::Get()->ErrorCheck(res);

	Fmod->playSound(windSound, nullptr, false, &windChannel);
	windChannel->setVolume(0.f);
}

void Character::Update(const SceneContext& sceneContext)
{
	if (m_pCameraComponent->IsActiveCamera() && !GameEnd)
	{
		constexpr float epsilon{ 0.01f }; //Constant that can be used to compare if a float is near zero

		//***************
		//HANDLE INPUT

		XMFLOAT2 move{ 0, 0 };
		move.y = m_LocalDirection.z = m_InputDirection.z;
		move.x = m_LocalDirection.x = m_InputDirection.x;
		m_InputDirection = {0,0,0};
		//## Input Gathering (look)
		XMFLOAT2 look = m_Look;
		m_Look = { 0.f, 0.f };

		if (fading > 0.f) {
			fading -= sceneContext.pGameTime->GetElapsed();
			
			float whiteOut = sin(fading * 6.f);
			m_pMeshBird->GetMaterial()->SetVariable_Scalar(L"gWhiteout", whiteOut);
			m_pMesh->GetMaterial()->SetVariable_Scalar(L"gWhiteout", whiteOut);
		}
		else {
			m_pMeshBird->GetMaterial()->SetVariable_Scalar(L"gWhiteout", 0.f);
			m_pMesh->GetMaterial()->SetVariable_Scalar(L"gWhiteout", 0.f);
		}

		if (isBird) {
			if (!isJumping) ToHuman();

			XMFLOAT3 c_for = m_pbirdHolder->GetTransform()->GetForward(); 
			const auto rotMat = XMMatrixRotationQuaternion(XMQuaternionRotationRollPitchYaw(XMConvertToRadians(m_FlightPitch), XMConvertToRadians(m_FlightYaw), 0.f));
			const auto forward = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), rotMat);
			const auto right = XMVector3TransformCoord(XMVectorSet(1, 0, 0, 0), rotMat);

			auto accel = m_MoveAcceleration * sceneContext.pGameTime->GetElapsed();
			XMStoreFloat3(&m_CurrentDirection, XMVector3Normalize(XMVectorAdd(forward, right * move.x)));

			auto rotator = m_CurrentDirection;
			auto forw = XMLoadFloat3(&rotator);
			auto right1 = XMVector3Cross(forward, {0,1,0});
			auto up = XMVector3Cross(forward, right);
			XMMATRIX mat = { forw, up, right1, {0,0,0} };

			m_FlightYaw += move.x * 60.f * sceneContext.pGameTime->GetElapsed();
			m_FlightPitch -= move.y * 60.f * sceneContext.pGameTime->GetElapsed();

			float limit = 40.f;
			if (m_FlightPitch < -limit)
				m_FlightPitch = -limit;
			if (m_FlightPitch > limit)
				m_FlightPitch = limit;
			m_pbirdHolder->GetTransform()->Rotate(m_FlightPitch, m_FlightYaw, 0.f);

			windChannel->setVolume(PxClamp(m_FlightPitch, 5.f, 40.f) / 40.f);
			m_pMesh->GetTransform()->Rotate(0.f, m_FlightYaw - 180.f, 0.f);

			m_MoveSpeed += accel;
			float max = m_CharacterDesc.maxSprintSpeed * 2.f * (m_CurrentDirection.y < 0.1f ? 1 + -m_CurrentDirection.y : 1.f);
			if (m_MoveSpeed > max) m_MoveSpeed = max;

			XMFLOAT3 velocity;
			XMStoreFloat3(&velocity, XMLoadFloat3(&m_CurrentDirection) * m_MoveSpeed);
			m_TotalVelocity.x = velocity.x;
			m_TotalVelocity.y = velocity.y + m_CurrentDirection.y * 5.f;
			m_TotalVelocity.z = velocity.z;

			//## Vertical Movement (Jump/Fall)
			isJumping = !m_pControllerComponent->GetCollisionFlags().isSet(PxControllerCollisionFlag::eCOLLISION_DOWN);
			m_TotalFallVelocity -= m_FallAcceleration * 0.05f * sceneContext.pGameTime->GetElapsed();
			if (m_TotalFallVelocity < -m_CharacterDesc.maxFallSpeed * 0.1f) m_TotalFallVelocity = -m_CharacterDesc.maxFallSpeed * 0.1f;

			//************
			//DISPLACEMENT

			XMFLOAT3 total = m_TotalVelocity;
			total.y += m_TotalFallVelocity;
			XMFLOAT3 dist;
			XMStoreFloat3(&dist, XMLoadFloat3(&total) * sceneContext.pGameTime->GetElapsed());
			m_pControllerComponent->Move(dist);
		}
		else {
			//************************
			//GATHERING TRANSFORM INFO
			XMFLOAT3 c_for = m_pCameraComponent->GetTransform()->GetForward();
			c_for.y = 0.f;
			XMVECTOR forward = XMLoadFloat3(&c_for);
			forward = XMVector3Normalize(forward);
			XMVECTOR right = XMLoadFloat3(&m_pCameraComponent->GetTransform()->GetRight());

			//********
			//MOVEMENT
			isJumping = !m_pControllerComponent->GetCollisionFlags().isSet(PxControllerCollisionFlag::eCOLLISION_DOWN);

			//## Horizontal Velocity (Forward/Backward/Right/Left)
			auto accel = m_MoveAcceleration * sceneContext.pGameTime->GetElapsed();
			if (abs(move.x) > epsilon || abs(move.y) > epsilon) {
				XMStoreFloat3(&m_CurrentDirection, XMVector3Normalize(XMVectorAdd(forward * move.y, right * move.x)));

				auto rotator = m_CurrentDirection;
				rotator.y = 0.f;
				rotator.x = -rotator.x;
				auto rot = XMQuaternionSlerp(
					XMLoadFloat4(&m_pMesh->GetTransform()->GetRotation()),
					XMQuaternionRotationMatrix(XMMatrixLookToRH(XMLoadFloat3(&GetTransform()->GetPosition()), XMLoadFloat3(&rotator), { 0,1,0 })),
					sceneContext.pGameTime->GetElapsed() * m_CharacterDesc.rotationSpeed);
				m_pMesh->GetTransform()->Rotate(rot);

				m_MoveSpeed += accel;
				if (m_Sprinting) {
					if (m_MoveSpeed > m_CharacterDesc.maxSprintSpeed) m_MoveSpeed = m_CharacterDesc.maxSprintSpeed;
				} else 
					if (m_MoveSpeed > m_CharacterDesc.maxMoveSpeed) m_MoveSpeed = m_CharacterDesc.maxMoveSpeed;
			}
			else {
				m_MoveSpeed -= accel * (isJumping ? 0.1f : 5.f);
				if (m_MoveSpeed < 0.f) m_MoveSpeed = 0.f;
			}

			XMFLOAT3 velocity;
			XMStoreFloat3(&velocity, XMLoadFloat3(&m_CurrentDirection) * m_MoveSpeed);
			m_TotalVelocity.x = velocity.x;
			m_TotalVelocity.z = velocity.z;

			//## Vertical Movement (Jump/Fall)
			if (isJumping) {
				m_TotalVelocity.y -= m_FallAcceleration * sceneContext.pGameTime->GetElapsed();
				if (m_TotalVelocity.y < -m_CharacterDesc.maxFallSpeed) m_TotalVelocity.y = -m_CharacterDesc.maxFallSpeed;
			}
			else if (hasJumped) {
				hasJumped = false;
				m_TotalVelocity.y = m_CharacterDesc.JumpSpeed;
			}
			else {
				m_TotalVelocity.y = -1.1f;
			}
			//************
			//DISPLACEMENT

			XMFLOAT3 dist;
			XMStoreFloat3(&dist, XMLoadFloat3(&m_TotalVelocity) * sceneContext.pGameTime->GetElapsed());
			m_pControllerComponent->Move(dist);
			m_Sprinting = false;
		}

		m_TotalYaw += look.x * m_CharacterDesc.rotationSpeed * sceneContext.pGameTime->GetElapsed();
		m_TotalPitch += look.y * m_CharacterDesc.rotationSpeed * sceneContext.pGameTime->GetElapsed();

		float limit = 80.f;
		if (m_TotalPitch < -limit)
			m_TotalPitch = -limit;
		if (m_TotalPitch > limit)
			m_TotalPitch = limit;
		m_pCameraComponent->GetTransform()->Rotate(XMFLOAT3{ m_TotalPitch, m_TotalYaw, 0.f });

		auto& p = m_pCameraComponent->GetTransform()->GetWorldPosition();
		auto& f = m_pCameraComponent->GetTransform()->GetForward();
		auto& u = m_pCameraComponent->GetTransform()->GetUp();

		FMOD_VECTOR Pos = { p.x, p.y, p.z };
		FMOD_VECTOR Vel = { m_TotalVelocity.x, m_TotalVelocity.y, m_TotalVelocity.z };
		FMOD_VECTOR Forward = { f.x, f.y, f.z };
		FMOD_VECTOR Up = { u.x, u.y, u.z };

		SoundManager::Get()->GetSystem()->set3DListenerAttributes(0,
			&Pos,
			&Vel,
			&Forward,
			&Up
		);
	}
}

void Character::PostDraw(const SceneContext&)
{
	if (Controls->IsActive()) return;

	TextRenderer::Get()->DrawText(m_pFont, L"Collected fragments", {10, 10}, (XMFLOAT4)Colors::ForestGreen);
	TextRenderer::Get()->DrawText(m_pFontBold, std::format(L"{}/{}", Score, MaxScore), {140, 60}, (XMFLOAT4)Colors::ForestGreen);
}

void Character::Move(XMFLOAT2 value)
{
	m_InputDirection.x = value.x;
	m_InputDirection.z = value.y;
}

void Character::Pause()
{
	auto time = GetScene()->GetSceneContext().pGameTime;
	if (time->IsRunning()) {
		time->Stop();
		ShowPauseMenu(true);
		InputManager::CursorVisible(true);
		InputManager::ForceMouseToCenter(false);
	}
	else {
		time->Start();
		ShowPauseMenu(false);
		InputManager::CursorVisible(false);
		InputManager::ForceMouseToCenter(true);
	}
}

void Character::Jump()
{
	if (isBird) {
		ToHuman();
	}
	else {
		if (isJumping) {
			ToBird();
		}
		else {
			hasJumped = true;
		}
	}
}

void Character::ToHuman()
{
	isBird = false;
	TransformEffect->Reset();
	TransformEffect->Activate();
	m_pCameraComponent->SetOffset({ 0, 0, -5 }, 0.2f);
	fading = XM_PI / 6.f;
	auto Fmod = SoundManager::Get()->GetSystem();
	FMOD::Channel* c;
	Fmod->playSound(transformSound, nullptr, false, &c);
	c->setVolume(0.3f);
	windChannel->setPaused(true);

	GetScene()->SetTimerByEvent(this, fading * 0.5f, [this]() {
		m_pMesh->Activate();
		m_pMeshBird->Deactivate();
	});
}

void Character::ToBird()
{
	isBird = true;
	TransformEffect->Reset();
	TransformEffect->Activate();
	m_pCameraComponent->SetOffset({ 0, 0, -10 }, 0.2f);
	fading = XM_PI / 6.f;
	m_FlightPitch = 0.f;
	m_FlightYaw = m_TotalYaw;
	m_pbirdHolder->GetTransform()->Rotate(m_FlightPitch, m_FlightYaw, 0.f);
	auto Fmod = SoundManager::Get()->GetSystem();
	FMOD::Channel* c;
	Fmod->playSound(transformSound, nullptr, false, &c);
	c->setVolume(0.3f);

	windChannel->setPaused(false);

	GetScene()->SetTimerByEvent(this, fading * 0.5f, [this]() {
		m_pMesh->Deactivate();
		m_pMeshBird->Activate();
	});
}

void Character::ShowPauseMenu(bool show)
{
	PauseMenu = show;
}

void Character::DrawImGui()
{
	using namespace ImGui;
	if (PauseMenu) {

		static ImGuiWindowFlags flags = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize;

		auto& sceneContext = GetScene()->GetSceneContext();

		SetNextWindowPos({ sceneContext.windowWidth * 0.5f, sceneContext.windowHeight * 0.5f }, ImGuiCond_Always, { 0.5f, 0.5f });
		Begin("Controls", nullptr, flags);
		PushFont(GetScene()->GetFont("title"));
		static std::string text = "Paused";
		auto windowWidth = ImGui::GetWindowSize().x;
		auto textWidth = ImGui::CalcTextSize(text.c_str()).x;

		ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
		TextColored({ 0.7f, 0.7f, 0.7f, 1.0f }, text.c_str());
		PopFont();
		Dummy({ 0.f, 20.f });

		PushFont(GetScene()->GetFont("menu"));
		if (Button("Continue", { 250, 65 })) {
			Pause();
		}
		if (Button("Controls", { 250, 65 })) {
			Pause();
			ShowControls();
		}
		Dummy({ 0.f, 20.f });
		if (Button("Restart", { 250, 65 })) {
			Pause();
			SceneManager::Get()->SetActiveGameScene(L"AERGameScene");
		}
		if (Button("Main menu", { 250, 65 })) {
			Pause();
			SceneManager::Get()->SetActiveGameScene(L"MainMenu");
		}
		if (Button("Exit", { 250, 65 })) {
			PostQuitMessage(0);
		}
		PopFont();

		End();
	}
	else if (GameEnd) {
		static ImGuiWindowFlags flags = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize;

		auto& sceneContext = GetScene()->GetSceneContext();

		SetNextWindowPos({ sceneContext.windowWidth * 0.5f, sceneContext.windowHeight * 0.5f }, ImGuiCond_Always, { 0.5f, 0.5f });
		Begin("End", nullptr, flags);
		PushFont(GetScene()->GetFont("title"));
		static std::string text = "Game ended";
		auto windowWidth = ImGui::GetWindowSize().x;
		auto textWidth = ImGui::CalcTextSize(text.c_str()).x;

		ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
		TextColored({ 0.2f, 0.2f, 0.2f, 1.0f }, text.c_str());
		PopFont();
		PushFont(GetScene()->GetFont("menu"));
		static std::string text2 = "Will you play again?";
		textWidth = ImGui::CalcTextSize(text2.c_str()).x;
		ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
		TextColored({ 0.2f, 0.2f, 0.2f, 1.0f }, text2.c_str());
		Dummy({ 0.f, 20.f });
		if (Button("Restart", { 450, 65 })) {
			SceneManager::Get()->SetActiveGameScene(L"AERGameScene");
		}
		if (Button("Main menu", { 450, 65 })) {
			SceneManager::Get()->SetActiveGameScene(L"MainMenu");
		}
		if (Button("Exit", { 450, 65 })) {
			PostQuitMessage(0);
		}
		PopFont();
		End();
	}
}

void Character::ShowControls()
{
	InputManager::SetUserMapping(0, "Controls");
	Controls->Activate();
}

void Character::GiveScore()
{
	Score++;

	if (Score >= MaxScore) {
		InputManager::SetUserMapping(0, "End");
		InputManager::CursorVisible(true);
		GameEnd = true;
	}
}

float Character::GetSpeed() const
{
	float speed;
	XMFLOAT2 vel;
	vel.x = m_TotalVelocity.x;
	vel.y = m_TotalVelocity.z;
	XMStoreFloat(&speed, XMVector2Length(XMLoadFloat2(&vel)));
	return speed;
}

void HumanAnimator::PlayFootstep() {
	auto Fmod = SoundManager::Get()->GetSystem();
	FMOD::Channel* c;
	Fmod->playSound(footsteps[rand() % 3], nullptr, false, &c);
	c->setVolume(0.5f);
}

HumanAnimator::HumanAnimator(ModelAnimator* anima, Character* c) : owner(c)
{
	auto Fmod = SoundManager::Get()->GetSystem();

	FMOD_RESULT res = Fmod->createStream("Resources/Sounds/foot01.Wav", FMOD_DEFAULT, nullptr, &footsteps[0]);
	SoundManager::Get()->ErrorCheck(res);
	res = Fmod->createStream("Resources/Sounds/foot02.Wav", FMOD_DEFAULT, nullptr, &footsteps[1]);
	SoundManager::Get()->ErrorCheck(res);
	res = Fmod->createStream("Resources/Sounds/foot03.Wav", FMOD_DEFAULT, nullptr, &footsteps[2]);
	SoundManager::Get()->ErrorCheck(res);

	idle = &anima->GetClip(3);
	walk = &anima->GetClip(8);
	walk.AddNotify(15, std::bind(&HumanAnimator::PlayFootstep, this));
	walk.AddNotify(47, std::bind(&HumanAnimator::PlayFootstep, this));
	run = &anima->GetClip(6);
	run.AddNotify(29, std::bind(&HumanAnimator::PlayFootstep, this));
	run.AddNotify(58, std::bind(&HumanAnimator::PlayFootstep, this));
	jump = &anima->GetClip(1);

	state.AddState("idle", [this](float delta, BoneArray bones) { idle.Update(delta); idle.MakeTransforms(bones); });
	state.AddState("walk", [this](float delta, BoneArray bones) { walk.Update(delta); walk.MakeTransforms(bones); });
	state.AddState("run", [this](float delta, BoneArray bones) { run.Update(delta); run.MakeTransforms(bones); });
	state.AddState("jump", [this](float delta, BoneArray bones) { jump.Update(delta); jump.MakeTransforms(bones); });

	state.AddPaths("idle", {
		{"walk", [this]() -> bool { return owner->GetSpeed() > 1.f; }, 0.2f},
		{"jump", [this]() -> bool { return owner->IsFalling(); }, 0.0f},
		});
	state.AddPaths("walk", {
		{"jump", [this]() -> bool { return owner->IsFalling(); }, 0.0f},
		{"idle", [this]() -> bool { return owner->GetSpeed() <= 1.f; }, 0.2f} ,
		{"run", [this]() -> bool { return owner->GetSpeed() >= 6.f; }, 0.2f}
		});
	state.AddPaths("run", {
		{"walk", [this]() -> bool { return owner->GetSpeed() < 6.f; }, 0.2f},
		{"jump", [this]() -> bool { return owner->IsFalling(); }, 0.0f},
		});
	state.AddPaths("jump", {
		{"idle", [this]() -> bool { return !owner->IsFalling() && owner->GetSpeed() <= 1.f; }, 0.0f},
		{"walk", [this]() -> bool { return !owner->IsFalling() && owner->GetSpeed() <= 6.f; }, 0.0f},
		{"run", [this]() -> bool { return !owner->IsFalling(); }, 0.0f},
		});
}

HumanAnimator::~HumanAnimator()
{
	for (int i = 0; i < 3; ++i) {
		footsteps[i]->release();
	}
}

void BirdAnimator::PlayFlaps()
{
	auto Fmod = SoundManager::Get()->GetSystem();
	FMOD::Channel* c;
	Fmod->playSound(flaps[rand() % 3], nullptr, false, &c);
	//c->setVolume(0.5f);
}

BirdAnimator::BirdAnimator(ModelAnimator* anima, Character* c) : owner(c) {
	auto Fmod = SoundManager::Get()->GetSystem();

	FMOD_RESULT res = Fmod->createStream("Resources/Sounds/flop01.Wav", FMOD_DEFAULT, nullptr, &flaps[0]);
	SoundManager::Get()->ErrorCheck(res);
	res = Fmod->createStream("Resources/Sounds/flop02.Wav", FMOD_DEFAULT, nullptr, &flaps[1]);
	SoundManager::Get()->ErrorCheck(res);
	res = Fmod->createStream("Resources/Sounds/flop03.Wav", FMOD_DEFAULT, nullptr, &flaps[2]);
	SoundManager::Get()->ErrorCheck(res);

	dive = &anima->GetClip(3);
	flap = &anima->GetClip(1);
	flap.AddNotify(37, std::bind(&BirdAnimator::PlayFlaps, this));
	turnL = &anima->GetClip(0);
	turnR = &anima->GetClip(2);

	state.AddState("flap", [this](float delta, BoneArray bones) { flap.Update(delta); flap.MakeTransforms(bones); });
	state.AddState("dive", [this](float delta, BoneArray bones) { dive.Update(delta); dive.MakeTransforms(bones); });
	state.AddState("turnL", [this](float delta, BoneArray bones) { turnL.Update(delta); turnL.MakeTransforms(bones); });
	state.AddState("turnR", [this](float delta, BoneArray bones) { turnR.Update(delta); turnR.MakeTransforms(bones); });

	state.AddPaths("flap", {
		{"dive", [this] { return owner->m_pbirdHolder->GetTransform()->GetForward().y < -0.2f; }, .2f},
		{"turnL", [this] { return owner->GetDirection().x < -0.1f; }, .2f},
		{"turnR", [this] { return owner->GetDirection().x > 0.1f; }, .2f},
		});
	state.AddPaths("dive", {
		{"flap", [this] { return owner->m_pbirdHolder->GetTransform()->GetForward().y > -0.2f; }, .2f},
		});
	state.AddPaths("turnL", {
		{"flap", [this] { return owner->GetDirection().x > -0.1f; }, .2f},
		});
	state.AddPaths("turnR", {
		{"flap", [this] { return owner->GetDirection().x < 0.1f; }, .2f},
		});
}

BirdAnimator::~BirdAnimator()
{
	for (int i = 0; i < 3; ++i) {
		flaps[i]->release();
	}
}
