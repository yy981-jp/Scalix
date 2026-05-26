#include "AnimSystem.h"

#include "../core/avater.h"

#include <iostream>


void AnimSystem::init(const std::string& path, const Avater& avater) {
	anims = loadAnim(path, avater);
}

void AnimSystem::run(StrHs animName) {
	const AnimRtFmt& anim = anims[animName];

	PlayingAnim plAnim;
	plAnim.anim = animName;
	plAnim.crtKeyIdxs.resize(anim.fmt.tracks.size());

	playing.push_back(std::move(plAnim));
}

void AnimSystem::stop(StrHs animName) {
	std::erase_if(playing, [&](const PlayingAnim& a) {
		return a.anim == animName;
	});
}

void AnimSystem::tick(Avater& avater, float dt) {
	blendBuffer.clear();
	blendBufferSR.clear();

	blend(dt);
	apply(avater);
}


void AnimSystem::apply(Avater& avater) {
	size_t blendIdx = 0;

	for (size_t playIdx = 0; playIdx < playing.size();) {
		const PlayingAnim& anim_p = playing[playIdx];
		AnimRtFmt& anim = anims[anim_p.anim];

		bool shouldRemove = false;

		for (const auto& track : anim.fmt.tracks) {
			switch (track.proc) {
				using enum FormatDef::Proc;

				case active: {
					Node& target = avater.model.nodes[track.target];
					target.visible = std::get<float>(blendBuffer[blendIdx]);
				} break;
			}

			shouldRemove = blendBufferSR[blendIdx];
			blendIdx++;
		}

		if (shouldRemove) {
			playing[playIdx] = std::move(playing.back());
			playing.pop_back();
		} else {
			playIdx++;
		}
	}
}


void AnimSystem::blend(float dt) {
	for (size_t playIdx = 0; playIdx < playing.size(); playIdx++) {
		PlayingAnim& anim_p = playing[playIdx];
		AnimRtFmt& anim = anims[anim_p.anim];

		anim_p.time += dt;

		bool shouldRemove = true;
		int trackNumber = 0;

		for (const auto& track : anim.fmt.tracks) {
			auto& crtKeyIdx = anim_p.crtKeyIdxs[trackNumber];

			while (
				crtKeyIdx + 1 < track.keys.size() &&
				anim_p.time >= track.keys[crtKeyIdx + 1].time
			) {
				crtKeyIdx++;
			}

			Value value;

			if (crtKeyIdx + 1 < track.keys.size()) {
				shouldRemove = false;

				const auto& f = track.keys[crtKeyIdx];
				const auto& n = track.keys[crtKeyIdx + 1];

				float t = (anim_p.time - f.time) / (n.time - f.time);

				value = std::visit(
					[&](const auto& f, const auto& n) -> Value {
						using T1 = std::decay_t<decltype(f)>;
						using T2 = std::decay_t<decltype(n)>;

						if constexpr (std::is_same_v<T1, T2>) {
							if constexpr (std::is_same_v<T1, float>) {
								return std::lerp(f, n, t);
							} else if constexpr (std::is_same_v<T1, vec3f>) {
								return lerp(f, n, t);
							} else if constexpr (std::is_same_v<T1, Quat>) {
								return lerp(f, n, t);
							} else if constexpr (std::is_same_v<T1, bool>) {
								return t < 0.5f ? f : n;
							}
						}

						return f;
					},
					f.value,
					n.value
				);

			}/* else {
				value = track.keys.back().value;
			}*/

			blendBuffer.push_back(std::move(value));
			blendBufferSR.push_back(shouldRemove);

			trackNumber++;
		}
	}
}
