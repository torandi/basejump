#ifndef ASSERTS_HPP
#define ASSERTS_HPP

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestAssert.h>
#include <glm/glm.hpp>
#include <cstring>

static void check_vec2(const glm::vec2 &expected, const glm::vec3 &actual, float delta, const char * message, CppUnit::SourceLine sourceline) {
	char * buffer = (char*) malloc(strlen(message) + 3);

	sprintf(buffer, "%s.x", message);
	CppUnit::assertDoubleEquals((double)expected.x,(double) actual.x, (double)delta, sourceline, buffer);
	sprintf(buffer, "%s.y", message);
	CppUnit::assertDoubleEquals((double)expected.y,(double) actual.y, (double)delta,sourceline,  buffer);

	free(buffer);
}

static void check_vec3(const glm::vec3 &expected, const glm::vec3 &actual, float delta, const char * message, CppUnit::SourceLine sourceline) {
	char * buffer = (char*) malloc(strlen(message) + 3);

	sprintf(buffer, "%s.x", message);
	CppUnit::assertDoubleEquals((double)expected.x,(double) actual.x, (double)delta, sourceline, buffer);
	sprintf(buffer, "%s.y", message);
	CppUnit::assertDoubleEquals((double)expected.y,(double) actual.y, (double)delta,sourceline,  buffer);
	sprintf(buffer, "%s.z", message);
	CppUnit::assertDoubleEquals((double)expected.z,(double) actual.z, (double)delta, sourceline, buffer);

	free(buffer);
}

#define CPPUNIT_ASSERT_VEC2_EQUAL(expected, actual, delta) check_vec2(expected, actual, delta,"", CPPUNIT_SOURCELINE())
#define CPPUNIT_ASSERT_VEC2_EQUAL_MESSAGE(message, expected, actual, delta) check_vec2(expected, actual, delta, message, CPPUNIT_SOURCELINE())

#define CPPUNIT_ASSERT_VEC3_EQUAL(expected, actual, delta) check_vec3(expected, actual, delta,"", CPPUNIT_SOURCELINE())
#define CPPUNIT_ASSERT_VEC3_EQUAL_MESSAGE(message, expected, actual, delta) check_vec3(expected, actual, delta, message, CPPUNIT_SOURCELINE())

#endif
