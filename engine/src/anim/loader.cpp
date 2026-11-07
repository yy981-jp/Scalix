#include <anim/loader.h>

#include <core/avatar.h>
#include <unordered_map>


std::unordered_map<StrHs,AnimRtFmt> loadAnim(const std::string& path, const Avatar& avatar) {
	std::unordered_map<StrHs,AnimRtFmt> anims;
	json j = readJson(path);

	// check json
	bool invalid = false;
	try {
		if (j.at("version") != 1) invalid = true;
		if (!j.contains("body")) invalid = true;
	} catch (...) {
		invalid = true;
	}

	if (invalid) throw std::runtime_error(".sxa is invalid");


	// ノード名から ID を引くための索引を作成する。
	std::unordered_map<StrHs,NodeId> map;
	map.reserve(avatar.model.nodes.size());
	for (const Node& node: avatar.model.nodes) {
		map[node.name] = node.id;
	}

	// トラックの対象ノードを解決し、実行時形式へ変換する。
	for (const auto& [key,value]: j["body"].items()) {
		const AnimImFmt& imf = value.get<AnimImFmt>();
		AnimRtFmt_base rf_b = imf;
		for (size_t i = 0; i < rf_b.tracks.size(); i++) {
			auto& track_r = rf_b.tracks[i];
			auto& track_i = imf.tracks[i];
			
			auto it = map.find(StrHs(track_i.target));
			if (it != map.end()) {
				track_r.target = it->second;
			} else {
				if (track_i.target.empty()) {
					track_r.target = NodeId::invalid(); // アバター全体を対象にするトラックは未対応
				} else {
					track_r.target = NodeId::invalid(); // 対応するノードが見つからない
				}
				// TODO: track_i.tarket.empty の場合の処理を考える必要があるかもしれない
				continue;
			}
		}

		AnimRtFmt rf;
		rf.fmt = std::move(rf_b);
		rf.end = rf.fmt.stopTime;

		StrHs hs = strsv().entry(key);
		anims[hs] = rf;

	}

	return anims;
}
