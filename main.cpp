#include "tree.cpp"
#include "Dump_t.cpp"

#define _NUM(v)   NewExpr(NUM, (v), NULL, NULL)
#define _VAR(v)   NewExpr(VAR, (v), NULL, NULL)

#define _ADD(l,r) NewExpr(OP, ADD, (l), (r))
#define _SUB(l,r) NewExpr(OP, SUB, (l), (r))
#define _MUL(l,r) NewExpr(OP, MUL, (l), (r))
#define _DIV(l,r) NewExpr(OP, DIV, (l), (r))

int main()
{
    Tree* t = TreeCtor();
    t = ReadTree("Source.txt");
    Node_t* expr = NewExpr(OP, ADD, NewExpr(NUM, 2, NULL, NULL), NewExpr(VAR, X, NULL, NULL));
    Node_t* expr1 = _MUL(_NUM(2), _VAR(X));
    Print(expr1);
    TreeDumpPng(expr1, "tree.png");
    TreeDumpTexFormula(t->root, "TexDump.tex");
    TreeDtor(t);
    int rc = system("pdflatex --version");
    printf("rc=%d\n", rc);
}
