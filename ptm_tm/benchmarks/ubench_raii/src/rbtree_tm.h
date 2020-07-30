#pragma once

#include <functional>

#include "../../../common/tm_api.h"

#include "../common/config.h"

/// rbtree_tm is a red-black tree that uses coarse-grained
/// transactions for concurrency control
template <typename T> class rbtree_tm {
  /// making this a bool probably wouldn't help us any.
  enum Color { RED, BLACK };

  /// Node of an RBTree
  struct RBNode {
    Color m_color;      /// color (RED or BLACK)
    T m_val;            /// value stored at this node
    RBNode *m_parent;   /// pointer to parent
    int m_ID;           /// 0 or 1 to indicate if left or right child
    RBNode *m_child[2]; /// pointers to children

    /// basic constructor
    RBNode(Color color = BLACK, T val = -1, RBNode *parent = NULL, long ID = 0,
           RBNode *child0 = NULL, RBNode *child1 = NULL)
        : m_color(color), m_val(val), m_parent(parent), m_ID(ID) {
      m_child[0] = child0;
      m_child[1] = child1;
    }
  };
  RBNode *sentinel;

public:
  /// Construct a list by creating a sentinel node at the head
  rbtree_tm(config *cfg) : sentinel(new RBNode()) {}
  rbtree_tm() : sentinel(new RBNode()) {}

  // binary search for the node that has v as its value
  bool contains(T v) const {
    // void *dummy;
    bool res = false;
    // TX_PRIVATE_STACK_REGION(&dummy);
    {
      TX_RAII;
      RBNode *x = (sentinel->m_child[0]);
      while (x != NULL && x->m_val != v) {
        x = (x->m_child[(v < x->m_val) ? 0 : 1]);
      }
      res = (x != NULL && x->m_val == v);
    }
    return res;
  }

  // insert a node with v as its value if no such node exists in the tree
  bool insert(T v) {
    bool res = false;
    // TX_PRIVATE_STACK_REGION(&dummy);
    {
      TX_RAII;
      // find insertion point
      const RBNode *curr(sentinel);
      int cID = 0;
      RBNode *child((curr->m_child[cID]));

      while (child != NULL) {
        long cval = (child->m_val);
        if (cval == v)
          return false;
        cID = v < cval ? 0 : 1;
        curr = child;
        child = (curr->m_child[cID]);
      }
      res = true; // the insertion should return true

      // create the new node ("child") and attach it as curr->child[cID]
      child = (RBNode *)malloc(sizeof(RBNode));
      child->m_color = RED;
      child->m_val = v;
      child->m_parent = const_cast<RBNode *>(curr);
      child->m_ID = cID;
      child->m_child[0] = NULL;
      child->m_child[1] = NULL;

      RBNode *curr_rw = const_cast<RBNode *>(curr);
      const RBNode *child_r(child);
      curr_rw->m_child[cID] = child;

      // balance the tree
      while (true) {
        const RBNode *parent_r((child_r->m_parent));
        // read the parent of curr as gparent
        const RBNode *gparent_r((parent_r->m_parent));

        if ((gparent_r == sentinel) || (BLACK == (parent_r->m_color)))
          break;

        // cache the ID of the parent
        int pID = (parent_r->m_ID);
        // get parent's sibling as aunt
        const RBNode *aunt_r((gparent_r->m_child[1 - pID]));
        // gparent and parent will be written on all control paths
        RBNode *gparent_w = const_cast<RBNode *>(gparent_r);
        RBNode *parent_w = const_cast<RBNode *>(parent_r);

        if ((aunt_r != NULL) && (RED == (aunt_r->m_color))) {
          // set parent and aunt to BLACK, grandparent to RED
          parent_w->m_color = BLACK;
          RBNode *aunt_rw = const_cast<RBNode *>(aunt_r);
          aunt_rw->m_color = BLACK;
          gparent_w->m_color = RED;
          // now restart loop at gparent level
          child_r = gparent_w;
          continue;
        }

        int cID = (child_r->m_ID);
        if (cID != pID) {
          // promote child
          RBNode *child_rw = const_cast<RBNode *>(child_r);
          RBNode *baby((child_rw->m_child[1 - cID]));
          // set child's child to parent's cID'th child
          parent_w->m_child[cID] = baby;
          if (baby != NULL) {
            RBNode *baby_w(baby);
            baby_w->m_parent = parent_w;
            baby_w->m_ID = cID;
          }
          // move parent into baby's position as a child of child
          child_rw->m_child[1 - cID] = parent_w;
          parent_w->m_parent = child_rw;
          parent_w->m_ID = 1 - cID;
          // move child into parent's spot as pID'th child of gparent
          gparent_w->m_child[pID] = child_rw;
          child_rw->m_parent = gparent_w;
          child_rw->m_ID = pID;
          // promote(child_rw);
          // now swap child with curr and continue
          const RBNode *t(child_rw);
          child_r = parent_w;
          parent_w = const_cast<RBNode *>(t);
        }

        parent_w->m_color = BLACK;
        gparent_w->m_color = RED;
        // promote parent
        RBNode *ggparent_w((gparent_w->m_parent));
        int gID = (gparent_w->m_ID);
        RBNode *ochild = (parent_w->m_child[1 - pID]);
        // make gparent's pIDth child ochild
        gparent_w->m_child[pID] = ochild;
        if (ochild != NULL) {
          RBNode *ochild_w(ochild);
          ochild_w->m_parent = gparent_w;
          ochild_w->m_ID = pID;
        }
        // make gparent the 1-pID'th child of parent
        parent_w->m_child[1 - pID] = gparent_w;
        gparent_w->m_parent = parent_w;
        gparent_w->m_ID = 1 - pID;
        // make parent the gIDth child of ggparent
        ggparent_w->m_child[gID] = parent_w;
        parent_w->m_parent = ggparent_w;
        parent_w->m_ID = gID;
        // promote(parent_w);
      }

      // now just set the root to black
      const RBNode *sentinel_r(sentinel);
      const RBNode *root_r((sentinel_r->m_child[0]));
      if ((root_r->m_color) != BLACK) {
        RBNode *root_rw = const_cast<RBNode *>(root_r);
        root_rw->m_color = BLACK;
      }
    }

    return res;
  }

  // remove the node with v as its value if it exists in the tree
  bool remove(T v) {
    // void *dummy;
    bool res = false;
    // TX_PRIVATE_STACK_REGION(&dummy);
    {
      TX_RAII;
      // find v
      const RBNode *sentinel_r(sentinel);
      // rename x_r to x_rw, x_rr to x_r
      const RBNode *x_r((sentinel_r->m_child[0]));

      while (x_r != NULL) {
        int xval = (x_r->m_val);
        if (xval == v)
          break;
        x_r = (x_r->m_child[v < xval ? 0 : 1]);
      }

      // if we found v, remove it.  Otherwise return
      if (x_r == NULL)
        return false;
      res = true;
      RBNode *x_rw = const_cast<RBNode *>(x_r); // upgrade to writable

      // ensure that we are deleting a node with at most one child
      // cache value of rhs child
      RBNode *xrchild((x_rw->m_child[1]));
      if ((xrchild != NULL) && ((x_rw->m_child[0]) != NULL)) {
        // two kids!  find right child's leftmost child and swap it
        // with x
        const RBNode *leftmost_r((x_rw->m_child[1]));

        while ((leftmost_r->m_child[0]) != NULL)
          leftmost_r = (leftmost_r->m_child[0]);

        x_rw->m_val = (leftmost_r->m_val);
        x_rw = const_cast<RBNode *>(leftmost_r);
      }

      // extract x from the tree and prep it for deletion
      RBNode *parent_rw((x_rw->m_parent));
      int cID = ((x_rw->m_child[0]) != NULL) ? 0 : 1;
      RBNode *child = (x_rw->m_child[cID]);
      // make child the xID'th child of parent
      int xID = (x_rw->m_ID);
      parent_rw->m_child[xID] = child;
      if (child != NULL) {
        RBNode *child_w(child);
        child_w->m_parent = parent_rw;
        child_w->m_ID = xID;
      }

      // fix black height violations
      if ((BLACK == (x_rw->m_color)) && (child != NULL)) {
        const RBNode *c_r(child);
        if (RED == (c_r->m_color)) {
          RBNode *c_rw = const_cast<RBNode *>(c_r);
          x_rw->m_color = RED;
          c_rw->m_color = BLACK;
        }
      }

      // rebalance
      RBNode *curr(x_rw);
      while (true) {
        parent_rw = (curr->m_parent);
        if ((parent_rw == sentinel) || (RED == (curr->m_color)))
          break;
        int cID = (curr->m_ID);
        RBNode *sibling_w((parent_rw->m_child[1 - cID]));

        // we'd like y's sibling s to be black
        // if it's not, promote it and recolor
        if (RED == (sibling_w->m_color)) {
          /*
              Bp          Bs
             / \         / \
            By  Rs  =>  Rp  B2
            / \     / \
           B1 B2  By  B1
         */
          parent_rw->m_color = RED;
          sibling_w->m_color = BLACK;
          // promote sibling
          RBNode *gparent_w((parent_rw->m_parent));
          int pID = (parent_rw->m_ID);
          RBNode *nephew_w((sibling_w->m_child[cID]));
          // set nephew as 1-cID child of parent
          parent_rw->m_child[1 - cID] = nephew_w;
          nephew_w->m_parent = parent_rw;
          nephew_w->m_ID = (1 - cID);
          // make parent the cID child of the sibling
          sibling_w->m_child[cID] = parent_rw;
          parent_rw->m_parent = sibling_w;
          parent_rw->m_ID = cID;
          // make sibling the pID child of gparent
          gparent_w->m_child[pID] = sibling_w;
          sibling_w->m_parent = gparent_w;
          sibling_w->m_ID = pID;
          // reset sibling
          sibling_w = nephew_w;
        }

        RBNode *n = (sibling_w->m_child[1 - cID]);
        const RBNode *n_r(n); // if n is null, n_r will be null too
        if ((n != NULL) && (RED == (n_r->m_color))) {
          // the far nephew is red
          RBNode *n_rw(n);
          /*
             ?p          ?s
             / \         / \
            By  Bs  =>  Bp  Bn
           / \         / \
          ?1 Rn      By  ?1
          */
          sibling_w->m_color = (parent_rw->m_color);
          parent_rw->m_color = BLACK;
          n_rw->m_color = BLACK;
          // promote sibling_w
          RBNode *gparent_w((parent_rw->m_parent));
          int pID = (parent_rw->m_ID);
          RBNode *nephew((sibling_w->m_child[cID]));
          // make nephew the 1-cID child of parent
          parent_rw->m_child[1 - cID] = nephew;
          if (nephew != NULL) {
            RBNode *nephew_w(nephew);
            nephew_w->m_parent = parent_rw;
            nephew_w->m_ID = 1 - cID;
          }
          // make parent the cID child of the sibling
          sibling_w->m_child[cID] = parent_rw;
          parent_rw->m_parent = sibling_w;
          parent_rw->m_ID = cID;
          // make sibling the pID child of gparent
          gparent_w->m_child[pID] = sibling_w;
          sibling_w->m_parent = gparent_w;
          sibling_w->m_ID = pID;
          break; // problem solved
        }

        n = (sibling_w->m_child[cID]);
        n_r = n;
        if ((n != NULL) && (RED == (n_r->m_color))) {
          /*
               ?p          ?p
               / \         / \
             By  Bs  =>  By  Bn
                 / \           \
                Rn B1          Rs
                                 \
                                 B1
          */
          RBNode *n_rw = const_cast<RBNode *>(n_r);
          sibling_w->m_color = RED;
          n_rw->m_color = BLACK;
          RBNode *t = sibling_w;
          // promote n_rw
          RBNode *gneph((n_rw->m_child[1 - cID]));
          // make gneph the cID child of sibling
          sibling_w->m_child[cID] = gneph;
          if (gneph != NULL) {
            RBNode *gneph_w(gneph);
            gneph_w->m_parent = sibling_w;
            gneph_w->m_ID = cID;
          }
          // make sibling the 1-cID child of n
          n_rw->m_child[1 - cID] = sibling_w;
          sibling_w->m_parent = n_rw;
          sibling_w->m_ID = 1 - cID;
          // make n the 1-cID child of parent
          parent_rw->m_child[1 - cID] = n_rw;
          n_rw->m_parent = parent_rw;
          n_rw->m_ID = 1 - cID;
          sibling_w = n_rw;
          n_rw = t;

          // now the far nephew is red... copy of code from above
          sibling_w->m_color = (parent_rw->m_color);
          parent_rw->m_color = BLACK;
          n_rw->m_color = BLACK;
          // promote sibling_w
          RBNode *gparent_w((parent_rw->m_parent));
          int pID = (parent_rw->m_ID);
          RBNode *nephew((sibling_w->m_child[cID]));
          // make nephew the 1-cID child of parent
          parent_rw->m_child[1 - cID] = nephew;
          if (nephew != NULL) {
            RBNode *nephew_w(nephew);
            nephew_w->m_parent = parent_rw;
            nephew_w->m_ID = 1 - cID;
          }
          // make parent the cID child of the sibling
          sibling_w->m_child[cID] = parent_rw;
          parent_rw->m_parent = sibling_w;
          parent_rw->m_ID = cID;
          // make sibling the pID child of gparent
          gparent_w->m_child[pID] = sibling_w;
          sibling_w->m_parent = gparent_w;
          sibling_w->m_ID = pID;

          break; // problem solved
        }
        /*
             ?p          ?p
             / \         / \
           Bx  Bs  =>  Bp  Rs
               / \         / \
              B1 B2      B1  B2
         */

        sibling_w->m_color = RED; // propagate upwards

        // advance to parent and balance again
        curr = parent_rw;
      }

      // if y was red, this fixes the balance
      curr->m_color = BLACK;

      // free storage associated with deleted node
      free(x_rw);
    }

    return res;
  }

  void check() {
    // TODO
  }
};
