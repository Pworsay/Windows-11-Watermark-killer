#ifndef FILE_HXX
#define FILE_HXX

#include "../include.hxx"

namespace util {
	inline auto load_file (const std::string& path) -> std::vector <unsigned __int8> {
		auto file = std::ifstream {path, std::ios::binary};
		if (!file.is_open ()) {
			return {};
		}

		file.seekg (0, std::ios::end);
		const auto& size = file.tellg ();
		file.seekg (0, std::ios::beg);

		auto buffer = std::vector <unsigned __int8> (size);
		file.read (reinterpret_cast <char*> (buffer.data ()), size);

		return buffer;
	}

	inline auto save_file (const std::string& path, const std::vector <unsigned __int8>& buffer) -> bool {
		auto file = std::ofstream {path, std::ios::binary};
		if (!file.is_open ()) {
			return false;
		}

		file.write (reinterpret_cast <const char*> (buffer.data ()), buffer.size ());
		return true;
	}
}

#endif // !FILE_HXX