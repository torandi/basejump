#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "quadtree.hpp"
#include "logging.hpp"

QuadTree::QuadTree(AABB_2D position, int level)
	: data(nullptr)
	, aabb(position)
	, level_(level)
{
		for(int i=0; i<4; ++i) children[i] = nullptr;
}

QuadTree::~QuadTree() {
	for(int i=0; i<4; ++i) {
		if(children[i] != nullptr) delete children[i];
	}
}

void QuadTree::add_child(QuadTree * node) {
	if(aabb.contains(node->aabb.min) && aabb.contains(node->aabb.max)) {
		int index = child_index(node->aabb.middle());
		if(children[index] != nullptr) delete children[index]; //Dellocated anything in that slot
		children[index] = node;
	} else {
		Logging::fatal("[QuadTree] Trying to add a child that is larger than the parent\n");
	}
}

QuadTree * QuadTree::child(const glm::vec2 &position) {
	if(aabb.contains(position)) {
		if(level_ == 0) return this; //This is a leaf

		int index = child_index(position);
		return child(index);
	} else {
		return nullptr;
	}
}

QuadTree * QuadTree::child(int index) {
	if(children[index] == nullptr) create_child(index);
	return children[index];
}

int QuadTree::child_index(const glm::vec2 &position) {
	if(position.x < aabb.middle().x) {
		if(position.y < aabb.middle().y) {
			return 0;
		} else {
			return 2;
		}
	} else {
		if(position.y < aabb.middle().y) {
			return 1;
		} else {
			return 3;
		}
	}
}

int QuadTree::level() const {
	return level_;
}

void QuadTree::create_child(int index) {
	glm::vec2 half_size = aabb.size() / 2.f;
	glm::vec2 min = aabb.min;
	switch(index) {
		case 3:
			min.x += half_size.x;
		case 2:
			min.y += half_size.y;
			break;
		case 1:
			min.x += half_size.x;
			break;
	}
	children[index] = new QuadTree(AABB_2D(min, min+half_size), level_ - 1);
}

void QuadTree::traverse(const std::function<bool(QuadTree*)> & func) {
	if(func(this)) {
		for(int i=0; i<4; ++i) {
			if(children[i] != nullptr) children[i]->traverse(func);
		}
	}
}
