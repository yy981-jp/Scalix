#pragma once

#include "../core/str.h"
#include "../core/vec3.h"
#include "../core/quat.h"
#include "loader.h"

#include <unordered_map>


// constexpr float lerp(float a, float b, float t) { return std::lerp(a,b,t); }
// constexpr vec3f lerp(const FormatDef::vec3f& a, const FormatDef::vec3f& b, float t) { return a + (b - a) * t; }

struct Pose {
    vec3f pos;
    Quat rot;
    vec3f scale;
};

struct PlayingAnim {
	StrHs anim;

	float time = 0.0f;
	std::vector<int> crtKeyIdxs;
	// float speed = 1.0f;

	bool loop = false;
	// bool finished = false;
};

class AnimSystem {
    std::unordered_map<StrHs, AnimRtFmt> anims;
    std::vector<PlayingAnim> playing;

public:
    void init(const std::string& path) {
        anims = loadAnim(path);
    }

    void run(StrHs animName) {
        const AnimRtFmt& anim = anims[animName];
        PlayingAnim plAnim;
        plAnim.crtKeyIdxs.resize(anim.fmt.tracks.size());
        playing.push_back(std::move(plAnim));
    }

    void stop(StrHs animName) {
        std::erase_if(playing, [&](const PlayingAnim& a) {
            return a.anim == animName;
        });
    }

    Pose update(float dt) {
        Pose res;
        for (PlayingAnim& anim_p: playing) {
            AnimRtFmt& anim = anims[anim_p.anim];
            anim_p.time += dt;
            int trackNumber = 0;
            for (const auto& track: anim.fmt.tracks) {
                auto& anim_p_crtKeyIdx = anim_p.crtKeyIdxs[trackNumber];
                while (
                    anim_p_crtKeyIdx + 1 < track.keys.size() &&
                    anim_p.time >= track.keys[anim_p_crtKeyIdx + 1].time
                ) { anim_p_crtKeyIdx++; }

                const auto& f = track.keys[anim_p_crtKeyIdx];
                const auto& n = track.keys[anim_p_crtKeyIdx + 1];

                float t = (anim_p.time - f.time) / (n.time - f.time);


                std::variant<float, vec3f, Quat, bool> value = std::visit(
                    [&](const auto& f, const auto& n) {
                        using T = std::decay_t<decltype(f)>;

                        if constexpr (std::is_same_v<T, float>) {
                            return std::lerp(f, n, t);
                        } else if constexpr (std::is_same_v<T, FormatDef::vec3f>) {
                            return lerp(f, n, t);
                        } else if constexpr (std::is_same_v<T, FormatDef::Quat>) {
                            return Quat::lerp(f, n, t);
                        }
                    },
                    f.value,
                    n.value
                );
                trackNumber++;
            }
        }
        return pos;
    }
};
