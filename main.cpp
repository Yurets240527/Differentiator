#include "tree.cpp"
#include "Dump_t.cpp"

#define _NUM(v)   NewExpr(NUM, (v), NULL, NULL)
#define _VAR(v)   NewExpr(VAR, (v), NULL, NULL)

#define _ADD(l,r) NewExpr(OP, ADD, (l), (r))
#define _SUB(l,r) NewExpr(OP, SUB, (l), (r))
#define _MUL(l,r) NewExpr(OP, MUL, (l), (r))
#define _DIV(l,r) NewExpr(OP, DIV, (l), (r))

#define _SIN(l)   NewExpr(OP, SIN, (l), NULL)
#define _COS(l)   NewExpr(OP, COS, (l), NULL)

#define _DL Diff(node->left)
#define _DR Diff(node->right)

#define _CL node->left
#define _CR node->right

static Node_t* CopyTree(const Node_t* n);

Node_t* Diff(const Node_t* node);

int main()
{
    Tree* t = ReadTree("Source.txt");
    if (!t) { fprintf(stderr, "ReadTree failed\n"); return 1; }
    if (!t->root) { fprintf(stderr, "empty tree\n"); TreeDtor(t); return 1; }

    Print(t->root); printf("\n");

    Node_t* dif = Diff(t->root);
    if (!dif) { fprintf(stderr, "Diff failed\n"); TreeDtor(t); return 1; }

    Print(dif); printf("\n");

    TreeDumpPng(dif, "tree.png");
    TreeDumpTexFormula(t->root, "Source.tex");
    TreeDumpTexFormula(dif,    "Diff.tex");

    TreeFree(dif);
    TreeDtor(t);
    return 0;
}
Node_t* Diff(const Node_t* node)
{
    switch (node->type)
    {
        case NUM: return NewExpr(NUM, 0, NULL, NULL);

        case VAR: return _NUM(1);

        case OP:
            switch (node->data.op)
            {
                case ADD: return _ADD(Diff(_CL), Diff(_CR));
                case SUB: return _SUB(Diff(_CL), Diff(_CR));

                case MUL: return _ADD(_MUL(Diff(_CL), CopyTree(_CR)),
                                      _MUL(CopyTree(_CL), Diff(_CR)));

                case DIV: return _DIV(_SUB(_MUL(Diff(_CL), CopyTree(_CR)),
                                           _MUL(CopyTree(_CL), Diff(_CR))),
                                      _MUL(CopyTree(_CR), CopyTree(_CR)));

                case SIN: return _MUL(_COS(CopyTree(_CL)), Diff(_CL));
                case COS: return _MUL(_NUM(-1), _MUL(_SIN(CopyTree(_CL)), Diff(_CL)));
                
                default:  fprintf(stderr, "Diff: unknown op\n"); return NULL;
            }
    }
}

static Node_t* CopyTree(const Node_t* n)
{
    if (!n) return NULL;
    Node_t* c = NewNode(n->type, 0, NULL, LEFT);
    c->data = n->data;
    c->left  = CopyTree(n->left);  if (c->left)  c->left->parent  = c;
    c->right = CopyTree(n->right); if (c->right) c->right->parent = c;
    return c;
}
