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

};

CPPUNIT_TEST_SUITE_REGISTRATION(Test);

int main(int argc, const char* argv[]){
  CppUnit::Test *suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();

  CppUnit::TextUi::TestRunner runner;

  runner.addTest( suite );
  runner.setOutputter(new CppUnit::CompilerOutputter(&runner.result(), std::cerr ));

  return runner.run() ? 0 : 1;
}
