#include <util/fmutil.h>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

std::vector<std::string> tokenizer(const std::string_view& input) {
	std::vector<std::string> result;
	size_t start = 0;

	auto push = [&](size_t end) {
		if (start < end) {
			std::string token(input.substr(start, end - start));

			for (char& c : token) {
				c = static_cast<char>(
					std::tolower(static_cast<unsigned char>(c))
				);
			}

			result.push_back(std::move(token));
		}
	};

	for (size_t i = 0; i < input.size(); ++i) {
		char c = input[i];

		// 記号で区切る
		if (c == '_' || c == '.' || c == '-' || c == ':') {
			push(i);
			start = i + 1;
			continue;
		}

		// 大文字で区切る（camelCase / PascalCase）
		if (std::isupper(static_cast<unsigned char>(c)) != 0 && start < i) {
			push(i);
			start = i;
		}

		// 数字と文字の境界で区切る
		bool digit = std::isdigit(static_cast<unsigned char>(c)) != 0;
		bool prevDigit = i > start &&
			std::isdigit(static_cast<unsigned char>(input[i - 1])) != 0;

		if (i > start && digit != prevDigit) {
			push(i);
			start = i;
		}
	}

	push(input.size());
	return result;
}
