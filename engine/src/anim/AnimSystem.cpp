#include "AnimSystem.h"


using Value = std::variant<float, vec3f, Quat, bool>;

void AnimSystem::init(const std::string& path) {
    anims = loadAnim(path);
}

void AnimSystem::run(StrHs animName) {
    const AnimRtFmt& anim = anims[animName];
    PlayingAnim plAnim;
    plAnim.crtKeyIdxs.resize(anim.fmt.tracks.size());
    playing.push_back(std::move(plAnim));
}

void AnimSystem::stop(StrHs animName) {
    std::erase_if(playing, [&](const PlayingAnim& a) {
        return a.anim == animName;
    });
}

void AnimSystem::update(Avater& avater, float dt) {
    for (size_t i = 0; i < playing.size(); ) {
        PlayingAnim& anim_p = playing[i];
        AnimRtFmt& anim = anims[anim_p.anim];
        anim_p.time += dt;
        int trackNumber = 0;
        for (const auto& track: anim.fmt.tracks) {
            auto& anim_p_crtKeyIdx = anim_p.crtKeyIdxs[trackNumber];
            while (
                anim_p.time >= track.keys[anim_p_crtKeyIdx + 1].time
            ) { anim_p_crtKeyIdx++; }

            if (anim_p_crtKeyIdx + 1 >= track.keys.size()) {
                // swap&pop
                playing[i] = std::move(playing.back());
                playing.pop_back();
                // TODO: もしかしてここcontinue必要だったりする?
            } else i++;

            const auto& f = track.keys[anim_p_crtKeyIdx];
            const auto& n = track.keys[anim_p_crtKeyIdx + 1];

            float t = (anim_p.time - f.time) / (n.time - f.time);


            Value value = std::visit(
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

            trackNumber++;
        }
    }
}
