#include <anim/loader.h>

#include <core/avatar.h>
#include <unordered_map>
#include <unordered_set>
#include <fstream>


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


	// process pre
	std::unordered_map<StrHs,NodeId> map;
	map.reserve(avatar.model.nodes.size());
	std::ofstream dbg("test.txt");
	dbg << "===== node names =====";
	for (const Node& node: avatar.model.nodes) {
		map[node.name] = node.id;
		dbg << node.name.hash << "\t\t\t" << strsv().get(node.name) << "\t=\t" << node.id << "\n";
	}

	for (size_t i = 0; i < 10; i++) dbg << "\n";
	dbg << "===== node names =====";



	// dbg << "----------\n";

	// for (const auto& node: avatar.model.nodes) {
	//	 dbg << strsv().get(node.name) << "\n";
	// }

	// --- DEBUG: Track missing node analysis ---
	dbg << "===== TRACK TARGET ANALYSIS =====\n";
	std::unordered_set<std::string> missingTargets;
	std::unordered_set<std::string> foundTargets;
	
	for (const auto& [key,value]: j["body"].items()) {
		const AnimImFmt& imf = value.get<AnimImFmt>();
		for (const auto& track_i: imf.tracks) {
			auto it = map.find(StrHs(track_i.target));
			if (it == map.end()) {
				missingTargets.insert(track_i.target);
			} else {
				foundTargets.insert(track_i.target);
			}
		}
	}
	
	dbg << "Found targets: " << foundTargets.size() << "\n";
	for (const auto& t: foundTargets) {
		dbg << "  OK: " << t << "\n";
	}
	
	dbg << "\nMissing targets: " << missingTargets.size() << "\n";
	for (const auto& t: missingTargets) {
		dbg << "  MISSING: " << t << "\n";
	}
	dbg << "===== END TRACK ANALYSIS =====\n\n";
	

	for (size_t i = 0; i < 10; i++) dbg << "\n";
	dbg << "===== anim names =====\n";

	// process
	for (const auto& [key,value]: j["body"].items()) {
		const AnimImFmt& imf = value.get<AnimImFmt>();
		AnimRtFmt_base rf_b = imf;
		for (size_t i = 0; i < rf_b.tracks.size(); i++) {
			auto& track_r = rf_b.tracks[i];
			auto& track_i = imf.tracks[i];
			
			auto it = map.find(StrHs(track_i.target));
			if (it != map.end()) {
				track_r.target = it->second;
				// dbg << "D: found: i-s:" << track_i.target << " i:" << StrHs(track_i.target).hash << " r:" << track_r.target << "\n";
			} else {
				if (track_i.target.empty()) {
					track_r.target = -1; // アバター全体
				} else {
					track_r.target = -404; // エラー
				}
				// dbg << "D: notfound: i-s:" << track_i.target << " i:" << StrHs(track_i.target).hash << " r:" << track_r.target << "\n";
				// TODO: track_i.tarket.empty の場合の処理を考える必要があるかもしれない
				continue;
			}
		}

		AnimRtFmt rf;
		rf.fmt = std::move(rf_b);
		rf.end = rf.fmt.stopTime;

		StrHs hs = strsv().entry(key);
		anims[hs] = rf;

		dbg << key << "\t\t\t" << StrHs(key).hash << "\n";
	}

	return anims;
}
