#pragma once

#include <iostream>

#include "static_types.h"

struct settings {
	float k_p{-.0004};	// constant factor for error
	float k_i{-.003};	// constant factor for integrated error
	float k_d{.0};	// constant factor for derivative error
	float max_amps{40};
	float low_to_high_ratio{1./3.}; // ration low_side_v / high_side_v
	float bat_max_v{170};
	float bat_min_v{130};

	static settings& Default() {
		static settings s{};
		return s;
	}
	/** @brief writes the settings struct as json to the static strig s */
	template<int N>
	constexpr void dump_to_json(static_string<N> &s) const {
		s.append_formatted(R"({{"k_p":{},"k_i":{},"k_d":{},"max_amps":{},"low_to_high_ratio":{}}})", 
		     k_p, k_i, k_d, max_amps, low_to_high_ratio);
	}
};

/** @brief prints formatted for monospace output, eg. usb */
std::ostream& operator<<(std::ostream &os, const settings &s) {
	os << "k_p:              " << s.k_p << '\n';
	os << "k_i:              " << s.k_i << '\n';
	os << "k_d:              " << s.k_d << '\n';
	os << "max_amps:         " << s.max_amps << '\n';
	os << "low_to_high_ratio:" << s.low_to_high_ratio << '\n';
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
	else if (key == "low_to_high_ratio")
		is >> s.low_to_high_ratio;
	else
		is.setstate(std::ios::failbit);
	return is;
}

