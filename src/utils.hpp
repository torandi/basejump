#ifndef UTILS_H
#define UTILS_H

#include <glm/glm.hpp>
#include <string>
#include <functional>
#include <glm/glm.hpp>

/**
 * Get the current in-engine time.
 */
float get_time();

/**
 * Read a monotonic clock with µs-precision.
 */
unsigned long util_utime();

/**
 * Sleep µs.
 */
void util_usleep(unsigned long wait);

int checkForGLErrors( const char *s );

float radians_to_degrees(double rad);

void print_mat4(const glm::mat4 &m);

inline float frand(float lower = 0.0f, float upper = RAND_MAX) {
	const float pivot = upper - lower;
	const float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
	return lower + r * pivot;
}

/**
 * Like atof but float.
 */
inline float atoff(const char* str){
	return static_cast<float>(atof(str));
}

/**
 * Reposition a position onto screen so [0,0] -> [0,0] and [1,1] -> [w-x, h-y].
 * E.g. [0.5, 0.5] will center box.
 *
 * @param v Point in [0,0] - [1,1].
 * @param outer Size of the boundary.
 * @param inner Size of a box which will be fit into the boundary.
 */
glm::vec2 screen_pos(const glm::vec2& v, const glm::vec2& outer, const glm::vec2& inner = glm::vec2(0,0));

/**
 * Parse timetable.
 * Calls func for each valid line.
 * @return 0 if successful and errno on errors.
 */
int timetable_parse(const std::string& filename, std::function<void(const std::string&, float, float)> func);

#endif
