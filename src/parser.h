#pragma once

#include <memory>
#include <vector>
#include "common.h"
#include "ast.h"
#include "token.h"
#include "error.h"
#include "types.h"

struct Compiler;

struct Parser : public TokenConsumer{
	CompilationUnit* unit;

	//u32 current = 0;
	ErrorHandler* err_handler;
	std::shared_ptr<AST> root_ast;
	std::shared_ptr<SymTable<Type>> sym_table;
	u8 parsing_constant_assignment = 0;
	u8 parsing_variable_assignment = 0;

	Parser();
	Parser(TokenList& tokens, CompilationUnit* unit);

	std::shared_ptr<AST> parse();
	std::shared_ptr<AST> parse_stmt_block();
	u8 end_of_block();
	u8 expecting_type();
	u8 expecting_expr();
	Type parse_type();
	std::shared_ptr<AST> parse_stmt();
	void do_newline();
	std::shared_ptr<AST> parse_directive();
	std::shared_ptr<AST> parse_if();
	std::shared_ptr<AST> parse_for();
	std::shared_ptr<AST> parse_expression();
	std::shared_ptr<AST> parse_define();
	std::shared_ptr<AST> parse_assign();
	std::shared_ptr<AST> parse_pattern();
	std::shared_ptr<AST> parse_lor(); // parse logical or
	std::shared_ptr<AST> parse_land(); // parse logical and
	std::shared_ptr<AST> parse_bor(); // parse bitwise or
	std::shared_ptr<AST> parse_band(); // parse bitwise and
	std::shared_ptr<AST> parse_eq(); // parse equality
	std::shared_ptr<AST> parse_comp(); // parse comparison
	std::shared_ptr<AST> parse_shift(); // bitwise shift
	std::shared_ptr<AST> parse_pm(); // plus minus
	std::shared_ptr<AST> parse_mdmr(); // mul div mod remain
	std::shared_ptr<AST> parse_un();
	std::shared_ptr<AST> parse_cast();
	std::shared_ptr<AST> parse_call();
	std::shared_ptr<AST> parse_single();
	std::shared_ptr<AST> parse_fn();
	std::shared_ptr<AST> parse_interface();
};