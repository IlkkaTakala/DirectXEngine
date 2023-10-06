#pragma once

using BoneArray = std::vector<XMFLOAT4X4>&;

XMFLOAT4X4 Interpolate(const XMFLOAT4X4& transformA, const XMFLOAT4X4& transformB, float blendFactor);

struct AnimationInstance
{
private:
	const AnimationClip* anim{ nullptr };
	float frametime{ 0 };
	float frame{ 0 };
	int previousframe{ 0 };
	int nextframe{ 0 };
	float blendFactor{ 0.f };
	std::vector<std::pair<float, std::function<void()>>> Callbacks;

public:
	AnimationInstance() : anim(nullptr) {}
	AnimationInstance(const AnimationClip* a) : anim(a) {}

	void SetAnimation(AnimationClip* a) {
		anim = a;
		frametime = 0.f;
		frame = 0;
		previousframe = 0;
		nextframe = 0;
	}
	void AddNotify(float time, std::function<void()> func) {
		Callbacks.emplace_back(std::pair<float, std::function<void()>>{ time, func });
	}

	void Update(float delta);

	void MakeTransforms(BoneArray bones) const;
};

class AnimationBlendSpace1D
{
	std::vector<std::pair<float, AnimationInstance>> anims;

public:

	void AddKey(float time, const AnimationClip* animation) {
		anims.emplace_back(time, animation);
	}

	void Evaluate(float delta, BoneArray bones, float axisValue);
};

class AnimationStateMachine
{
	typedef std::function<bool(void)> PathFunc;
public:
	AnimationStateMachine() {
		currentState = 0;
		interpolating = false;
		interpTime = 0.f;
		currentInterpTime = 0.f;
		maxState = -1;
	}

	void Evaluate(float delta, BoneArray bones) {
		if (delta <= 0.0001f && delta > -0.0001f) return;
		if (states.empty()) return;
		if (current.size() != bones.size()) { current.resize(bones.size()); old.resize(bones.size()); }

		states[currentState](delta, current);

		if (interpolating) {
			currentInterpTime += delta;
			if (currentInterpTime > interpTime) { interpolating = false; }
			else {
				float d = currentInterpTime / interpTime;
				for (int i = 0; i < bones.size(); i++)
					bones[i] = Interpolate(old[i], current[i], d);
			}
		}
		else {
			for (int i = 0; i < bones.size(); i++)
				bones[i] = current[i];
		}

		if (paths.size() > currentState)
			for (const auto& [target, func, time] : paths[currentState]) {
				if (func()) {
					std::copy(bones.begin(), bones.end(), old.begin());
					currentState = target;
					interpolating = true;
					interpTime = time;
					currentInterpTime = 0.f;
					break;
				}
			}
	}

	void AddState(const std::string& name, std::function<void(float, BoneArray)> func) {
		if (ids.find(name) == ids.end()) {
			ids.emplace(name, ++maxState);

			states.emplace_back(func);
		}
	}

	void AddPaths(const std::string& name, const std::vector<std::tuple<std::string, std::function<bool(void)>, float>>& statepaths) {
		if (paths.size() != states.size()) paths.resize(states.size());
		if (ids.find(name) != ids.end()) {
			auto& it = paths[ids[name]];
			for (auto& [pname, func, time] : statepaths)
				it.emplace_back(ids[pname], func, time);
		}
	}

private:
	std::vector<XMFLOAT4X4> old;
	std::vector<XMFLOAT4X4> current;

	std::unordered_map<std::string, int> ids;

	std::vector<std::function<void(float, BoneArray)>> states;
	std::vector<std::vector<std::tuple<int, std::function<bool(void)>, float>>> paths;

	int maxState;

	int currentState;
	bool interpolating;
	float interpTime;
	float currentInterpTime;
};

class ModelAnimator final
{
public:
	ModelAnimator(MeshFilter* pMeshFilter);
	~ModelAnimator() = default;
	ModelAnimator(const ModelAnimator& other) = delete;
	ModelAnimator(ModelAnimator&& other) noexcept = delete;
	ModelAnimator& operator=(const ModelAnimator& other) = delete;
	ModelAnimator& operator=(ModelAnimator&& other) noexcept = delete;

	void SetAnimation(const std::wstring& clipName);
	void SetAnimation(UINT clipNumber);
	void SetAnimation(const AnimationClip& clip);
	void Update(const SceneContext& sceneContext);
	void Reset(bool pause = true);
	void Play() { m_IsPlaying = true; }
	void Pause() { m_IsPlaying = false; }
	void SetPlayReversed(bool reverse) { m_Reversed = reverse; }
	void SetAnimationSpeed(float speedPercentage) { m_AnimationSpeed = speedPercentage; }

	bool IsPlaying() const { return m_IsPlaying; }
	bool IsReversed() const { return m_Reversed; }
	float GetAnimationSpeed() const { return m_AnimationSpeed; }
	const AnimationClip& GetClip(int clipId) { ASSERT_IF_(clipId >= m_pMeshFilter->m_AnimationClips.size())return m_pMeshFilter->m_AnimationClips[clipId]; }
	UINT GetClipCount() const { return UINT(m_pMeshFilter->m_AnimationClips.size()); }
	const std::wstring& GetClipName() const { ASSERT_IF_(!m_ClipSet) return m_CurrentClip.name; }
	const std::vector<XMFLOAT4X4>& GetBoneTransforms() const { return m_Transforms; }

	std::function<void(float,BoneArray)> Updator;
private:

	AnimationClip m_CurrentClip{};
	MeshFilter* m_pMeshFilter{};
	std::vector<XMFLOAT4X4> m_Transforms{};
	bool m_IsPlaying{}, m_Reversed{}, m_ClipSet{};
	float m_TickCount{}, m_AnimationSpeed{1.f};
};

