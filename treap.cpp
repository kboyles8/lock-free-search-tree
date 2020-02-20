/**
 * @file treap.cpp
 * 
 * An immutable treap that stores integers. The immutable operations are thread safe.
 * Algorithms based on pseudo-code from https://algorithmtutor.com/Data-Structures/Tree/Treaps/
 * 
 */

#include "treap.h"

Treap::Treap() {};

/**
 * Creates a new Treap as a copy of another treap
 * 
 * @param other
 * The treap to copy
 */
Treap::Treap(const Treap &other) {
    // Copy nodes from other to self
    copy(begin(other.nodes), end(other.nodes), begin(nodes));

    size = other.size;
    root = other.root;
}

/**
 * Moves a node from one index to another
 * 
 * @param srcIndex
 * The node index to move
 * 
 * @param dstIndex
 * The new node index to be replaced
 */
void Treap::moveNode(int srcIndex, int dstIndex) {
    if (srcIndex == dstIndex) {
        return;
    }

    TreapIndex leftIndex = nodes[srcIndex].left;
    TreapIndex rightIndex = nodes[srcIndex].right;
    TreapIndex parentIndex = nodes[srcIndex].parent;

    // Transfer the node over
    nodes[dstIndex] = nodes[srcIndex];

    // Fix children pointers
    if (leftIndex != NullNode) {
        nodes[leftIndex].parent = dstIndex;
    }
    if (rightIndex != NullNode) {
        nodes[rightIndex].parent = dstIndex;
    }

    // Fix parent
    if (parentIndex == NullNode) {
        // Update the root to reflect the moved node
        root = dstIndex;
    }
    else {
        // Determine if the source node was the right or left child before being moved
        if (nodes[parentIndex].left == srcIndex) {
            nodes[parentIndex].left = dstIndex;
        }
        else {
            nodes[parentIndex].right = dstIndex;
        }
    }
}

/**
 * Creates a new node and initializes it
 * 
 * @param val
 * The value of the node
 * 
 * @return TreapIndex
 * The index of the created node
 */
TreapIndex Treap::createNewNode(int val) {
    TreapIndex newNodeIndex = size++;
    Treap::TreapNode *newNode = &nodes[newNodeIndex];
    newNode->val = val;
    newNode->weight = weightDist(randEngine);
    newNode->parent = NullNode;
    newNode->left = NullNode;
    newNode->right = NullNode;

    return newNodeIndex;
}

/**
 * @brief
 * Performs a right rotation on the target index
 * 
 * @param index
 * The index to perform the rotation on
 */
void Treap::rightRotate(TreapIndex index) {
    TreapIndex parentIndex = nodes[index].parent;
    TreapIndex leftIndex = nodes[index].left;
    TreapIndex leftRightIndex = nodes[leftIndex].right;

    // Move the target down to the right
    nodes[index].parent = leftIndex;
    nodes[leftIndex].right = index;

    // Hook the left node up to the target's old parent (or the root)
    nodes[leftIndex].parent = parentIndex;
    if (parentIndex == NullNode) {
        // This node is the new root
        root = leftIndex;
    }
    else {
        // Determine if the target node was the left or right child of the parent
        if (index == nodes[parentIndex].left) {
            nodes[parentIndex].left = leftIndex;
        }
        else {
            nodes[parentIndex].right = leftIndex;
        }
    }

    // Move any orphaned nodes to the left of the target
    nodes[index].left = leftRightIndex;
    if (leftRightIndex != NullNode) {
        nodes[leftRightIndex].parent = index;
    }
}

/**
 * Performs a left rotation on the target index
 * 
 * @param index
 * The index to perform the rotation on
 */
void Treap::leftRotate(TreapIndex index) {
    TreapIndex parentIndex = nodes[index].parent;
    TreapIndex rightIndex = nodes[index].right;
    TreapIndex rightLeftIndex = nodes[rightIndex].left;

    // Move the target down to the left
    nodes[index].parent = rightIndex;
    nodes[rightIndex].left = index;

    // Hook the right node up to the target's old parent (or the root)
    nodes[rightIndex].parent = parentIndex;
    if (parentIndex == NullNode) {
        // This node is the new root
        root = rightIndex;
    }
    else {
        // Determine if the target node was the left or right child of the parent
        if (index == nodes[parentIndex].left) {
            nodes[parentIndex].left = rightIndex;
        }
        else {
            nodes[parentIndex].right = rightIndex;
        }
    }

    // Move any orphaned nodes to the right of the target
    nodes[index].right = rightLeftIndex;
    if (rightLeftIndex != NullNode) {
        nodes[rightLeftIndex].parent = index;
    }
}

/**
 * Moves a node up in the treap based on its weight
 * 
 * @param index
 * The index of the node to move up
 */
void Treap::moveUp(TreapIndex index) {
    while (true) {
        TreapIndex parentIndex = nodes[index].parent;

        // Stop when the current node becomes the root, or no longer has a smaller weight than its parent
        if (parentIndex == NullNode || nodes[index].weight >= nodes[parentIndex].weight) {
            return;
        }

        // Determine if the current node is the left or right child of the parent
        if (index == nodes[parentIndex].left) {
            rightRotate(parentIndex);
        }
        else {
            leftRotate(parentIndex);
        }
    }
}

/**
 * Moves a node down in the treap so that it becomes a leaf node
 * 
 * @param index
 * The index of the node to move down
 */
void Treap::moveDown(TreapIndex index) {
    while (true) {
        TreapIndex leftIndex = nodes[index].left;
        TreapIndex rightIndex = nodes[index].right;

        if (leftIndex == NullNode && rightIndex == NullNode) {
            // The node is a leaf node
            return;
        }

        if (leftIndex != NullNode && rightIndex != NullNode) {
            // The node has two children. Rotate in the direction of higher priority
            if (nodes[leftIndex].weight < nodes[rightIndex].weight) {
                rightRotate(index);
            }
            else {
                leftRotate(index);
            }
        }
        else if (leftIndex != NullNode) {
            // There is only a left child
            rightRotate(index);
        }
        else {
            // There is only a right child
            leftRotate(index);
        }
    }
}

/**
 * Inserts a node into the treap BST-style, based on its value
 * 
 * @param index
 * The index of the node to insert
 */
void Treap::bstInsert(TreapIndex index) {
    TreapIndex searchIndex = root;
    while (true) {
        if (nodes[searchIndex].val > nodes[index].val) {
            if (nodes[searchIndex].left == NullNode) {
                nodes[searchIndex].left = index;
                nodes[index].parent = searchIndex;
                return;
            }
            else {
                searchIndex = nodes[searchIndex].left;
            }
        }
        else {
            if (nodes[searchIndex].right == NullNode) {
                nodes[searchIndex].right = index;
                nodes[index].parent = searchIndex;
                return;
            }
            else {
                searchIndex = nodes[searchIndex].right;
            }
        }
    }
}

/**
 * Finds a node in the treap using a BST search
 * 
 * @param val
 * The value to find
 * 
 * @return TreapIndex
 * The index of the node containing the search value, or NullNode if it could not be found
 */
TreapIndex Treap::bstFind(int val) {
    TreapIndex searchIndex = root;
    while (searchIndex != NullNode) {
        if (nodes[searchIndex].val == val) {
            // The value was found
            return searchIndex;
        }
        else if (nodes[searchIndex].val > val) {
            // The value is smaller than this node
            searchIndex = nodes[searchIndex].left;
        }
        else {
            // The value is greater than this node
            searchIndex = nodes[searchIndex].right;
        }
    }

    // The value could not be found
    return NullNode;
}

/**
 * @brief
 * Inserts a value into the treap, maintining both BST ordering and heap ordering
 * 
 * @param val
 * The value to insert
 */
void Treap::insert(int val) {
    // Retrieve the node for this insertion
    TreapIndex newNodeIndex = createNewNode(val);

    if (size == 1) {
        // This is the first node in the treap. Make the new node the root.
        root = newNodeIndex;
        return;
    }

    // Perform BST insertion with the new node
    bstInsert(newNodeIndex);

    // Move the new node up based on its weight
    moveUp(newNodeIndex);
}

/**
 * Removes a value from the treap
 * 
 * @param val
 * The value to remove
 * 
 * @return true
 * If the node was removed
 * 
 * @return false
 * If the node did not exist in the treap
 */
bool Treap::remove(int val) {
    // Search for the target value
    TreapIndex foundIndex = bstFind(val);

    if (foundIndex == NullNode) {
        // The value could not be found
        return false;
    }

    // Move the target node down to a leaf node so it can be removed
    moveDown(foundIndex);

    // Cut off the node from the tree
    TreapIndex parentIndex = nodes[foundIndex].parent;
    if (parentIndex == NullNode) {
        // The root was removed
        root = NullNode;
    }
    else {
        // Determine if the removed node was the right or left child of its parent
        if (nodes[parentIndex].left == foundIndex) {
            nodes[parentIndex].left = NullNode;
        }
        else {
            nodes[parentIndex].right = NullNode;
        }
    }

    // Move the last node in the nodes array to fill the gap created by removing this node
    moveNode(size - 1, foundIndex);

    size--;
}

/**
 * Performs an immutable insertion of a value into a copy of the treap
 * 
 * @param val
 * The value to insert
 * 
 * @return Treap*
 * A pointer to a copy of the treap with the value inserted
 */
Treap *Treap::immutableInsert(int val) {
    // Copy the current object
    Treap *newTreap = new Treap(*this);

    // Insert the value in the copy
    newTreap->insert(val);

    return newTreap;
}

/**
 * Performs an immutable removal of a value from a copy of the treap
 * 
 * @param val
 * The value to remove
 * 
 * @return Treap*
 * A pointer to a copy of the treap with the value removed
 */
Treap *Treap::immutableRemove(int val) {
    // Copy the current object
    Treap *newTreap = new Treap(*this);

    // Remove the value from the copy
    newTreap->remove(val);

    return newTreap;
}

/**
 * Determine if a value is stored within the treap
 * 
 * @param val
 * The value to search for
 * 
 * @return true
 * If the value is in the treap
 * 
 * @return false
 * If the value is not in the treap
 */
bool Treap::contains(int val) {
    TreapIndex foundIndex = bstFind(val);

    return foundIndex != NullNode;
}
