#ifndef _BPLUS_TREE_H
#define _BPLUS_TREE_H
#include <cstdio>
#include <cstdlib>
#include <assert.h>
#include <string.h>
#include <iostream>

#define MIN_ORDER        3
#define MAX_ORDER        16
#define MAX_ENTRIES      16
#define MAX_LEVEL        12

#define MAX_NON_LEAF_NUM (1<<18)
#define MAX_LEAF_NUM     (1<<20)

struct bplus_node {
        long type;
        struct bplus_non_leaf *parent;
}__attribute__((aligned(32)));

struct bplus_non_leaf {
        long type;
        struct bplus_non_leaf *parent;
        struct bplus_non_leaf *next;
        long children;
        long key[MAX_ORDER - 1];
        struct bplus_node *sub_ptr[MAX_ORDER];
}__attribute__((aligned(32)));

struct bplus_leaf {
        long type;
        struct bplus_non_leaf *parent;
        struct bplus_leaf *next;
        long entries;
        long key[MAX_ENTRIES];
        long data[MAX_ENTRIES];
}__attribute__((aligned(32)));

struct bplus_tree {
        long order;
        long entries;
        long level;
        struct bplus_node* root;
        struct bplus_node* head[MAX_LEVEL];
}__attribute__((aligned(32)));

// [tiz214] main bplus tree functions
// long bplus_tree_get(struct bplus_tree *ptree, long key);
// long bplus_tree_put(struct bplus_tree *ptree, long key, long data);
// long bplus_tree_delete(struct bplus_tree *tree, long key);
// long bplus_tree_get_range(struct bplus_tree *tree, long key1, long key2);
// void bplus_tree_dump(struct bplus_tree *tree);
///////////////////////////////////////

enum {
        BPLUS_TREE_LEAF,
        BPLUS_TREE_NON_LEAF = 1,
};

enum {
        BORROW_FROM_LEFT,
        BORROW_FROM_RIGHT = 1,
};


long key_binary_search(long *arr, long len, long target)
{
        long low = -1;
        long high = len;
        while (low + 1 < high) {
                long mid = low + (high - low) / 2;
                if (target > arr[mid]) {
                        low = mid;
                } else {
                        high = mid;
                }
        }
        if (high >= len || arr[high] != target) {
                return -high - 1;
        } else {
                return high;
        }
}

//#define PRIVE_MEMORY_POOL

//---------BEGIN [tiz214] Private Node Pool implemenation ---------
#ifdef PRIVE_MEMORY_POOL
thread_local uint32_t t_lcounter = 0;
thread_local uint32_t t_nlcounter = 0;
thread_local bplus_leaf* t_leaves = nullptr;
thread_local bplus_non_leaf* t_nonleaves = nullptr; 
thread_local size_t t_local_max_size = 0;

// call this function to initialize the node poll
// memory leak option to elimilate the overhead for memory allocation
void initialize_private_node_pool(uint64_t total_threads, uint64_t total_nodes) {
	size_t local_nodes = total_nodes / total_threads;
	t_local_max_size = local_nodes;
	t_leaves = (bplus_leaf*)aligned_alloc(32, sizeof(bplus_leaf) * local_nodes);
	t_nonleaves = (bplus_non_leaf*)aligned_alloc(32, sizeof(bplus_non_leaf) * local_nodes);
}

struct bplus_leaf* get_leaf_from_pool() {
	assert(t_lcounter < t_local_max_size);
	return &t_leaves[t_lcounter++];
}

struct bplus_non_leaf* get_non_leaf_from_pool() {
	assert(t_nlcounter < t_local_max_size);
        return &t_nonleaves[t_nlcounter++];
}
#endif
//---------END [tiz214] Private Node Pool implemenation ---------

struct bplus_non_leaf *non_leaf_new()
{
#ifdef PRIVE_MEMORY_POOL
	struct bplus_non_leaf *node = get_non_leaf_from_pool(); // leak
#else
        struct bplus_non_leaf *node = //(struct bplus_non_leaf*)malloc(sizeof(struct bplus_non_leaf));
	(struct bplus_non_leaf*)aligned_alloc(32, sizeof(struct bplus_non_leaf));
#endif
	node->type = BPLUS_TREE_NON_LEAF;
        return node;
}


struct bplus_leaf *leaf_new()
{
#ifdef PRIVE_MEMORY_POOL
	struct bplus_leaf *node = get_leaf_from_pool(); // leak
#else
	struct bplus_leaf *node = //(struct bplus_leaf*)malloc(sizeof(struct bplus_leaf));
	(struct bplus_leaf*)aligned_alloc(32, sizeof(struct bplus_leaf));
#endif
	node->type = BPLUS_TREE_LEAF;
        return node;
} 


void non_leaf_delete(struct bplus_non_leaf *node)
{
#ifndef PRIVE_MEMORY_POOL
        free(node);
#endif
}


void leaf_delete(struct bplus_leaf *node)
{
#ifndef PRIVE_MEMORY_POOL
        free(node);
#endif
}

// ---------------------------------------------------------------

long bplus_tree_search(struct bplus_tree *tree, long key)
{
        long i;
        struct bplus_node *node = tree->root;
        struct bplus_non_leaf *nln;
        struct bplus_leaf *ln;

        while (node != nullptr) {
                switch (node->type) {
                case BPLUS_TREE_NON_LEAF:
                        nln = (struct bplus_non_leaf *)node;
                        i = key_binary_search(nln->key, nln->children - 1, key);
                        if (i >= 0) {
                                node = nln->sub_ptr[i + 1];
                        } else {
                                i = -i - 1;
                                node = nln->sub_ptr[i];
                        }
                        break;
                case BPLUS_TREE_LEAF:
                        ln = (struct bplus_leaf *)node;
                        i = key_binary_search(ln->key, ln->entries, key);
                        if (i >= 0) {
                                return ln->data[i];
                        } else {
                                return 0;
                        }
                /*default:
                        assert(0);*/
                }
        }

        return 0;
}


long non_leaf_insert(struct bplus_tree *tree, struct bplus_non_leaf *node, struct bplus_node *sub_node, long key, long level)
{       
        long i, j, split_key;
        long split = 0;
        struct bplus_non_leaf *sibling;

        long insert = key_binary_search(node->key, node->children - 1, key);
        //assert(insert < 0);
        insert = -insert - 1;

        /* node full */
        if (node->children == tree->order) {
                /* split = [m/2] */
                split = (tree->order + 1) / 2;
                /* splited sibling node */
                sibling = non_leaf_new();
                sibling->next = node->next;
                node->next = sibling;
                /* non-leaf node's children always equals to split + 1 after insertion */
                node->children = split + 1;
                /* sibling node replication due to location of insertion */
                if (insert < split) {
                        split_key = node->key[split - 1];
                        /* sibling node's first sub-node */
                        sibling->sub_ptr[0] = node->sub_ptr[split];
                        node->sub_ptr[split]->parent = sibling;
                        /* insertion point is before split point, replicate from key[split] */
                        for (i = split, j = 0; i < tree->order - 1; i++, j++) {
                                sibling->key[j] = node->key[i];
                                sibling->sub_ptr[j + 1] = node->sub_ptr[i + 1];
                                node->sub_ptr[i + 1]->parent = sibling;
                        }
                        sibling->children = j + 1;
                        /* insert new key and sub-node */
                        for (i = node->children - 2; i > insert; i--) {
                                node->key[i] = node->key[i - 1];
                                node->sub_ptr[i + 1] = node->sub_ptr[i];
                        }
                        node->key[i] = key;
                        node->sub_ptr[i + 1] = sub_node;
                        sub_node->parent = node;
                } else if (insert == split) {
                        split_key = key;
                        /* sibling node's first sub-node */
                        sibling->sub_ptr[0] = sub_node;
                        sub_node->parent = sibling;
                        /* insertion point is split point, replicate from key[split] */
                        for (i = split, j = 0; i < tree->order - 1; i++, j++) {
                                sibling->key[j] = node->key[i];
                                sibling->sub_ptr[j + 1] = node->sub_ptr[i + 1];
                                node->sub_ptr[i + 1]->parent = sibling;
                        }
                        sibling->children = j + 1;
                } else {
                        split_key = node->key[split];
                        /* sibling node's first sub-node */
                        sibling->sub_ptr[0] = node->sub_ptr[split + 1];
                        node->sub_ptr[split + 1]->parent = sibling;
                        /* insertion point is after split point, replicate from key[split + 1] */
                        for (i = split + 1, j = 0; i < tree->order - 1; j++) {
                                if (j != insert - split - 1) {
                                        sibling->key[j] = node->key[i];
                                        sibling->sub_ptr[j + 1] = node->sub_ptr[i + 1];
                                        node->sub_ptr[i + 1]->parent = sibling;
                                        i++;
                                }
                        }
                        /* reserve a hole for insertion */
                        if (j > insert - split - 1) {
                                sibling->children = j + 1;
                        } else {
                                // assert(j == insert - split - 1);
                                sibling->children = j + 2;
                        }
                        /* insert new key and sub-node*/
                        j = insert - split - 1;
                        sibling->key[j] = key;
                        sibling->sub_ptr[j + 1] = sub_node;
                        sub_node->parent = sibling;
                }
        } else {
                /* simple insertion */
                for (i = node->children - 1; i > insert; i--) {
                        node->key[i] = node->key[i - 1];
                        node->sub_ptr[i + 1] = node->sub_ptr[i];
                }
                node->key[i] = key;
                node->sub_ptr[i + 1] = sub_node;
                node->children++;
        }

        if (split) {
		if (tree->head[level + 1] == nullptr) {
                        if (++level >= tree->level) {
                                //fprintf(stderr, "!!Panic: Level exceeded, please expand the tree level, non-leaf order or leaf entries for element capacity!\n");
                                node->next = sibling->next;
                                non_leaf_delete(sibling);
                                return -1;
                        }
                        /* new parent */
                        struct bplus_non_leaf *parent = non_leaf_new();
                        parent->key[0] = split_key;
                        parent->sub_ptr[0] = (struct bplus_node *)node;
                        parent->sub_ptr[1] = (struct bplus_node *)sibling;
                        parent->children = 2;
			parent->next = nullptr;
			parent->parent = nullptr;
                        node->parent = parent;
                        sibling->parent = parent;


			/* update root */
                        tree->root = (struct bplus_node *)parent;
                        tree->head[level] = (struct bplus_node *)parent;
                } else {
                        /* Trace upwards */
                        sibling->parent = node->parent;
                        return non_leaf_insert(tree, node->parent, (struct bplus_node *)sibling, split_key, level + 1);
                }
        }

        return 0;
}


long leaf_insert(struct bplus_tree *tree, struct bplus_leaf *leaf, long key, long data)
{
        long i, j, split = 0;
        struct bplus_leaf *sibling;

        long insert = key_binary_search(leaf->key, leaf->entries, key);
        if (insert >= 0) {
                /* Already exists */
                return -1;
        }
        insert = -insert - 1;

        /* node full */
        if (leaf->entries == tree->entries) {
                /* split = [m/2] */
                split = (tree->entries + 1) / 2;
                /* splited sibling node */
                sibling = leaf_new();
                sibling->next = leaf->next;
                leaf->next = sibling;
                /* leaf node's entries always equals to split after insertion */
                leaf->entries = split;
                /* sibling leaf replication due to location of insertion */
                if (insert < split) {
                        /* insertion point is before split point, replicate from key[split - 1] */
                        for (i = split - 1, j = 0; i < tree->entries; i++, j++) {
                                sibling->key[j] = leaf->key[i];
                                sibling->data[j] = leaf->data[i];
                        }
                        sibling->entries = j;
                        /* insert new key and sub-node */
                        for (i = split - 1; i > insert; i--) {
                                leaf->key[i] = leaf->key[i - 1];
                                leaf->data[i] = leaf->data[i - 1];
                        }
                        leaf->key[i] = key;
                        leaf->data[i] = data;
                } else {
                        /* insertion point is or after split point, replicate from key[split] */
                        for (i = split, j = 0; i < tree->entries; j++) {
                                if (j != insert - split) {
                                        sibling->key[j] = leaf->key[i];
                                        sibling->data[j] = leaf->data[i];
                                        i++;
                                }
                        }
                        /* reserve a hole for insertion */
                        if (j > insert - split) {
                                sibling->entries = j;
                        } else {
                                // assert(j == insert - split);
                                sibling->entries = j + 1;
                        }
                        /* insert new key */
                        j = insert - split;
                        sibling->key[j] = key;
                        sibling->data[j] = data;
                }
        } else {
                /* simple insertion */
                for (i = leaf->entries; i > insert; i--) {
                        leaf->key[i] = leaf->key[i - 1];
                        leaf->data[i] = leaf->data[i - 1];
                }
                leaf->key[i] = key;
                leaf->data[i] = data;
                leaf->entries++;
        }

        if (split) {
                struct bplus_non_leaf *parent = leaf->parent;
                if (parent == nullptr) {
                        /* new parent */
                        parent = non_leaf_new();
                        parent->key[0] = sibling->key[0];
                        parent->sub_ptr[0] = (struct bplus_node *)leaf;
                        parent->sub_ptr[1] = (struct bplus_node *)sibling;
                        parent->children = 2;
			parent->next = nullptr;
			leaf->parent = parent;
                        sibling->parent = parent;

                        /* update root */
                        tree->root = (struct bplus_node *)parent;
                        tree->head[1] = (struct bplus_node *)parent;
                } else {
                        /* trace upwards */
                        sibling->parent = parent;
                        return non_leaf_insert(tree, parent, (struct bplus_node *)sibling, sibling->key[0], 1);
                }
        }

        return 0;
}


long
bplus_tree_insert(bplus_tree* tree, long key, long data)
{
        long i;
        struct bplus_node *node = tree->root;
        struct bplus_non_leaf *nln;
        struct bplus_leaf *ln, *root;

        while (node != nullptr) {
                switch (node->type) {
                case BPLUS_TREE_NON_LEAF:
                        nln = (struct bplus_non_leaf *)node;
                        i = key_binary_search(nln->key, nln->children - 1, key);
                        if (i >= 0) {
                                node = nln->sub_ptr[i + 1];
                        } else {
                                i = -i - 1;
                                node = nln->sub_ptr[i];
                        }
                        break;
                case BPLUS_TREE_LEAF:
                        ln = (struct bplus_leaf *)node;
                        return leaf_insert(tree, ln, key, data);
                }
        }

        /* new root */
        root = leaf_new();
        root->key[0] = key;
        root->data[0] = data;
        root->entries = 1;
	root->next = nullptr;
	root->parent = nullptr;
        tree->head[0] = (struct bplus_node *)root;
        tree->root = (struct bplus_node *)root;
        return 0;
}


void non_leaf_remove(struct bplus_tree *tree, struct bplus_non_leaf *node, long remove, long level)
{
        long i, j, k;
        struct bplus_non_leaf *sibling;

        if (node->children <= (tree->order + 1) / 2) {
                struct bplus_non_leaf *parent = node->parent;
                if (parent != nullptr) {
                        long borrow = 0;
                        /* find which sibling node with same parent to be borrowed from */
                        i = key_binary_search(parent->key, parent->children - 1, node->key[0]);
                        // assert(i < 0);
                        i = -i - 1;
                        if (i == 0) {
                                /* no left sibling, choose right one */
                                sibling = (struct bplus_non_leaf *)parent->sub_ptr[i + 1];
                                borrow = BORROW_FROM_RIGHT;
                        } else if (i == parent->children - 1) {
                                /* no right sibling, choose left one */
                                sibling = (struct bplus_non_leaf *)parent->sub_ptr[i - 1];
                                borrow = BORROW_FROM_LEFT;
                        } else {
                                struct bplus_non_leaf *l_sib = (struct bplus_non_leaf *)parent->sub_ptr[i - 1];
                                struct bplus_non_leaf *r_sib = (struct bplus_non_leaf *)parent->sub_ptr[i + 1];
                                /* if both left and right sibling found, choose the one with more children */
                                sibling = l_sib->children >= r_sib->children ? l_sib : r_sib;
                                borrow = l_sib->children >= r_sib->children ? BORROW_FROM_LEFT : BORROW_FROM_RIGHT;
                        }

                        /* locate parent node key to update later */
                        i = i - 1;

                        if (borrow == BORROW_FROM_LEFT) {
                                if (sibling->children > (tree->order + 1) / 2) {
                                        /* node's elements right shift */
                                        for (j = remove; j > 0; j--) {
                                                node->key[j] = node->key[j - 1];
                                        }
                                        for (j = remove + 1; j > 0; j--) {
                                                node->sub_ptr[j] = node->sub_ptr[j - 1];
                                        }
                                        /* parent key right rotation */
                                        node->key[0] = parent->key[i];
                                        parent->key[i] = sibling->key[sibling->children - 2];
                                        /* borrow the last sub-node from left sibling */
                                        node->sub_ptr[0] = sibling->sub_ptr[sibling->children - 1];
                                        sibling->sub_ptr[sibling->children - 1]->parent = node;
                                        sibling->children--;
                                } else {
                                        /* move parent key down */
                                        sibling->key[sibling->children - 1] = parent->key[i];
                                        /* merge with left sibling */
                                        for (j = sibling->children, k = 0; k < node->children - 1; k++) {
                                                if (k != remove) {
                                                        sibling->key[j] = node->key[k];
                                                        j++;
                                                }
                                        }
                                        for (j = sibling->children, k = 0; k < node->children; k++) {
                                                if (k != remove + 1) {
                                                        sibling->sub_ptr[j] = node->sub_ptr[k];
                                                        node->sub_ptr[k]->parent = sibling;
                                                        j++;
                                                }
                                        }
                                        sibling->children = j;
                                        /* delete merged node */
                                        sibling->next = node->next;
                                        non_leaf_delete(node);
                                        /* trace upwards */
                                        non_leaf_remove(tree, parent, i, level + 1);
                                }
                        } else {
                                /* remove key first in case of overflow during merging with sibling node */
                                for (; remove < node->children - 2; remove++) {
                                        node->key[remove] = node->key[remove + 1];
                                        node->sub_ptr[remove + 1] = node->sub_ptr[remove + 2];
                                }
                                node->children--;
                                if (sibling->children > (tree->order + 1) / 2) {
                                        /* parent key left rotation */
                                        node->key[node->children - 1] = parent->key[i + 1];
                                        parent->key[i + 1] = sibling->key[0];
                                        /* borrow the frist sub-node from right sibling */
                                        node->sub_ptr[node->children] = sibling->sub_ptr[0];
                                        sibling->sub_ptr[0]->parent = node;
                                        node->children++;
                                        /* left shift in right sibling */
                                        for (j = 0; j < sibling->children - 2; j++) {
                                                sibling->key[j] = sibling->key[j + 1];
                                        }
                                        for (j = 0; j < sibling->children - 1; j++) {
                                                sibling->sub_ptr[j] = sibling->sub_ptr[j + 1];
                                        }
                                        sibling->children--;
                                } else {
                                        /* move parent key down */
                                        node->key[node->children - 1] = parent->key[i + 1];
                                        node->children++;
                                        /* merge with right sibling */
                                        for (j = node->children - 1, k = 0; k < sibling->children - 1; j++, k++) {
                                                node->key[j] = sibling->key[k];
                                        }
                                        for (j = node->children - 1, k = 0; k < sibling->children; j++, k++) {
                                                node->sub_ptr[j] = sibling->sub_ptr[k];
                                                sibling->sub_ptr[k]->parent = node;
                                        }
                                        node->children = j;
                                        /* delete merged sibling */
                                        node->next = sibling->next;
                                        non_leaf_delete(sibling);
                                        /* trace upwards */
                                        non_leaf_remove(tree, parent, i + 1, level + 1);
                                }
                        }
                        /* deletion finishes */
                        return;
                } else {
                        if (node->children == 2) {
                                /* delete old root node */
                                // assert(remove == 0);
                                node->sub_ptr[0]->parent = nullptr;
                                tree->root = node->sub_ptr[0];
                                tree->head[level] = nullptr;
                                non_leaf_delete(node);
                                return;
                        }
                }
        }
        
        /* simple deletion */
        // assert(node->children > 2);
        for (; remove < node->children - 2; remove++) {
                node->key[remove] = node->key[remove + 1];
                node->sub_ptr[remove + 1] = node->sub_ptr[remove + 2];
        }
        node->children--;
}


long leaf_remove(struct bplus_tree *tree, struct bplus_leaf *leaf, long key)
{
        long i, j, k;
        struct bplus_leaf *sibling;

        long remove = key_binary_search(leaf->key, leaf->entries, key);
        if (remove < 0) {
                /* Not exist */
                return -1;
        }

        if (leaf->entries <= (tree->entries + 1) / 2) {
                struct bplus_non_leaf *parent = leaf->parent;
                if (parent != nullptr) {
                        long borrow = 0;
                        /* find which sibling node with same parent to be borrowed from */
                        i = key_binary_search(parent->key, parent->children - 1, leaf->key[0]);
                        if (i >= 0) {
                                i = i + 1;
                                if (i == parent->children - 1) {
                                        /* the last node, no right sibling, choose left one */
                                        sibling = (struct bplus_leaf *)parent->sub_ptr[i - 1];
                                        borrow = BORROW_FROM_LEFT;
                                } else {
                                        struct bplus_leaf *l_sib = (struct bplus_leaf *)parent->sub_ptr[i - 1];
                                        struct bplus_leaf *r_sib = (struct bplus_leaf *)parent->sub_ptr[i + 1];
                                        /* if both left and right sibling found, choose the one with more entries */
                                        sibling = l_sib->entries >= r_sib->entries ? l_sib : r_sib;
                                        borrow = l_sib->entries >= r_sib->entries ? BORROW_FROM_LEFT : BORROW_FROM_RIGHT;
                                }
                        } else {
                                i = -i - 1;
                                if (i == 0) {
                                        /* the frist node, no left sibling, choose right one */
                                        sibling = (struct bplus_leaf *)parent->sub_ptr[i + 1];
                                        borrow = BORROW_FROM_RIGHT;
                                } else if (i == parent->children - 1) {
                                        /* the last node, no right sibling, choose left one */
                                        sibling = (struct bplus_leaf *)parent->sub_ptr[i - 1];
                                        borrow = BORROW_FROM_LEFT;
                                } else {
                                        struct bplus_leaf *l_sib = (struct bplus_leaf *)parent->sub_ptr[i - 1];
                                        struct bplus_leaf *r_sib = (struct bplus_leaf *)parent->sub_ptr[i + 1];
                                        /* if both left and right sibling found, choose the one with more entries */
                                        sibling = l_sib->entries >= r_sib->entries ? l_sib : r_sib;
                                        borrow = l_sib->entries >= r_sib->entries ? BORROW_FROM_LEFT : BORROW_FROM_RIGHT;
                                }
                        }

                        /* locate parent node key to update later */
                        i = i - 1;

                        if (borrow == BORROW_FROM_LEFT) {
                                if (sibling->entries > (tree->entries + 1) / 2) {
                                        /* right shift in leaf node */
                                        for (; remove > 0; remove--) {
                                                leaf->key[remove] = leaf->key[remove - 1];
                                                leaf->data[remove] = leaf->data[remove - 1];
                                        }
                                        /* borrow the last element from left sibling */
                                        leaf->key[0] = sibling->key[sibling->entries - 1];
                                        leaf->data[0] = sibling->data[sibling->entries - 1];
                                        sibling->entries--;
                                        /* update parent key */
                                        parent->key[i] = leaf->key[0];
                                } else {
                                        /* merge with left sibling */
                                        for (j = sibling->entries, k = 0; k < leaf->entries; k++) {
                                                if (k != remove) {
                                                        sibling->key[j] = leaf->key[k];
                                                        sibling->data[j] = leaf->data[k];
                                                        j++;
                                                }
                                        }
                                        sibling->entries = j;
                                        /* delete merged leaf */
                                        sibling->next = leaf->next;
                                        leaf_delete(leaf);
                                        /* trace upwards */
                                        non_leaf_remove(tree, parent, i, 1);
                                }
                        } else {
                                /* remove element first in case of overflow during merging with sibling node */
                                for (; remove < leaf->entries - 1; remove++) {
                                        leaf->key[remove] = leaf->key[remove + 1];
                                        leaf->data[remove] = leaf->data[remove + 1];
                                }
                                leaf->entries--;
                                if (sibling->entries > (tree->entries + 1) / 2) {
                                        /* borrow the first element from right sibling */
                                        leaf->key[leaf->entries] = sibling->key[0];
                                        leaf->data[leaf->entries] = sibling->data[0];
                                        leaf->entries++;
                                        /* left shift in right sibling */
                                        for (j = 0; j < sibling->entries - 1; j++) {
                                                sibling->key[j] = sibling->key[j + 1];
                                                sibling->data[j] = sibling->data[j + 1];
                                        }
                                        sibling->entries--;
                                        /* update parent key */
                                        parent->key[i + 1] = sibling->key[0];
                                } else {
                                        /* merge with right sibling */
                                        for (j = leaf->entries, k = 0; k < sibling->entries; j++, k++) {
                                                leaf->key[j] = sibling->key[k];
                                                leaf->data[j] = sibling->data[k];
                                        }
                                        leaf->entries = j;
                                        /* delete right sibling */
                                        leaf->next = sibling->next;
                                        leaf_delete(sibling);
                                        /* trace upwards */
                                        non_leaf_remove(tree, parent, i + 1, 1);
                                }
                        }
                        /* deletion finishes */
                        return 0;
                } else {
                        if (leaf->entries == 1) {
                                /* delete the only last node */
                                //assert(key == leaf->key[0]);
                                tree->root = nullptr;
                                tree->head[0] = nullptr;
                                leaf_delete(leaf);
                                return 0;
                        }
                }
        }

        /* simple deletion */
        for (; remove < leaf->entries - 1; remove++) {
                leaf->key[remove] = leaf->key[remove + 1];
                leaf->data[remove] = leaf->data[remove + 1];
        }
        leaf->entries--;

        return 0;
}


long
bplus_tree_delete(struct bplus_tree *tree, long key)
{
        long i;
        struct bplus_node *node = tree->root;
        struct bplus_non_leaf *nln;
        struct bplus_leaf *ln;

        while (node != nullptr) {
                switch (node->type) {
                case BPLUS_TREE_NON_LEAF:
                        nln = (struct bplus_non_leaf *)node;
                        i = key_binary_search(nln->key, nln->children - 1, key);
                        if (i >= 0) {
                                node = nln->sub_ptr[i + 1];
                        } else {
                                i = -i - 1;
                                node = nln->sub_ptr[i];
                        }
                        break;
                case BPLUS_TREE_LEAF:
                        ln = (struct bplus_leaf *)node;
                        return leaf_remove(tree, ln, key);
                }
        }

        return -1;
}

void
bplus_tree_dump(struct bplus_tree *tree)
{
        long i, j;

        for (i = tree->level - 1; i > 0; i--) {
                struct bplus_non_leaf *node = (struct bplus_non_leaf *)tree->head[i];
                if (node != nullptr) {
                        printf("LEVEL %ld:\n", i);
                        while (node != nullptr) {
                                printf("node: ");
                                for (j = 0; j < node->children - 1; j++) {
                                        printf("%ld ", node->key[j]);
                                }
                                printf("\n");
                                node = node->next;
                        }
                }
        }

        struct bplus_leaf *leaf = (struct bplus_leaf *)tree->head[0];
        if (leaf != nullptr) {
                printf("LEVEL 0:\n");
                while (leaf != nullptr) {
                        printf("leaf: ");
                        for (j = 0; j < leaf->entries; j++) {
                                printf("%ld ", leaf->key[j]);
                        }
                        printf("\n");
                        leaf = leaf->next;
                }
        } else {
                printf("Empty tree!\n");
        }
}

long
bplus_tree_size(struct bplus_tree* tree){
        long i = 0;
        struct bplus_leaf * leaf = (struct bplus_leaf*)tree->head[0];
        while(leaf != nullptr){
                i++;
                leaf = leaf->next;
        }
        return i;
}

long
bplus_tree_get(struct bplus_tree *tree, long key)
{
        long data = bplus_tree_search(tree, key); 
        return data;
}


long
bplus_tree_put(struct bplus_tree *tree, long key, long data)
{
        return bplus_tree_insert(tree, key, data);
}

struct bplus_tree *
bplus_tree_init(long level, long order, long entries)
{
        /* The max order of non leaf nodes must be more than two */
        assert(MAX_ORDER > MIN_ORDER);
        assert(level <= MAX_LEVEL && order <= MAX_ORDER && entries <= MAX_ENTRIES);

        struct bplus_tree *tree = (struct bplus_tree *)aligned_alloc(32, sizeof(struct bplus_tree)); 
        tree->root = nullptr;
        tree->level = level;
        tree->order = order;
        tree->entries = entries;
        memset(tree->head, 0, MAX_LEVEL*sizeof(struct bplus_node*));
        return tree;
}

void
bplus_tree_deinit(struct bplus_tree *tree)
{
        free(tree);
}

long
bplus_tree_get_range(struct bplus_tree *tree, long key1, long key2)
{
    long i, data = 0;
    long min = key1 <= key2 ? key1 : key2;
    long max = min == key1 ? key2 : key1;
    struct bplus_node *node = tree->root;
    struct bplus_non_leaf *nln;
    struct bplus_leaf *ln;

    while (node != nullptr) {
            switch (node->type) {
            case BPLUS_TREE_NON_LEAF:
                    nln = (struct bplus_non_leaf *)node;
                    i = key_binary_search(nln->key, nln->children - 1, min);
                    if (i >= 0) {
                            node = nln->sub_ptr[i + 1];
                    } else  {
                            i = -i - 1;
                            node = nln->sub_ptr[i];
                    }
                    break;
            case BPLUS_TREE_LEAF:
                    ln = (struct bplus_leaf *)node;
                    i = key_binary_search(ln->key, ln->entries, min);
                    if (i < 0) {
                            i = -i - 1;
                            if (i >= ln->entries) {
                                    ln = ln->next;
                            }
                    }
                    while (ln != nullptr && ln->key[i] <= max) {
                            data = ln->data[i];
                            if (++i >= ln->entries) {
                                    ln = ln->next;
                                    i = 0;
                            }
                    }
                    return data;
            default:
                    assert(0);
            }
    }

    return 0;
}

#endif  /* _BPLUS_TREE_H */
