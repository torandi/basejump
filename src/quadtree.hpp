#ifndef QUADTREE_HPP

#include <functional>
#include <glm/glm.hpp>

#include "aabb2d.hpp"

/*
 * This is actually just a node in a quad tree, but
 * the first node is the root of the tree
 *
 * NOTE: Apparently this is not a real quad tree (more like a 2d-tree), but it works for the current purpose
 */

class QuadTree {
	public:
		QuadTree(AABB_2D position, int level = 0);
		~QuadTree();

		void * data;

		/*
		 * Traverse the tree, executing given function at each node.
		 * Return true from function to continue down the branch
		 */
		void traverse(const std::function<bool(QuadTree*)> & func);

		/*
		 * Finds or allocates child that contain the given position, 
		 * if the position is in this quad tree a quadtree ptr will be returned
		 * if the position is not in this quad tree, a nullptr will be returned
		 */
		QuadTree * child(const glm::vec2 &position);

		/*
		 * Same as above, but from specified index
		 */
		QuadTree * child(int index);

		/*
		 * Add the node to this quad tree. 
		 * Will abort (log fatal) if the aabb is not contain within this nodes aabb,
		 * but no other sanity check will be made (such as that it is exactly 1/4 of this aabb),
		 * but if that is not the case, things will be weird.
		 *
		 * If the slot that the child should occupy is already taken the old child will be deallocaded.
		 */
		void add_child(QuadTree * node);

		const AABB_2D aabb;

		int level() const;

		/*
		 * This grows the tree upwards. That is: create a new quad tree that is twice the size of
		 * this node, and then add this node to the new node. 
		 *
		 * returns the new node
		 */
		QuadTree * grow();

	private:
		/**
		 * 0 | 1
		 * -----
		 * 2 | 3
		 */
		QuadTree * children[4]; 

		int level_; //Level 0 is leaf, all above are non-teriminal nodes

		/**
		 * Calculates which quadrant the position lies in.
		 * Assumes that the position is in this nodes aabb
		 */
		int child_index(const glm::vec2 &position);

		void create_child(int index);

};

#endif
