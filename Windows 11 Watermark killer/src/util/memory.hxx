#ifndef MEMORY_HXX
#define MEMORY_HXX

#include "../include.hxx"

namespace util {
	template <typename T>
	concept valid_address_type = std::is_pointer_v<T> || std::is_lvalue_reference_v<T> ||
		std::is_rvalue_reference_v<T> || std::is_same_v<T, std::uintptr_t> ||
		std::is_arithmetic_v<T>;

	struct address_t {
		explicit address_t (std::uintptr_t address) : m_address (address) {
		}
		explicit address_t (std::uintptr_t* address) : m_address (reinterpret_cast<std::uintptr_t>(address)) {
		}
		address_t (const void* address = nullptr) : m_address (reinterpret_cast<std::uintptr_t>(address)) {
		}

		template <typename T>
		auto as () const -> T {
			static_assert(valid_address_type<T>, "Invalid type");

			if constexpr (std::is_pointer_v<T>) {
				return reinterpret_cast<T>(m_address);
			} else if constexpr (std::is_lvalue_reference_v<T>) {
				return *reinterpret_cast<std::remove_reference_t<T>*>(m_address);
			} else if constexpr (std::is_rvalue_reference_v<T>) {
				return std::move (*reinterpret_cast<std::remove_reference_t<T>*>(m_address));
			} else if constexpr (std::is_same_v<T, std::uintptr_t>) {
				return m_address;
			} else if constexpr (std::is_arithmetic_v<T>) {
				return static_cast<T>(m_address);
			}
		}

		auto operator+ (std::size_t offset) const -> address_t {
			return address_t (m_address + offset);
		}
	private:
		std::uintptr_t m_address;
	};

	struct memory_section_t {
		explicit memory_section_t (address_t base, std::size_t size, std::string name) : m_base (base), m_size (size), m_name (std::move (name)) {
		}
		explicit memory_section_t (address_t base, std::size_t size) : m_base (base), m_size (size), m_name ("unknown") {
		}

		auto base () const -> address_t {
			return m_base;
		}

		auto size () const -> std::size_t {
			return m_size;
		}

		auto name () const -> std::string {
			return m_name;
		}

		auto begin () const -> address_t {
			return m_base;
		}

		auto end () const -> address_t {
			return m_base + m_size;
		}

		auto contains (address_t address) const -> bool {
			return address.as<std::uintptr_t> () >= m_base.as<std::uintptr_t> () && address.as<std::uintptr_t> () < end ().as < uintptr_t > ();
		}
	private:
		address_t m_base;
		std::size_t m_size;
		std::string m_name;
	};

	inline auto compare_masked_bytes (const std::uint8_t* data, const std::uint8_t* pattern, const std::string_view mask) -> bool {
		for (auto i = 0u; i < mask.size (); ++i) {
			if (mask [i] == 'x' && data [i] != pattern [i]) {
				return false;
			}
		}

		return true;
	}

	inline auto search_pattern (const memory_section_t& section, const char* pattern, const std::string_view mask) -> std::optional <address_t> {
		const auto data = section.begin ().as <std::uint8_t*> ();
		const auto size = section.size () - mask.size () - 1;

		const auto pattern_bytes = reinterpret_cast <const std::uint8_t*> (pattern);
		const auto mask_bytes = mask.data ();
		const auto first_byte = pattern_bytes [0];

		for (auto i = 0u; i < size; ++i) {
			if (data [i] != first_byte) {
				continue;
			}

			if (compare_masked_bytes (data + i, pattern_bytes, mask)) {
				return section.begin () + i;
			}
		}

		return std::nullopt;
	}
}

#endif // !MEMORY_HXX