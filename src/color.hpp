#ifndef COLOR_H
#define COLOR_H

#include <cstdio>
#include <glm/glm.hpp>

class Color {
public:
	Color()
		: r(0.0f)
		, g(0.0f)
		, b(0.0f)
		, a(1.0f) {

	}
	Color(const Color& color)
		: r(color.r)
		, g(color.g)
		, b(color.b)
		, a(color.a){

	}
	Color(const glm::vec3& color)
		: r(color.r)
		, g(color.g)
		, b(color.b)
		, a(1.0f){

	}
	Color(const glm::vec4& color)
		: r(color.r)
		, g(color.g)
		, b(color.b)
		, a(color.a){

	}
	Color(float r, float g, float b, float a)
		: r(r)
		, g(g)
		, b(b)
		, a(a){

	}

	~Color(){}

	static Color rgb(float r, float g, float b){ return Color(r, g, b, 1.0f); }
	static Color rgba(float r, float g, float b, float a){ return Color(r,g,b,a); }
	static Color from_hex(const char* str);

	static const Color black;
	static const Color blue;
	static const Color green;
	static const Color cyan;
	static const Color red;
	static const Color magenta;
	static const Color yellow;
	static const Color white;
	static const Color transparent;

	unsigned int to_32_bit(){
		unsigned int v = 0;
		static const int shift[4] = { 24, 16, 8, 0 };

		for ( int i = 0; i < 4; i++ ){
			v += static_cast<unsigned int>(value[i] / 1.0f * 0xFF) << shift[i];
		}

		return v;
	}

	const char* to_hex() const {
		static char buffer[9]; /* #RRGGBBAA + null-terminator */
		snprintf(buffer, sizeof(buffer), "#%02x%02x%02x%02x",
		         static_cast<unsigned int>(r / 1.0f * 0xFF) & 0xFF,
		         static_cast<unsigned int>(g / 1.0f * 0xFF) & 0xFF,
		         static_cast<unsigned int>(b / 1.0f * 0xFF) & 0xFF,
		         static_cast<unsigned int>(a / 1.0f * 0xFF) & 0xFF);
		return buffer;
	}

	glm::vec3 to_vec3() const {
		return glm::vec3(r,g,b);
	}

	glm::vec4 to_vec4() const {
		return glm::vec4(r,g,b,a);
	}

	static Color mix(const Color& c1, const Color& c2, float s){
		return Color(
			c1.r + ( c2.r - c1.r ) * s,
			c1.g + ( c2.g - c1.g ) * s,
			c1.b + ( c2.b - c1.b ) * s,
			c1.a + ( c2.a - c1.a ) * s
			);
	}

	union {
		struct {
			float r;
			float g;
			float b;
			float a;
		};
		float value[4];
	};

};

#endif // COLOR_H
