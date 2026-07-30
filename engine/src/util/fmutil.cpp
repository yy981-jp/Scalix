#include <util/fmutil.h>
#include <string>


std::vector<std::string> tokenizer(const std::string_view& input) {
	std::vector<std::string> result;
	std::string current;

	auto push = [&]() {
		if (!current.empty()) {
			result.push_back(current);
			current.clear();
		}
	};

	for (size_t i = 0; i < input.size(); ++i) {
		char c = input[i];

		// 記号で区切る
		if (c == '_' || c == '.' || c == '-' || c == ':') {
			push();
			continue;
		}

		// 大文字で区切る（camelCase / PascalCase）
		if (std::isupper(c) && !current.empty()) {
			push();
		}

		// 数字と文字の境界で区切る（例: "support1" -> "support", "1"）
		if (!current.empty() && std::isdigit(c) != 0 && std::isdigit(current.back()) == 0) {
			push();
		}
		if (!current.empty() && std::isdigit(c) == 0 && std::isdigit(current.back()) != 0) {
			push();
		}

		current += std::tolower(c);
	}

	push();
	return result;
}
