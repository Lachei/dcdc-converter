#pragma once

#include <iostream>

#include "static_types.h"

struct settings {
	float k_p{.1};	// constant factor for error
	float k_i{.1};	// constant factor for integrated error
	float k_d{.1};	// constant factor for derivative error
	float max_amps{40};
	float high_to_low_ratio{3}; // ration high_side_v / low_side_v

	static settings& Default() {
		static settings s{};
		return s;
	}
	/** @brief writes the settings struct as json to the static strig s */
	template<int N>
	constexpr void dump_to_json(static_string<N> &s) const {
		s.append_formatted(R"({{"k_p":{},"k_i":{},"k_d":{},"max_amps":{},"high_to_low_ratio":{}}})", 
		     k_p, k_i, k_d, max_amps, high_to_low_ratio);
	}
};

/** @brief prints formatted for monospace output, eg. usb */
std::ostream& operator<<(std::ostream &os, const settings &s) {
	os << "k_p:              " << s.k_p << '\n';
	os << "k_i:              " << s.k_i << '\n';
	os << "k_d:              " << s.k_d << '\n';
	os << "max_amps:         " << s.max_amps << '\n';
	os << "high_to_low_ratio:" << s.high_to_low_ratio << '\n';
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
	else if (key == "max_amps")
		is >> s.max_amps;
	else if (key == "high_to_low_ratio")
		is >> s.high_to_low_ratio;
	else
		is.setstate(std::ios::failbit);
	return is;
}

