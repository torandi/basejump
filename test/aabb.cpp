#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "aabb.hpp"
#include "test/asserts.hpp"

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

class Test: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(Test);
	CPPUNIT_TEST(test_addition_change_min);
	CPPUNIT_TEST(test_addition_change_max);
	CPPUNIT_TEST(test_corner_calculation);
	CPPUNIT_TEST(test_add_point_new_min);
	CPPUNIT_TEST(test_add_point_new_max);
	CPPUNIT_TEST(test_add_point_no_change);
  CPPUNIT_TEST_SUITE_END();

public:

	void test_addition_change_min() {
		AABB aabb1(glm::vec3(0.f), glm::vec3(1.f));
		AABB aabb2(glm::vec3(-1.f), glm::vec3(0.5f));
		AABB res = aabb1 + aabb2;
		CPPUNIT_ASSERT_VEC3_EQUAL(glm::vec3(-1.f), res.min, 0.01f);
		CPPUNIT_ASSERT_VEC3_EQUAL(glm::vec3(1.f), res.max, 0.01f);
	}

	void test_addition_change_max() {
		AABB aabb1(glm::vec3(0.f), glm::vec3(1.f));
		AABB aabb2(glm::vec3(0.5f), glm::vec3(1.5f));
		AABB res = aabb1 + aabb2;
		CPPUNIT_ASSERT_VEC3_EQUAL(glm::vec3(0.f), res.min, 0.01f);
		CPPUNIT_ASSERT_VEC3_EQUAL(glm::vec3(1.5f), res.max, 0.01f);
	}

	void test_corner_calculation() {
		AABB aabb(glm::vec3(-1.f), glm::vec3(1.f));
		const std::vector<glm::vec3> &corners = aabb.corners();
		CPPUNIT_ASSERT_VEC3_EQUAL_MESSAGE("c0", glm::vec3(-1.f), corners[0], 0.01f);
		CPPUNIT_ASSERT_VEC3_EQUAL_MESSAGE("c1", glm::vec3(-1.f, -1.f, 1.f), corners[1], 0.01f);
		CPPUNIT_ASSERT_VEC3_EQUAL_MESSAGE("c2", glm::vec3(1.f, -1.f, 1.f), corners[2], 0.01f);
		CPPUNIT_ASSERT_VEC3_EQUAL_MESSAGE("c3", glm::vec3(1.f, -1.f, -1.f), corners[3], 0.01f);

		CPPUNIT_ASSERT_VEC3_EQUAL_MESSAGE("c4", glm::vec3(-1.f, 1.f, -1.f), corners[4], 0.01f);
		CPPUNIT_ASSERT_VEC3_EQUAL_MESSAGE("c5", glm::vec3(-1.f, 1.f, 1.f), corners[5], 0.01f);
		CPPUNIT_ASSERT_VEC3_EQUAL_MESSAGE("c6", glm::vec3(1.f, 1.f, 1.f), corners[6], 0.01f);
		CPPUNIT_ASSERT_VEC3_EQUAL_MESSAGE("c7", glm::vec3(1.f, 1.f, -1.f), corners[7], 0.01f);
	}

	void test_add_point_new_min() {
		AABB aabb(glm::vec3(0.f), glm::vec3(1.f));
		glm::vec3 p(-1.f);
		aabb.add_point(p);
		CPPUNIT_ASSERT_VEC3_EQUAL(glm::vec3(-1.f), aabb.min, 0.01f);
		CPPUNIT_ASSERT_VEC3_EQUAL(glm::vec3(1.f), aabb.max, 0.01f);
	}

	void test_add_point_new_max() {
		AABB aabb(glm::vec3(0.f), glm::vec3(1.f));
		glm::vec3 p(2.f);
		aabb.add_point(p);
		CPPUNIT_ASSERT_VEC3_EQUAL(glm::vec3(0.f), aabb.min, 0.01f);
		CPPUNIT_ASSERT_VEC3_EQUAL(glm::vec3(2.f), aabb.max, 0.01f);
	}

	void test_add_point_no_change() {
		AABB aabb(glm::vec3(0.f), glm::vec3(1.f));
		glm::vec3 p(0.5f);
		aabb.add_point(p);
		CPPUNIT_ASSERT_VEC3_EQUAL(glm::vec3(0.f), aabb.min, 0.01f);
		CPPUNIT_ASSERT_VEC3_EQUAL(glm::vec3(1.f), aabb.max, 0.01f);
	}

};

CPPUNIT_TEST_SUITE_REGISTRATION(Test);

int main(int argc, const char* argv[]){
  CppUnit::Test *suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();

  CppUnit::TextUi::TestRunner runner;

  runner.addTest( suite );
  runner.setOutputter(new CppUnit::CompilerOutputter(&runner.result(), std::cerr ));

  return runner.run() ? 0 : 1;
}
