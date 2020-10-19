#ifndef _SYNTAX_TREE_HPP_
#define _SYNTAX_TREE_HPP_
extern "C" {
#include "syntax_tree.h"
    extern syntax_tree *parse(const char *input);
}
#include <vector>
#include <memory>

enum cminus_type {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_VOID
};

enum relop {
    // <=
    OP_LE,
    // <
    OP_LT,
    // >
    OP_GT,
    // >=
    OP_GE,
    // ==
    OP_EQ,
    // !=
    OP_NEQ
};

enum addop {
    // +
    OP_PLUS,
    // -
    OP_MINUS
};

enum mulop {
    // *
    OP_MUL,
    // /
    OP_DIV
};

class ast;

struct ast_node;
struct ast_program ;
struct ast_declaration;
struct ast_num;
struct ast_var_declaration;
struct ast_fun_declaration;
struct ast_param;
struct ast_compound_stmt;
struct ast_statement;
struct ast_expresion_stmt;
struct ast_selection_stmt;
struct ast_iteration_stmt;
struct ast_return_stmt;
struct ast_factor;
struct ast_expression;
struct ast_var;
struct ast_assign_expression;
struct ast_simple_expression;
struct ast_additive_expression;
struct ast_term ;
struct ast_call;

class ast_visitor;

class ast {
public:
    ast() = delete;
    ast(syntax_tree *);
    ast(ast &&tree) {
        root = tree.root;
        tree.root = nullptr;
    };
    ast_program* get_root() { return root.get(); }
    void run_visitor(ast_visitor& visitor);
private:
    ast_node* transform_node_iter(syntax_tree_node *);
    std::shared_ptr<ast_program> root = nullptr;
};

struct ast_node {
    virtual void accept(ast_visitor &) = 0;
};

struct ast_program : ast_node {
    virtual void accept(ast_visitor &) override final;
    std::vector<std::shared_ptr<ast_declaration>>
        declarations;
};

struct ast_declaration: ast_node {
    virtual void accept(ast_visitor &) override;
    cminus_type type;
    std::string id;
};

struct ast_factor: ast_node {
    virtual void accept(ast_visitor &) override;
};

struct ast_num: ast_factor {
    virtual void accept(ast_visitor &) override final;
    cminus_type type;
    union {
        int i_val;
        float f_val;
    };
};

struct ast_var_declaration: ast_declaration {
    virtual void accept(ast_visitor &) override final;
    std::shared_ptr<ast_num> num;
};

struct ast_fun_declaration: ast_declaration {
    virtual void accept(ast_visitor &) override final;
    std::vector<std::shared_ptr<ast_param>> params;
    std::shared_ptr<ast_compound_stmt> compound_stmt;
};

struct ast_param: ast_node {
    virtual void accept(ast_visitor &) override final;
    cminus_type type;
    std::string id;
    // true if it is array param
    bool isarray;
};

struct ast_statement : ast_node {
    virtual void accept(ast_visitor &) override;
};

struct ast_compound_stmt: ast_statement {
    virtual void accept(ast_visitor&) override final;
    std::vector<std::shared_ptr<ast_var_declaration>> local_declarations;
    std::vector<std::shared_ptr<ast_statement>> statement_list;
};

struct ast_expresion_stmt: ast_statement { 
    virtual void accept(ast_visitor &) override final;
    std::shared_ptr<ast_expression> expression; 
};

struct ast_selection_stmt: ast_statement {
    virtual void accept(ast_visitor &) override final;
    std::shared_ptr<ast_expression> expression; 
    std::shared_ptr<ast_statement> if_statement;
    // should be nullptr if no else structure exists
    std::shared_ptr<ast_statement> else_statement;
};

struct ast_iteration_stmt: ast_statement {
    virtual void accept(ast_visitor &) override final;
    std::shared_ptr<ast_expression> expression; 
    std::shared_ptr<ast_statement> statement;
};

struct ast_return_stmt: ast_statement {
    virtual void accept(ast_visitor &) override final;
    // should be nullptr if return void
    std::shared_ptr<ast_expression> expression; 
};

struct ast_expression: ast_factor {
    virtual void accept(ast_visitor &) override;
};

struct ast_assign_expression: ast_expression {
    virtual void accept(ast_visitor &) override final;
    std::shared_ptr<ast_var> var;
    std::shared_ptr<ast_expression> expression;
};

struct ast_simple_expression: ast_expression {
    virtual void accept(ast_visitor &) override final;
    std::shared_ptr<ast_additive_expression> additive_expression_l;
    std::shared_ptr<ast_additive_expression> additive_expression_r;
    relop op;
};

struct ast_var: ast_factor {
    virtual void accept(ast_visitor &) override final;
    std::string id;
    // nullptr if var is of int type
    std::shared_ptr<ast_expression> expression;
};

struct ast_additive_expression: ast_node {
    virtual void accept(ast_visitor &) override final;
    std::shared_ptr<ast_additive_expression> additive_expression;
    addop op;
    std::shared_ptr<ast_term> term;
};

struct ast_term : ast_node {
    virtual void accept(ast_visitor &) override final;
    std::shared_ptr<ast_term> term;
    mulop op;
    std::shared_ptr<ast_factor> factor;
};

struct ast_call: ast_factor {
    virtual void accept(ast_visitor &) override final;
    std::string id;
    std::vector<std::shared_ptr<ast_expression>> args;
};

class ast_visitor {
public:
    virtual void visit(ast_program &) = 0;
    virtual void visit(ast_num &) = 0;
    virtual void visit(ast_var_declaration &) = 0;
    virtual void visit(ast_fun_declaration &) = 0;
    virtual void visit(ast_param &) = 0;
    virtual void visit(ast_compound_stmt &) = 0;
    virtual void visit(ast_expresion_stmt &) = 0;
    virtual void visit(ast_selection_stmt &) = 0;
    virtual void visit(ast_iteration_stmt &) = 0;
    virtual void visit(ast_return_stmt &) = 0;
    virtual void visit(ast_assign_expression &) = 0;
    virtual void visit(ast_simple_expression &) = 0;
    virtual void visit(ast_additive_expression &) = 0;
    virtual void visit(ast_var &) = 0;
    virtual void visit(ast_term &) = 0;
    virtual void visit(ast_call &) = 0;
};

class ast_printer : public ast_visitor {
public:
    virtual void visit(ast_program &) override final;
    virtual void visit(ast_num &) override final;
    virtual void visit(ast_var_declaration &) override final;
    virtual void visit(ast_fun_declaration &) override final;
    virtual void visit(ast_param &) override final;
    virtual void visit(ast_compound_stmt &) override final;
    virtual void visit(ast_expresion_stmt &) override final;
    virtual void visit(ast_selection_stmt &) override final;
    virtual void visit(ast_iteration_stmt &) override final;
    virtual void visit(ast_return_stmt &) override final;
    virtual void visit(ast_assign_expression &) override final;
    virtual void visit(ast_simple_expression &) override final;
    virtual void visit(ast_additive_expression &) override final;
    virtual void visit(ast_var &) override final;
    virtual void visit(ast_term &) override final;
    virtual void visit(ast_call &) override final;
    void add_depth() { depth += 2; }
    void remove_depth() { depth -= 2; }
private:
    int depth = 0;
};
#endif

