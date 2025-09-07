#pragma once

#include <iostream>

#include "static_types.h"

struct settings {
	float k_p{.1};	// constant factor for error
	float k_i{.1};	// constant factor for integrated error
	float k_d{.1};	// constant factor for derivative error
	float max_amp{};

	static settings& Default() {
		static settings s{};
		return s;
	}
	/** @brief writes the settings struct as json to the static strig s */
	template<int N>
	constexpr void dump_to_json(static_string<N> &s) const {
		s.append_formatted(R"({{"k_p":{},"k_i":{},"k_d"}})", k_p, k_i, k_d);
	}
};

/** @brief prints formatted for monospace output, eg. usb */
std::ostream& operator<<(std::ostream &os, const settings &s) {
	os << "k_p:   " << s.k_p << '\n';
	os << "k_i:   " << s.k_i << '\n';
	os << "k_d:   " << s.k_d << '\n';
	return os;
}

/** @brief parses a single key, value pair from the istream */
std::istream& operator>>(std::istream &is, settings &s) {
	std::string key;
	is >> key;
	if (key == "k_p")
		is >> s.k_p;
	else if (key == "k_i")
		is >> s.k_i;
	else if (key == "k_d")
		is >> s.k_d;
	else
		is.fail();
	return is;
}

