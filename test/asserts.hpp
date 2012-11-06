#ifndef ASSERTS_HPP
#define ASSERTS_HPP

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestAssert.h>
#include <glm/glm.hpp>

static void check_vec3(const glm::vec3 &expected, const glm::vec3 &actual, float delta, CppUnit::SourceLine sourceline) {
	CppUnit::assertDoubleEquals((double)expected.x,(double) actual.x, (double)delta, sourceline, "x");
	CppUnit::assertDoubleEquals((double)expected.y,(double) actual.y, (double)delta,sourceline,  "y");
	CppUnit::assertDoubleEquals((double)expected.z,(double) actual.z, (double)delta, sourceline, "z");
}

#define CPPUNIT_ASSERT_VEC3_EQUAL(expected, actual, delta) check_vec3(expected, actual, delta, CPPUNIT_SOURCELINE())

#endif
