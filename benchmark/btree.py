#!/usr/bin/python

import sys

class TreeNode:
    left = None
    right = None

    def check(self):
        if self.left == None:
            return 1
        return self.left.check() + self.right.check() + 1

def CreateTreeNode(depth):
    return ChildTreeNodes(depth)

def ChildTreeNodes(depth):
    node = TreeNode()
    if depth > 0:
        node.left = ChildTreeNodes(depth - 1)
        node.right = ChildTreeNodes(depth - 1)
    return node


min_depth = 4
max_depth = 16
stretch_depth = max_depth + 1

print("stretch tree of depth %d\t check:" %
    stretch_depth, CreateTreeNode(stretch_depth).check())

long_lived_tree = CreateTreeNode(max_depth)

for depth in range(min_depth, stretch_depth, 2):

    check = 0
    iterations = 2**(max_depth - depth + min_depth)
    for i in range(1, iterations + 1):
        check += CreateTreeNode(depth).check()

    print("%d\t trees of depth %d\t check:" % (iterations, depth), check)

print("long lived tree of depth %d\t check:" %
    max_depth, long_lived_tree.check())
