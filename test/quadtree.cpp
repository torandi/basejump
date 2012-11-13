#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "test/asserts.hpp"

#include "quadtree.hpp"
#include "aabb2d.hpp"

#include <glm/glm.hpp>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

static int sum;

class Test: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(Test);
	CPPUNIT_TEST(test_create);
	CPPUNIT_TEST(test_traverse);
  CPPUNIT_TEST_SUITE_END();

public:

	struct test_data {
		int index;
	};

	void free_tree(QuadTree * tree) {
		tree->traverse([](QuadTree * node) -> bool {
			delete (test_data*) node->data;
			return true;
		});
		delete tree;
	}

	QuadTree * create_top_down_tree(int size) {
		int side = static_cast<int>(glm::exp2(static_cast<float>(size)));
		QuadTree * tree = new QuadTree(AABB_2D(glm::vec2(0.f), glm::vec2(side)), size);

		sum = 0;

		for(int y=0; y<side; ++y) {
			for(int x=0; x<side; ++x) {
				test_data * d = new test_data();
				d->index = (y * side + x);
				sum += d->index;
				QuadTree * node = tree->child(glm::vec2(static_cast<float>(x), static_cast<float>(y)));
				node->data = (void*) d;
			}
		}
		return tree;
	}

	void debug_traverse(QuadTree * tree) {
		printf("Tree debug print\n");
		tree->traverse([](QuadTree * node) -> bool {
				printf("\t[%d] (%f, %f) -  (%f, %f): %d\n",
					node->level(),
					node->aabb.min.x, node->aabb.min.y,
					node->aabb.max.x, node->aabb.max.y,
					(node->data == nullptr) ? -1 : ((test_data*)node->data)->index
				);
				return true;
		});
	}

	void test_create() {
		static const int size = 5;
		QuadTree * tree = create_top_down_tree(size);
		CPPUNIT_ASSERT_EQUAL(size, tree->level());
		QuadTree * node = tree->child(glm::vec2(0.f));
		CPPUNIT_ASSERT(node != nullptr);
		CPPUNIT_ASSERT_EQUAL(0, node->level());
		free_tree(tree);
	}

	void test_traverse() {
		static const int size = 5;
		QuadTree * tree = create_top_down_tree(size);
		int accum = 0;
		tree->traverse([&accum](QuadTree * tree) -> bool {
			if(tree->level() == 0) accum += ((test_data*) tree->data)->index;
			return true;
		});

		CPPUNIT_ASSERT_EQUAL(sum, accum);
		free_tree(tree);
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
