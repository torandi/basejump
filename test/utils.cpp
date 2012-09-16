#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "utils.hpp"
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

class Test: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(Test);
	CPPUNIT_TEST(test_screen_pos_lower);
	CPPUNIT_TEST(test_screen_pos_upper);
	CPPUNIT_TEST(test_screen_pos_center);
	CPPUNIT_TEST(test_screen_pos_box);
  CPPUNIT_TEST_SUITE_END();

public:

  void test_screen_pos_lower(){
	  const glm::vec2 tmp = screen_pos(glm::vec2(0.0f, 0.0f), glm::vec2(200, 100));
	  CPPUNIT_ASSERT_EQUAL((int)tmp.x, 0);
	  CPPUNIT_ASSERT_EQUAL((int)tmp.y, 0);
  }

	void test_screen_pos_upper(){
		const glm::vec2 tmp = screen_pos(glm::vec2(1.0f, 1.0f), glm::vec2(200, 100));
	  CPPUNIT_ASSERT_EQUAL((int)tmp.x, 200);
	  CPPUNIT_ASSERT_EQUAL((int)tmp.y, 100);
  }

	void test_screen_pos_center(){
		const glm::vec2 tmp = screen_pos(glm::vec2(0.5f, 0.5f), glm::vec2(200, 100));
	  CPPUNIT_ASSERT_EQUAL((int)tmp.x, 100);
	  CPPUNIT_ASSERT_EQUAL((int)tmp.y, 50);
  }

	void test_screen_pos_box(){
		const glm::vec2 tmp = screen_pos(glm::vec2(1,1), glm::vec2(200, 100), glm::vec2(50, 25));
	  CPPUNIT_ASSERT_EQUAL((int)tmp.x, 150);
	  CPPUNIT_ASSERT_EQUAL((int)tmp.y, 75);
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
