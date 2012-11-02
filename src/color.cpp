#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "color.hpp"
#include "logging.hpp"

const Color Color::black       ( 0, 0, 0, 1);
const Color Color::blue        ( 0, 0, 1, 1);
const Color Color::green       ( 0, 1, 0, 1);
const Color Color::cyan        ( 0, 1, 1, 1);
const Color Color::red         ( 1, 0, 0, 1);
const Color Color::magenta     ( 1, 0, 1, 1);
const Color Color::yellow      ( 1, 1, 0, 1);
const Color Color::white       ( 1, 1, 1, 1);
const Color Color::transparent ( 0, 0, 0, 0);

Color Color::from_hex(const char* str){
	int hr, hg, hb, ha;
	int n = sscanf(str, "%02x%02x%02x%02x", &hr, &hg, &hb, &ha);
	if ( n == 3 ){
		return Color(static_cast<float>(hr) / 255.0f, static_cast<float>(hg) / 255.0f, static_cast<float>(hb) / 255.0f, 1.0f);
	} else if ( n == 4 ){
		return Color(static_cast<float>(hr) / 255.0f, static_cast<float>(hg) / 255.0f, static_cast<float>(hb) / 255.0f, static_cast<float>(ha) / 255.0f);
	} else {
		Logging::warning("Bad color value \"%s\", defaulting to black\n", str);
		return Color::black;
	}
}
