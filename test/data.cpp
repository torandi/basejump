#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "data.hpp"
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

class Test: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(Test);
	CPPUNIT_TEST(test_initial_path);
	CPPUNIT_TEST(test_remove_path);
	CPPUNIT_TEST(test_add_empty_path);
	CPPUNIT_TEST(test_trailing_slash);
	CPPUNIT_TEST(test_dot_slash);
  CPPUNIT_TEST_SUITE_END();

public:

	void tearDown(){
	  Data::remove_search_paths();
	}

  void test_initial_path(){
	  CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(0), Data::get_search_path().size());
  }

  void test_remove_path(){
	  CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(0), Data::get_search_path().size());
	  Data::add_search_path("");
	  CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), Data::get_search_path().size());
	  Data::remove_search_paths();
	  CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(0), Data::get_search_path().size());
  }

	void test_add_empty_path(){
	  Data::add_search_path("");
	  const std::vector<std::string> path = Data::get_search_path();
	  CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), path.size());
	  CPPUNIT_ASSERT_EQUAL(std::string(""), path[0]);
  }

	void test_trailing_slash(){
	  Data::add_search_path("path1");
	  Data::add_search_path("path2/");
	  const std::vector<std::string> path = Data::get_search_path();
	  CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(2), path.size());
	  CPPUNIT_ASSERT_EQUAL(std::string("path1/"), path[0]);
	  CPPUNIT_ASSERT_EQUAL(std::string("path2/"), path[1]);
  }

	void test_dot_slash(){
	  Data::add_search_path("");
	  Data::add_search_path(".");
	  Data::add_search_path("./");
	  const std::vector<std::string> path = Data::get_search_path();
	  CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(3), path.size());
	  CPPUNIT_ASSERT_EQUAL(std::string(""), path[0]);
	  CPPUNIT_ASSERT_EQUAL(std::string(""), path[1]);
	  CPPUNIT_ASSERT_EQUAL(std::string(""), path[2]);
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION(Test);

int main(int argc, const char* argv[]){
  CppUnit::Test *suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();
  CppUnit::TextUi::TestRunner runner;

  runner.addTest(suite);
  runner.setOutputter(new CppUnit::CompilerOutputter(&runner.result(), std::cerr));

  return runner.run() ? 0 : 1;
}
