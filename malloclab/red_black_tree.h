#define DEBUG_ASSERT 1

typedef struct rbt_node {
  void* key;
  int red; /* if red=0 then the node is black */
  struct rbt_node* left;
  struct rbt_node* right;
  struct rbt_node* parent;
} rbt_node;

typedef struct rbt_tree {
  int (*compare)(const void* a, const void* b); 
  rbt_node* root;             
  rbt_node* nil;              
} rbt_tree;

void rbt_init( rbt_tree *tree,
            rbt_node *nil,
            rbt_node *root,
            int (*compare)(const void* a, const void* b));
void rbt_insert(rbt_tree* tree, rbt_node *node);
void rbt_remove(rbt_tree* , rbt_node* );
rbt_node* rbt_query_ge(rbt_tree*, void*);
