#include "red_black_tree.h"

#include <assert.h>

/***********************************************************************/
/*  FUNCTION:  rbt_init */
/**/
/*  INPUTS:  Initialize a red black tree of tree, nil for nil node, */
/*  root for root node, compre takes to void pointers to keys and returns */
/*  1 if the first arguement is "less than" the second. */
/**/
/*  OUTPUT:  none */
/**/
/*  Modifies Input: tree, nil, root */
/***********************************************************************/

void rbt_init( rbt_tree *tree,
            rbt_node *nil,
            rbt_node *root,
            int (*compare)(const void* a, const void* b)) {
  rbt_tree* newTree;
  rbt_node* temp;

  newTree=tree;
  newTree->compare=  compare;

  /*  see the comment in the rbt_tree structure in red_black_tree.h */
  /*  for information on nil and root */
  temp=newTree->nil= nil;
  temp->parent=temp->left=temp->right=temp;
  temp->red=0;
  temp->key=0;
  temp=newTree->root= root;
  temp->parent=temp->left=temp->right=newTree->nil;
  temp->key=0;
  temp->red=0;
}

/***********************************************************************/
/*  FUNCTION:  left_rotate */
/**/
/*  INPUTS:  This takes a tree so that it can access the appropriate */
/*           root and nil pointers, and the node to rotate on. */
/**/
/*  OUTPUT:  None */
/**/
/*  Modifies Input: tree, x */
/**/
/*  EFFECTS:  Rotates as described in _Introduction_To_Algorithms by */
/*            Cormen, Leiserson, Rivest (Chapter 14).  Basically this */
/*            makes the parent of x be to the left of x, x the parent of */
/*            its parent before the rotation and fixes other pointers */
/*            accordingly. */
/***********************************************************************/

void left_rotate(rbt_tree* tree, rbt_node* x) {
  rbt_node* y;
  rbt_node* nil=tree->nil;

  /*  I originally wrote this function to use the sentinel for */
  /*  nil to avoid checking for nil.  However this introduces a */
  /*  very subtle bug because sometimes this function modifies */
  /*  the parent pointer of nil.  This can be a problem if a */
  /*  function which calls left_rotate also uses the nil sentinel */
  /*  and expects the nil sentinel's parent pointer to be unchanged */
  /*  after calling this function.  For example, when rbt_removeFixUP */
  /*  calls left_rotate it expects the parent pointer of nil to be */
  /*  unchanged. */

  y=x->right;
  x->right=y->left;

  if (y->left != nil) y->left->parent=x; /* used to use sentinel here */
  /* and do an unconditional assignment instead of testing for nil */
  
  y->parent=x->parent;   

  /* instead of checking if x->parent is the root as in the book, we */
  /* count on the root sentinel to implicitly take care of this case */
  if( x == x->parent->left) {
    x->parent->left=y;
  } else {
    x->parent->right=y;
  }
  y->left=x;
  x->parent=y;

#ifdef DEBUG_ASSERT
  // nil not red in left_rotate
  assert(!tree->nil->red);
#endif
}


/***********************************************************************/
/*  FUNCTION:  right_rotate */
/**/
/*  INPUTS:  This takes a tree so that it can access the appropriate */
/*           root and nil pointers, and the node to rotate on. */
/**/
/*  OUTPUT:  None */
/**/
/*  Modifies Input?: tree, y */
/**/
/*  EFFECTS:  Rotates as described in _Introduction_To_Algorithms by */
/*            Cormen, Leiserson, Rivest (Chapter 14).  Basically this */
/*            makes the parent of x be to the left of x, x the parent of */
/*            its parent before the rotation and fixes other pointers */
/*            accordingly. */
/***********************************************************************/

void right_rotate(rbt_tree* tree, rbt_node* y) {
  rbt_node* x;
  rbt_node* nil=tree->nil;

  /*  I originally wrote this function to use the sentinel for */
  /*  nil to avoid checking for nil.  However this introduces a */
  /*  very subtle bug because sometimes this function modifies */
  /*  the parent pointer of nil.  This can be a problem if a */
  /*  function which calls left_rotate also uses the nil sentinel */
  /*  and expects the nil sentinel's parent pointer to be unchanged */
  /*  after calling this function.  For example, when rbt_removeFixUP */
  /*  calls left_rotate it expects the parent pointer of nil to be */
  /*  unchanged. */

  x=y->left;
  y->left=x->right;

  if (nil != x->right)  x->right->parent=y; /*used to use sentinel here */
  /* and do an unconditional assignment instead of testing for nil */

  /* instead of checking if x->parent is the root as in the book, we */
  /* count on the root sentinel to implicitly take care of this case */
  x->parent=y->parent;
  if( y == y->parent->left) {
    y->parent->left=x;
  } else {
    y->parent->right=x;
  }
  x->right=y;
  y->parent=x;

#ifdef DEBUG_ASSERT
  // nil not red in right_rotate
  assert(!tree->nil->red);
#endif
}

/***********************************************************************/
/*  FUNCTION:  tree_insert_help  */
/**/
/*  INPUTS:  tree is the tree to insert into and z is the node to insert */
/**/
/*  OUTPUT:  none */
/**/
/*  Modifies Input:  tree, z */
/**/
/*  EFFECTS:  Inserts z into the tree as if it were a regular binary tree */
/*            using the algorithm described in _Introduction_To_Algorithms_ */
/*            by Cormen et al.  This funciton is only intended to be called */
/*            by the rbt_insert function and not by the user */
/***********************************************************************/

void tree_insert_help(rbt_tree* tree, rbt_node* z) {
  /*  This function should only be called by InsertRBTree (see above) */
  rbt_node* x;
  rbt_node* y;
  rbt_node* nil=tree->nil;
  
  z->left=z->right=nil;
  y=tree->root;
  x=tree->root->left;
  while( x != nil) {
    y=x;
    if (1 == tree->compare(x->key,z->key)) { /* x.key > z.key */
      x=x->left;
    } else { /* x,key <= z.key */
      x=x->right;
    }
  }
  z->parent=y;
  if ( (y == tree->root) ||
       (1 == tree->compare(y->key,z->key))) { /* y.key > z.key */
    y->left=z;
  } else {
    y->right=z;
  }

#ifdef DEBUG_ASSERT
  // nil not red in tree_insert_help
  assert(!tree->nil->red);
#endif
}

/*  Before calling Insert RBTree the node x should have its key set */

/***********************************************************************/
/*  FUNCTION:  rbt_insert */
/**/
/*  INPUTS:  tree is the red-black tree to insert a node x.  */
/**/
/*  OUTPUT:  x */
/**/
/*  EFFECTS:  Creates a node node which contains the appropriate key and */
/*            info pointers and inserts it into the tree. */
/***********************************************************************/

void rbt_insert(rbt_tree* tree, rbt_node *x) {
  rbt_node * y;

  tree_insert_help(tree,x);
  x->red=1;
  while(x->parent->red) { /* use sentinel instead of checking for root */
    if (x->parent == x->parent->parent->left) {
      y=x->parent->parent->right;
      if (y->red) {
	x->parent->red=0;
	y->red=0;
	x->parent->parent->red=1;
	x=x->parent->parent;
      } else {
	if (x == x->parent->right) {
	  x=x->parent;
	  left_rotate(tree,x);
	}
	x->parent->red=0;
	x->parent->parent->red=1;
	right_rotate(tree,x->parent->parent);
      } 
    } else { /* case for x->parent == x->parent->parent->right */
      y=x->parent->parent->left;
      if (y->red) {
	x->parent->red=0;
	y->red=0;
	x->parent->parent->red=1;
	x=x->parent->parent;
      } else {
	if (x == x->parent->left) {
	  x=x->parent;
	  right_rotate(tree,x);
	}
	x->parent->red=0;
	x->parent->parent->red=1;
	left_rotate(tree,x->parent->parent);
      } 
    }
  }
  tree->root->left->red=0;

#ifdef DEBUG_ASSERT
  // nil not red in rbt_insert
  assert(!tree->nil->red);
  // root not red in rbt_insert
  assert(!tree->root->red);
#endif
}

/***********************************************************************/
/*  FUNCTION:  tree_successor  */
/**/
/*    INPUTS:  tree is the tree in question, and x is the node we want the */
/*             the successor of. */
/**/
/*    OUTPUT:  This function returns the successor of x or NULL if no */
/*             successor exists. */
/**/
/*    Modifies Input: none */
/**/
/*    Note:  uses the algorithm in _Introduction_To_Algorithms_ */
/***********************************************************************/
  
static rbt_node* tree_successor(rbt_tree* tree,rbt_node* x) { 
  rbt_node* y;
  rbt_node* nil=tree->nil;
  rbt_node* root=tree->root;

  if (nil != (y = x->right)) { /* assignment to y is intentional */
    while(y->left != nil) { /* returns the minium of the right subtree of x */
      y=y->left;
    }
    return(y);
  } else {
    y=x->parent;
    while(x == y->right) { /* sentinel used instead of checking for nil */
      x=y;
      y=y->parent;
    }
    if (y == root) return(nil);
    return(y);
  }
}

/***********************************************************************/
/*  FUNCTION:  tree_predecessor  */
/**/
/*    INPUTS:  tree is the tree in question, and x is the node we want the */
/*             the predecessor of. */
/**/
/*    OUTPUT:  This function returns the predecessor of x or NULL if no */
/*             predecessor exists. */
/**/
/*    Modifies Input: none */
/**/
/*    Note:  uses the algorithm in _Introduction_To_Algorithms_ */
/***********************************************************************/

static rbt_node* tree_predecessor(rbt_tree* tree, rbt_node* x) {
  rbt_node* y;
  rbt_node* nil=tree->nil;
  rbt_node* root=tree->root;

  if (nil != (y = x->left)) { /* assignment to y is intentional */
    while(y->right != nil) { /* returns the maximum of the left subtree of x */
      y=y->right;
    }
    return(y);
  } else {
    y=x->parent;
    while(x == y->left) { 
      if (y == root) return(nil); 
      x=y;
      y=y->parent;
    }
    return(y);
  }
}

/***********************************************************************/
/*  FUNCTION:  rbt_query_ge */
/**/
/*    INPUTS:  tree is the tree to print and q is a less than or equal to the key */
/*             we are searching for */
/**/
/*    OUTPUT:  returns the a node with key equal to q.  If there are */
/*             multiple nodes with key equal to q this function returns */
/*             the one highest in the tree */
/**/
/*    Modifies Input: none */
/**/
/***********************************************************************/
  
rbt_node* rbt_query_ge(rbt_tree* tree, void* q) {
  rbt_node* y=0;
  rbt_node* x=tree->root->left;
  rbt_node* nil=tree->nil;
  int compVal;
  if (x == nil) return(0);
  compVal=tree->compare(x->key,(int*) q);
  while(0 != compVal) {/*assignemnt*/
    if (1 == compVal) { /* x->key > q */
      x=x->left;
      y=x;
    } else {
      x=x->right;
    }
    if ( x == nil) return(0);
    compVal=tree->compare(x->key,(int*) q);
  }
  return(compVal == 0 ? x : y);
}

/***********************************************************************/
/*  FUNCTION:  rbt_remove_fix_up */
/**/
/*    INPUTS:  tree is the tree to fix and x is the child of the spliced */
/*             out node in RBTreeDelete. */
/**/
/*    OUTPUT:  none */
/**/
/*    EFFECT:  Performs rotations and changes colors to restore red-black */
/*             properties after a node is deleted */
/**/
/*    Modifies Input: tree, x */
/**/
/*    The algorithm from this function is from _Introduction_To_Algorithms_ */
/***********************************************************************/

static void rbt_remove_fix_up(rbt_tree* tree, rbt_node* x) {
  rbt_node* root=tree->root->left;
  rbt_node* w;

  while( (!x->red) && (root != x)) {
    if (x == x->parent->left) {
      w=x->parent->right;
      if (w->red) {
	w->red=0;
	x->parent->red=1;
	left_rotate(tree,x->parent);
	w=x->parent->right;
      }
      if ( (!w->right->red) && (!w->left->red) ) { 
	w->red=1;
	x=x->parent;
      } else {
	if (!w->right->red) {
	  w->left->red=0;
	  w->red=1;
	  right_rotate(tree,w);
	  w=x->parent->right;
	}
	w->red=x->parent->red;
	x->parent->red=0;
	w->right->red=0;
	left_rotate(tree,x->parent);
	x=root; /* this is to exit while loop */
      }
    } else { /* the code below is has left and right switched from above */
      w=x->parent->left;
      if (w->red) {
	w->red=0;
	x->parent->red=1;
	right_rotate(tree,x->parent);
	w=x->parent->left;
      }
      if ( (!w->right->red) && (!w->left->red) ) { 
	w->red=1;
	x=x->parent;
      } else {
	if (!w->left->red) {
	  w->right->red=0;
	  w->red=1;
	  left_rotate(tree,w);
	  w=x->parent->left;
	}
	w->red=x->parent->red;
	x->parent->red=0;
	w->left->red=0;
	right_rotate(tree,x->parent);
	x=root; /* this is to exit while loop */
      }
    }
  }
  x->red=0;

#ifdef DEBUG_ASSERT
  // nil not black in rbt_remove_fix_up
  assert(!tree->nil->red);
#endif
}


/***********************************************************************/
/*  FUNCTION:  rbt_remove */
/**/
/*    INPUTS:  tree is the tree to delete node z from */
/**/
/*    OUTPUT:  none */
/**/
/*    EFFECT:  Deletes z from tree and frees the key and info of z */
/*             using DestoryKey and DestoryInfo.  Then calls */
/*             rbt_remove_fix_up to restore red-black properties */
/**/
/*    Modifies Input: tree, z */
/**/
/*    The algorithm from this function is from _Introduction_To_Algorithms_ */
/***********************************************************************/

void rbt_remove(rbt_tree* tree, rbt_node* z){
  rbt_node* y;
  rbt_node* x;
  rbt_node* nil=tree->nil;
  rbt_node* root=tree->root;

  y= ((z->left == nil) || (z->right == nil)) ? z : tree_successor(tree,z);
  x= (y->left == nil) ? y->right : y->left;
  if (root == (x->parent = y->parent)) { /* assignment of y->p to x->p is intentional */
    root->left=x;
  } else {
    if (y == y->parent->left) {
      y->parent->left=x;
    } else {
      y->parent->right=x;
    }
  }
  if (y != z) { /* y should not be nil in this case */

#ifdef DEBUG_ASSERT
    // y is nil in rbt_remove
    assert( (y!=tree->nil));
#endif
    /* y is the node to splice out and x is its child */

    if (!(y->red)) rbt_remove_fix_up(tree,x);
  
    y->left=z->left;
    y->right=z->right;
    y->parent=z->parent;
    y->red=z->red;
    z->left->parent=z->right->parent=y;
    if (z == z->parent->left) {
      z->parent->left=y; 
    } else {
      z->parent->right=y;
    }
  } else {
    if (!(y->red)) rbt_remove_fix_up(tree,x);
  }
  
#ifdef DEBUG_ASSERT
  // nil not black in rbt_remove
  assert(!tree->nil->red);
#endif
}
