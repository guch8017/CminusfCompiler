#include "cminusf_builder.hpp"

// use these macros to get constant value
#define CONST_FP(num) \
    ConstantFP::get((float)num, module.get())
#define CONST_ZERO(type) \
    ConstantZero::get(var_type, module.get())


// You can define global variables here
// to store state

/*
 * use CMinusfBuilder::Scope to construct scopes
 * scope.enter: enter a new scope
 * scope.exit: exit current scope
 * scope.push: add a new binding to current scope
 * scope.find: find and return the value bound to the name
 */

void CminusfBuilder::visit(ASTProgram &node) { }

void CminusfBuilder::visit(ASTNum &node) { }

void CminusfBuilder::visit(ASTVarDeclaration &node) { }

void CminusfBuilder::visit(ASTFunDeclaration &node) { }

void CminusfBuilder::visit(ASTParam &node) { }

void CminusfBuilder::visit(ASTCompoundStmt &node) { }

void CminusfBuilder::visit(ASTExpressionStmt &node) { }

void CminusfBuilder::visit(ASTSelectionStmt &node) { }

void CminusfBuilder::visit(ASTIterationStmt &node) { }

void CminusfBuilder::visit(ASTReturnStmt &node) { }

void CminusfBuilder::visit(ASTVar &node) { }

void CminusfBuilder::visit(ASTAssignExpression &node) { }

void CminusfBuilder::visit(ASTSimpleExpression &node) { }

void CminusfBuilder::visit(ASTAdditiveExpression &node) { }

void CminusfBuilder::visit(ASTTerm &node) { }

void CminusfBuilder::visit(ASTCall &node) { }
