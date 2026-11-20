#ifndef TREE_H
#define TREE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

typedef const char* Elem_t;
#define TREE_FMT "%s"

#define LEFT false
#define RIGHT true

#define MAXLEN 128

enum Types
{
    NUM = 1,
    OP  = 2,
    VAR = 3,
};

enum Operations
{
    OP_NONE = 0,
    ADD = 1,
    SUB = 2,
    MUL = 3,
    DIV = 4,
    SIN = 5,
    COS = 6,
};

enum Vars
{
    VAR_NONE = 0,
    X = 1,
    Y = 2,
};

struct OpInfo
{
    const char* text;

    Operations code;

    bool is_un;
};

static const OpInfo Ops[] =
{
    { "+", ADD, false},
    { "-", SUB, false},
    { "*", MUL, false},
    { "/", DIV, false},
    { "sin", SIN, true},
    { "cos", COS, true},
};

#define OPS_COUNT (sizeof(Ops)/sizeof(Ops[0]))



union value
{
    int num;

    Operations op;

    Vars var;
};

struct VarInfo
{
    const char* text;
    Vars        code;

};

static const VarInfo Variables[] =
{
    { "x", X },
    { "y", Y },
};

#define VARS_COUNT (sizeof(Variables) / sizeof(Variables[0]))

struct Node_t
{
    Types type;

    value data;

    Node_t* left;
    Node_t* right;
    
    Node_t* parent;
};

struct Tree
{
    Node_t* root;

    size_t num_of_nodes;
};

struct Buffer
{
    char* buf;
    size_t size;
};

Node_t* NewNode(Types type, int arg, Node_t* parent, bool dir);

void DeleteLeaf(Node_t* node);

void Print(const Node_t* node);

void TreeFree(Node_t* node);

Operations  OpFromStr(const char* s);
const char* OpToStr(Operations op);

Vars        VarFromStr(const char* s);
const char* VarToStr(Vars v);


Node_t* NewNode(Types type, int arg, Node_t* parent, bool dir)
{
    Node_t* n = (Node_t*)calloc(1, sizeof(Node_t));
    if (!n) { fprintf(stderr, "calloc\n"); exit(1); }

    n->type   = type;
    n->parent = parent;

    switch (type)
    {
        case NUM: n->data.num = arg;            break;
        case OP:  n->data.op  = (Operations)arg; break;
        case VAR: n->data.var = (Vars)arg;       break;
        default:
            fprintf(stderr, "NewNode: bad type %d\n", (int)type);
            free(n);
            return NULL;
    }

    if (parent) {
        if (dir == LEFT) parent->left  = n;
        else             parent->right = n;
    }
    return n;
}

Node_t* NewExpr(Types type, int arg, Node_t* left, Node_t* right)
{
    Node_t* n = NewNode(type, arg, NULL, LEFT);
    if (!n) return NULL;
    n->left  = left;  if (left)  left->parent  = n;
    n->right = right; if (right) right->parent = n;
    return n;
}

Tree* TreeCtor()
{
    Tree* t = (Tree*)calloc(1, sizeof(Tree));
    if (!t) { fprintf(stderr, "calloc\n"); exit(1); }
    t->root = NULL;
    t->num_of_nodes = 0;
    return t;
}

void TreeDtor(Tree* t)
{
    if (!t) return;
    TreeFree(t->root);
    free(t);
}

void DeleteLeaf(Node_t* node)
{
    if (!node) return;
    if (node->left || node->right) return;

    if (node->parent) {
        if (node->parent->left  == node) node->parent->left  = NULL;
        if (node->parent->right == node) node->parent->right = NULL;
    }
    free(node);
}

void TreeFree(Node_t* node)
{
    if (!node) return;

    TreeFree(node->left);
    TreeFree(node->right);

    free(node);
}

static void PrintNodeData(const Node_t* n)
{
    switch (n->type)
    {
        case NUM: printf("%d", n->data.num);          break;
        case OP:  printf("%s", OpToStr(n->data.op));  break;
        case VAR: printf("%s", VarToStr(n->data.var));break;
    }
}

void Print(const Node_t* node)
{
    if (!node) return;

    printf("(");
    PrintNodeData(node);
    printf(" ");

    Print(node->left);

    Print(node->right);
    printf(") ");
}


int ReadFile(const char* filename, Buffer* preorder)
{
    if (!preorder) return 1;

    int fd = open(filename, O_RDONLY);
    if (fd < 0) { fprintf(stderr, "open\n"); return 1; }

    struct stat st;
    if (fstat(fd, &st) != 0) { fprintf(stderr, "fstat\n"); close(fd); return 1; }

    size_t sz = (size_t)st.st_size;

    preorder->buf = (char*) calloc(sz + 1, sizeof(char));
    if (!preorder->buf) { close(fd); return 1; }

    preorder->size = read(fd, preorder->buf, sz);
    if (preorder->size < 0) { fprintf(stderr, "read"); free(preorder->buf); close(fd); return 1; }

    preorder->buf[preorder->size] = '\0';

    close(fd);

    return 0;
}

static bool IsDigit(const char* s)
{
    if (!s || !*s) return false;

    if (*s == '-') s++;
    if (!*s) return false;
    for (; *s; ++s) if (*s < '0' || *s > '9') return false;
    return true;
}

static Node_t* MakeAtom(const char* atom, Node_t* parent, bool dir)
{
    if (IsDigit(atom))
        return NewNode(NUM, atoi(atom), parent, dir);

    Operations op = OpFromStr(atom);
    if (op != OP_NONE)
        return NewNode(OP, (int)op, parent, dir);

    Vars var = VarFromStr(atom);
    if (var == VAR_NONE) {
        fprintf(stderr, "Parse error: unknown identifier '%s'\n", atom);
        return NULL;
    }
    return NewNode(VAR, (int)var, parent, dir);
}

static const char* Skip(const char* p)
{
    while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
    return p;
}


static const char* ReadAtom(const char* p, char* buf, size_t bufsz)
{
    size_t i = 0;
    while (*p && *p != ' ' && *p != '\t' && *p != '\n' &&
           *p != '\r' && *p != '(' && *p != ')')
    {
        if (i + 1 >= bufsz) return NULL;
        buf[i++] = *p++;
    }
    buf[i] = '\0';
    return (i == 0) ? NULL : p;
}

static bool IsUnary(Operations op)
{
    if (op == OP_NONE) return false;
    for (size_t i = 0; i < OPS_COUNT; ++i)
        if (Ops[i].code == op)
            return Ops[i].is_un;
    return false;
}

Node_t* ParseSubtree(const char** p, Node_t* parent, bool dir, bool* ok)
{
    *ok = false;
    *p = Skip(*p);
    if (**p != '(') return NULL;
    (*p)++;
    *p = Skip(*p);

    if (**p == ')') { (*p)++; *ok = true; return NULL; }

    char buf[MAXLEN] = {};
    const char* next = ReadAtom(*p, buf, sizeof(buf));
    if (!next) return NULL;
    *p = next;
    *p = Skip(*p);

    if (**p == ')')
    {    
        (*p)++;
        Node_t* leaf = MakeAtom(buf, parent, dir);
        *ok = true;
        return leaf;
    }

    Operations op = OpFromStr(buf);
    if (op == OP_NONE)
    {
        fprintf(stderr, "Parse error: expected operator, got '%s'\n", buf);
        return NULL;
    }
    Node_t* node = NewNode(OP, (int)op, parent, dir);

    bool lok = false, rok = false;
    node->left = ParseSubtree(p, node, LEFT, &lok);
    if (!lok) { TreeFree(node); return NULL; }

    *p = Skip(*p);
    if (Ops[op].is_un && **p == ')')
    {
        (*p)++;
        *ok = true;
        return node;
    }
    node->right = ParseSubtree(p, node, RIGHT, &rok);
    if (!rok) { TreeFree(node); return NULL; }

    *p = Skip(*p);
    if (**p != ')') { TreeFree(node); return NULL; }
    (*p)++;
    *ok = true;
    return node;
}

Tree* ReadTree(const char* filename)
{
    Buffer preorder = {};
    if (ReadFile(filename, &preorder) != 0) return NULL;

    const char* p = preorder.buf;
    bool ok = false;
    Node_t* root = ParseSubtree(&p, NULL, LEFT, &ok);

    if (!ok) { free(preorder.buf); TreeFree(root); return NULL; }

    p = Skip(p);
    if (*p != '\0') {
        fprintf(stderr, "Parse error: trailing data near '%s'\n", p);
        free(preorder.buf);
        TreeFree(root);
        return NULL;
    }
    free(preorder.buf);

    Tree* t = TreeCtor();
    t->root = root;
    return t;
}

Operations OpFromStr(const char* s)
{
    if (!s) return OP_NONE;
    for (size_t i = 0; i < OPS_COUNT; ++i)
        if (strcmp(s, Ops[i].text) == 0)
            return Ops[i].code;
    return OP_NONE;
}

const char* OpToStr(Operations op)
{
    for (size_t i = 0; i < OPS_COUNT; ++i)
        if (Ops[i].code == op)
            return Ops[i].text;
    return "?";
}

Vars VarFromStr(const char* s)
{
    if (!s) return VAR_NONE;
    for (size_t i = 0; i < VARS_COUNT; ++i)
        if (strcmp(s, Variables[i].text) == 0)
            return Variables[i].code;
    return VAR_NONE;
}

const char* VarToStr(Vars v)
{
    for (size_t i = 0; i < VARS_COUNT; ++i)
        if (Variables[i].code == v)
            return Variables[i].text;
    return "?";
}



#endif