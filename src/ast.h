#pragma once

#include <sstream>
#include <string>
#include "common.h"
#include "types.h"
#include "token.h"


struct ASTVisitor;

struct StatementAST;
struct ExpressionAST;
struct ErrorAST;
struct ProgramAST;
struct StmtBlockAST;
struct StmtExpressionAST;
struct StmtDefineAST;
struct StmtInterfaceDefineAST; // e.g. app.main := () io.println "hello world"
struct StmtAssignAST;
struct StmtInterfaceAssignAST;
struct StmtReturnAST;
struct StmtContinueAST;
struct StmtBreakAST;
struct StmtIfAST;
struct StmtLoopAST;
struct ExprVarAST;
struct ExprPatternAST;
struct ExprInterfaceGetAST;
struct ExprBinAST;
struct ExprUnAST;
struct ExprGroupAST;
struct ExprLiteralAST;


struct AST {
	
	enum class ASTType{
		BASE,
		STMT,
		EXPR,
		ERR,
		PROG,
		STMT_BLOCK,
		STMT_EXPR,
		STMT_DEF,
		STMT_ASSIGN,
		STMT_INTER_ASSIGN,
		STMT_RET,
		STMT_CONT,
		STMT_BRK,
		STMT_IF,
		STMT_LOOP,
		EXPR_INTER,
		EXPR_FN,
		EXPR_VAR,
		EXPR_PATTERN,
		EXPR_INTER_GET,
		EXPR_BIN,
		EXPR_UN,
		EXPR_GROUP,
		EXPR_LIT,
	};
	
	u32 index;
	u32 line;
	u32 end_index;
	u32 end_line;

	virtual void debug();
	virtual std::string to_json();
	virtual ASTType type() { return ASTType::BASE; }
	virtual void* visit(ASTVisitor* visitor);
};


struct StatementAST : public AST {
	virtual std::string to_json();
	virtual ASTType type() { return ASTType::STMT; }
	virtual void* visit(ASTVisitor* visitor);
};

struct ExpressionAST : public AST {
	virtual ASTType type() { return ASTType::EXPR; }
	virtual void* visit(ASTVisitor* visitor);
};

struct ErrorAST : public StatementAST {
	std::string error_msg;
	virtual std::string to_json();
	virtual ASTType type() { return ASTType::ERR; }
	virtual void* visit(ASTVisitor* visitor);
};

struct ProgramAST : public StatementAST {
	std::vector<std::shared_ptr<AST>> stmts;
	virtual std::string to_json();
	virtual ASTType type() { return ASTType::PROG; }
	virtual void* visit(ASTVisitor* visitor);
};

struct StmtBlockAST : public StatementAST {
	std::vector<std::shared_ptr<AST>> stmts;
	virtual std::string to_json();
	virtual ASTType type() { return ASTType::STMT_BLOCK; }
	virtual void* visit(ASTVisitor* visitor);
};

struct StmtExpressionAST : public StatementAST {
	std::shared_ptr<AST> expression;
	virtual std::string to_json();
	virtual ASTType type() { return ASTType::STMT_EXPR; }
	virtual void* visit(ASTVisitor* visitor);
};

struct StmtDefineAST : public StatementAST {
	u8 is_global = 0;
	u8 is_initialised = 0;
	Token identifier;
	Type define_type;
	std::shared_ptr<AST> value;
	// if the define was a quick assign
	u8 requires_type_inference = 0;
	virtual std::string to_json();
	virtual ASTType  type() { return ASTType::STMT_DEF; }
	virtual void* visit(ASTVisitor* visitor);
};

struct StmtAssignAST : public StatementAST {
	Token variable;
	std::shared_ptr<AST> value;

	StmtAssignAST(Token variable, std::shared_ptr<AST> value) : variable(variable), value(value){}

	virtual std::string to_json();
	virtual ASTType  type() { return ASTType::STMT_ASSIGN; }
	virtual void* visit(ASTVisitor* visitor);
};

struct StmtInterfaceAssignAST : public StatementAST {
	std::shared_ptr<AST> variable; // the actual interface
	Token member;				   // the interface member
	std::shared_ptr<AST> value;    // the value to assign

	StmtInterfaceAssignAST(std::shared_ptr<AST> variable,
						   Token member,
						   std::shared_ptr<AST> value
		) : variable(variable), member(member), value(value) {}

	virtual std::string to_json();
	virtual ASTType  type() { return ASTType::STMT_INTER_ASSIGN; }
	virtual void* visit(ASTVisitor* visitor);
};

struct StmtReturnAST : public StatementAST {
	std::shared_ptr<ExpressionAST> value;
	StmtReturnAST(){}
	virtual std::string to_json();
	virtual ASTType  type() { return ASTType::STMT_RET; }
	virtual void* visit(ASTVisitor* visitor);
};

struct StmtContinueAST: public StatementAST {
	virtual std::string to_json();
	virtual ASTType  type() { return ASTType::STMT_CONT; }
	virtual void* visit(ASTVisitor* visitor);
};

struct StmtBreakAST : public StatementAST {
	virtual std::string to_json();
	virtual ASTType  type() { return ASTType::STMT_BRK; }
	virtual void* visit(ASTVisitor* visitor);
};

struct StmtIfAST : public StatementAST {
	std::shared_ptr<AST> if_cond;
	std::shared_ptr<AST> if_stmt;
	virtual std::string to_json();
	virtual ASTType  type() { return ASTType::STMT_IF; }
	virtual void* visit(ASTVisitor* visitor);
};

struct StmtLoopAST : public StatementAST {
	virtual std::string to_json();
	virtual ASTType  type() { return ASTType::STMT_LOOP; }
	virtual void* visit(ASTVisitor* visitor);
};

// e.g. x : interface = { y : s32 }
struct ExprInterfaceAST : public ExpressionAST {
	// even though they are first class, they still need a name
	std::string anonymous_name;
	ExprInterfaceAST(){}
	ExprInterfaceAST(std::string anonymous_name) : anonymous_name(anonymous_name) {}
	virtual std::string to_json();
	virtual ASTType type() { return ASTType::EXPR_INTER; }
	virtual void* visit(ASTVisitor* visitor);
};

// this is a lambda e.g. () io.println "hello world!", it can be assigned to variables e.g. x := () io.println "hello world!"
struct ExprFnAST : public ExpressionAST {
	// even though we can have lambdas e.g. () 1, they need a name
	std::string anonymous_name;
	std::shared_ptr<AST> body;
	u8 has_body = 0; // e.g. if we are declaring an external fn
	// the full type signature
	Type full_type;
	ExprFnAST() {}
	ExprFnAST(std::shared_ptr<AST> body, Type full_type) : body(body), full_type(full_type) {}
	virtual std::string to_json();
	virtual ASTType type() { return ASTType::EXPR_FN; }
	virtual void* visit(ASTVisitor* visitor);
};

struct ExprVarAST : public ExpressionAST {
	Token identifier;
	ExprVarAST(Token identifier) : identifier(identifier){}
	virtual std::string to_json();
	virtual ASTType  type() { return ASTType::EXPR_VAR; }
	virtual void* visit(ASTVisitor* visitor);
};


struct ExprPatternAST : public ExpressionAST {
	std::vector<std::shared_ptr<AST>> asts;
	ExprPatternAST(){}
	ExprPatternAST(std::vector<std::shared_ptr<AST>> asts) : asts(asts) {}
	virtual std::string to_json();
	virtual ASTType  type() { return ASTType::EXPR_PATTERN; }
	virtual void* visit(ASTVisitor* visitor);
};

struct ExprInterfaceGetAST : public ExpressionAST {
	std::shared_ptr<AST> value;
	Token member;
	virtual std::string to_json();
	virtual ASTType  type() { return ASTType::EXPR_INTER_GET; }
	virtual void* visit(ASTVisitor* visitor);
};


struct ExprBinAST : public ExpressionAST {
	std::shared_ptr<AST> lhs;
	std::shared_ptr<AST> rhs;
	Token op;
	ExprBinAST(){}
	ExprBinAST(std::shared_ptr<AST> lhs, std::shared_ptr<AST> rhs, Token op) : lhs(lhs), rhs(rhs), op(op){}
	virtual std::string to_json();
	virtual ASTType  type() { return ASTType::EXPR_BIN; }
	virtual void* visit(ASTVisitor* visitor);
};

struct ExprUnAST : public ExpressionAST {
	Token op;
	std::shared_ptr<AST> ast;
	u8 side; // left (0), right (1)

	ExprUnAST(){}
	ExprUnAST(Token op, std::shared_ptr<AST> ast, u8 side) : op(op), ast(ast), side(side){}

	virtual std::string to_json();
	virtual ASTType  type() { return ASTType::EXPR_UN; }
	virtual void* visit(ASTVisitor* visitor);
};

struct ExprGroupAST : public ExpressionAST {
	std::shared_ptr<AST> expression;
	ExprGroupAST(){}
	ExprGroupAST(std::shared_ptr<AST> expression) : expression(expression) {}
	virtual std::string to_json();
	virtual ASTType  type() { return ASTType::EXPR_GROUP; }
	virtual void* visit(ASTVisitor* visitor);
};

// @TODO technically a function is a literal?
struct ExprLiteralAST : public ExpressionAST {
	Type t;
	Value v;
	ExprLiteralAST(){}
	ExprLiteralAST(Type t, Value v) : t(t), v(v){}
	virtual std::string to_json();
	virtual ASTType  type() { return ASTType::EXPR_LIT; }
	virtual void* visit(ASTVisitor* visitor);
};

struct ASTVisitor {
	std::shared_ptr<AST> ast;

	ASTVisitor(){}
	ASTVisitor(std::shared_ptr<AST> ast) : ast(ast){}

	virtual void* visit_program(ProgramAST* program_ast) = 0;
	virtual void* visit_stmt_block(StmtBlockAST* stmt_block_ast) = 0;
	virtual void* visit_stmt_expression(StmtExpressionAST* stmt_expression_ast) = 0;
	virtual void* visit_stmt_define(StmtDefineAST* stmt_define_ast) = 0;
	virtual void* visit_stmt_interface_define(StmtInterfaceDefineAST* stmt_interface_define_ast) = 0;
	virtual void* visit_stmt_assign(StmtAssignAST* stmt_assign_ast) = 0;
	virtual void* visit_stmt_interface_assign(StmtInterfaceAssignAST* stmt_interface_assign_ast) = 0;
	virtual void* visit_stmt_return(StmtReturnAST* stmt_return_ast) = 0;
	virtual void* visit_stmt_continue_ast(StmtContinueAST* stmt_continue_ast) = 0;
	virtual void* visit_stmt_break_ast(StmtBreakAST* stmt_break_ast) = 0;
	virtual void* visit_stmt_if_ast(StmtIfAST* stmt_if_ast) = 0;
	virtual void* visit_stmt_loop_ast(StmtLoopAST* stmt_loop_ast) = 0;
	virtual void* visit_expr_inter_ast(ExprInterfaceAST* expr_interface_ast) = 0;
	virtual void* visit_expr_fn_ast(ExprFnAST* expr_fn_ast) = 0;
	virtual void* visit_expr_var_ast(ExprVarAST* expr_var_ast) = 0;
	virtual void* visit_expr_interface_get_ast(ExprInterfaceGetAST* expr_interface_get_ast) = 0;
	virtual void* visit_expr_bin_ast(ExprBinAST* expr_bin_ast) = 0;
	virtual void* visit_expr_un_ast(ExprUnAST* expr_un_ast) = 0;
	virtual void* visit_expr_group_ast(ExprGroupAST* expr_group_ast) = 0;
	virtual void* visit_expr_literal_ast(ExprLiteralAST* expr_literal_ast) = 0;
};