#include <stdio.h>
#include <stdlib.h>
#include "tree.cpp"

static const char* TypeName(Types t)
{
    switch (t) {
        case NUM: return "NUM";
        case OP:  return "OP";
        case VAR: return "VAR";
        default:  return "???";
    }
}

static void FprintValue(FILE* out, const Node_t* node)
{
    switch (node->type) {
        case NUM: fprintf(out, "%d", node->data.num);            break;
        case OP:  fprintf(out, "%s", OpToStr(node->data.op));    break;
        case VAR: fprintf(out, "%s", VarToStr(node->data.var));  break;
        default:  fprintf(out, "???");                           break;
    }
}

static void DumpNode(const Node_t* node, FILE* out)
{
    if (!node) return;

    fprintf(out, "    node%p [shape=Mrecord, label=\"{ %s | ",
            (const void*)node, TypeName(node->type));
    FprintValue(out, node);
    fprintf(out, " }\"];\n");

    if (node->left) {
        fprintf(out, "    node%p -> node%p [color=\"#f70202\", label=\"L\"];\n",
                (const void*)node, (const void*)node->left);
        DumpNode(node->left, out);
    }
    if (node->right) {
        fprintf(out, "    node%p -> node%p [color=\"#0ef702\", label=\"R\"];\n",
                (const void*)node, (const void*)node->right);
        DumpNode(node->right, out);
    }
}

void TreeDumpPng(const Node_t* root, const char* png_path)
{
    char dot_path[512];
    snprintf(dot_path, sizeof dot_path, "%s.dot", png_path);

    FILE* f = fopen(dot_path, "w");
    if (!f) { perror("fopen"); return; }

    fprintf(f, "digraph ExprTree {\n");
    fprintf(f, "    node [shape=Mrecord, fontname=\"Consolas\", fontsize=10];\n");
    if (root) DumpNode(root, f);
    else      fprintf(f, "    empty [label=\"(null root)\"];\n");
    fprintf(f, "}\n");
    fclose(f);

    char cmd[1024];
    snprintf(cmd, sizeof cmd, "dot -Tpng \"%s\" -o \"%s\"", dot_path, png_path);

    int rc = system(cmd);
    if (rc != 0)
        fprintf(stderr, "[dump] dot failed (rc=%d). graphviz в PATH?\n", rc);
}

static void FprintNodeTex(FILE* out, const Node_t* node)
{
    if (!node) { fprintf(out, "?"); return; }

    switch (node->type)
    {
        case NUM:
            fprintf(out, "%d", node->data.num);
            break;

        case VAR:
            fprintf(out, "%s", VarToStr(node->data.var));
            break;

        case OP:
            switch (node->data.op)
            {
                case ADD:
                    fprintf(out, "\\left(");
                    FprintNodeTex(out, node->left);
                    fprintf(out, " + ");
                    FprintNodeTex(out, node->right);
                    fprintf(out, "\\right)");
                    break;

                case SUB:
                    fprintf(out, "\\left(");
                    FprintNodeTex(out, node->left);
                    fprintf(out, " - ");
                    FprintNodeTex(out, node->right);
                    fprintf(out, "\\right)");
                    break;

                case MUL:
                    fprintf(out, "\\left(");
                    FprintNodeTex(out, node->left);
                    fprintf(out, " \\cdot ");
                    FprintNodeTex(out, node->right);
                    fprintf(out, "\\right)");
                    break;

                case DIV:
                    fprintf(out, "\\frac{");
                    FprintNodeTex(out, node->left);
                    fprintf(out, "}{");
                    FprintNodeTex(out, node->right);
                    fprintf(out, "}");
                    break;

                default:
                    fprintf(out, "\\mathrm{?}");
                    break;
            }
            break;

        default:
            fprintf(out, "?");
            break;
    }
}

void TreeDumpTexFormula(const Node_t* root, const char* tex_path)
{
    FILE* f = fopen(tex_path, "w");
    if (!f) { perror("fopen"); return; }

    fprintf(f,
        "\\documentclass[border=10pt]{standalone}\n"
        "\\usepackage{amsmath}\n"
        "\\begin{document}\n"
        "$\n  ");
    if (root) FprintNodeTex(f, root);
    else      fprintf(f, "\\varnothing");
    fprintf(f,
        "\n$\n"
        "\\end{document}\n");
    fclose(f);

    char cmd[1024];
    snprintf(cmd, sizeof cmd,"pdflatex -interaction=nonstopmode -halt-on-error \"%s\" >nul 2>&1", tex_path);
    int rc = system(cmd);
    if (rc != 0)
        fprintf(stderr, "[dump] pdflatex failed (rc=%d). Есть ли pdflatex в PATH?\n", rc);
}