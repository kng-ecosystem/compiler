/*
James Clarke - 2021
*/

#include "parser.h"
#include "compiler.h"

Parser::Parser(){}

Parser::Parser(TokenList& tokens, CompilationUnit* unit){
	this->tokens = tokens.tokens;
	this->unit = unit;
	this->sym_table = std::make_shared<SymTable<Type>>();
}


std::shared_ptr<AST> Parser::parse() {
	this->root_ast = std::make_shared<AST>();
	auto program = ProgramAST();
	// @TODO ignore newlines when parsing stuff
	while (!end() && peek().type != Token::Type::END)	
		program.stmts.push_back(parse_stmt());
	this->root_ast = std::make_shared<ProgramAST>(program);
	return this->root_ast;
}

std::shared_ptr<AST> Parser::parse_stmt(){
	// consume empty newlines
	requiring_delimiter = 0; // e.g. if 1 {} doesn't require a delimiter
	std::shared_ptr<AST> stmt = std::make_shared<ErrorAST>();
	switch (peek().type) {
		case Token::Type::HASH: {
			stmt = parse_directive();
			break;
		}
		case Token::Type::RETURN:{
			StmtReturnAST r;
			requiring_delimiter = 1;
			next();
			if (expecting_expr())
				r.value = parse_expression();
			stmt = std::make_shared<StmtReturnAST>(r);
			break;
		}
		case Token::Type::CONTINUE: {
			requiring_delimiter = 1;
			next();
			stmt = std::make_shared<StmtContinueAST>();
			break;
		}
		case Token::Type::BREAK: {
			requiring_delimiter = 1;
			next();
			stmt = std::make_shared<StmtBreakAST>();
			break;
		}
		case Token::Type::IF: {
			stmt = parse_if();
			break;
		}
		case Token::Type::FOR: {
			stmt = parse_for();
			break;
		}
		case Token::Type::LCURLY: {
			stmt = parse_stmt_block();
			break;
		}
		case Token::Type::IDENTIFIER: {
			requiring_delimiter = 1;
			// if we are dealing with an identifier, we could be doing various operations
			switch (peek(1).type) {
				case Token::Type::COLON:
				case Token::Type::QUICK_ASSIGN: stmt = parse_define(); break;
				default: stmt = parse_expression(); break;
			}
			break;
		}; // @TODO how do we know we are doing an assignment or expression?
		default: {
			requiring_delimiter = 1;
			stmt = parse_expression();
			break;
		}
	}
	// @TODO it doesnt work if we dont have a newline at the end
	// we use ; when we want multiple statements on one line
	if (requiring_delimiter && !(consume(Token::Type::SEMI_COLON) || consume(Token::Type::END))) {
		// @TODO this prev() stuff should probably be done with a function
		unit->error_handler.error("expected ; as statement delimiter",
			prev().index + prev().length + 1, prev().line, prev().index + prev().length + 1, prev().line);
		return std::make_shared<ErrorAST>();
	}
	return stmt;
}

std::shared_ptr<AST> Parser::parse_directive() {
	consume(Token::HASH);
	switch (next().type) {
		case Token::Type::RUN: {
				// create a new compilation unit to JIT the next statement
				auto stmt_to_jit = parse_stmt();
				kng_log("JITing {}", stmt_to_jit->to_json());
				// tmp
				return std::make_shared<ErrorAST>();
				break;
		}
		case Token::Type::INCLUDE: {
			//// @TODO allow expressions to be the filename so we can support #include (#run win ? "windows.k" : "unix.k")
			//auto to_include = parse_expression();
			//if (!(to_include->type() == AST::Type::EXPR_LIT)){
			//	unit->error_handler.error("expected string as filename to include",
			//		prev().index + prev().length + 1, prev().line, prev().index + prev().length + 1, prev().line);
			//	return std::make_shared<ErrorAST>();
			//}

			if (!expect(Token::Type::STRING)) {
				unit->error_handler.error("expected string as filename to include",
					prev().index, prev().line, prev().index + prev().length, prev().line);
				return std::make_shared<ErrorAST>();
			}
			auto path = next();
			auto valid_include = unit->importer->valid_include_path(unit->compile_file.file_path, path.value);
			if (!(valid_include==Importer::DepStatus::OK)) {
				std::string problem;
				switch (valid_include) {
				case Importer::DepStatus::CYCLIC_DEP: problem = "include path is causing a cyclic dependency"; break;
				}
				unit->error_handler.error(problem,
					prev().index, prev().line, prev().index + prev().length, prev().line);
				return std::make_shared<ErrorAST>();
			}
			// finally if we have't already included this file then include it!
			if (!unit->importer->already_included(unit->compile_file.file_path, path.value)) {
				// @TODO_URGENT add a way for the compilers Importer to track which CompilationUnits exist so we can get some stats
				auto compilation_unit = unit->importer->include(unit->compile_file.file_path, path.value);
				auto ast = compilation_unit->compile_to_ast();
			}
			break;
		}
	}
	return parse_stmt();
}

std::shared_ptr<AST> Parser::parse_if(){
	StmtIfAST if_ast;
	consume(Token::Type::IF);
	auto if_cond = parse_expression();
	auto if_stmt = parse_stmt();
	if(consume(Token::Type::ELSE)){
		if_ast.has_else = 1;
		if_ast.else_stmt = parse_stmt();
	}
	if_ast.if_cond = if_cond;
	if_ast.if_stmt = if_stmt;
	return std::make_shared<StmtIfAST>(if_ast);
}

std::shared_ptr<AST> Parser::parse_for(){
	StmtLoopAST loop_ast;
	consume(Token::Type::FOR);
	return std::make_shared<StmtLoopAST>(loop_ast);
}

// a stmt block is simply a list of statements.
// enter with { and exit with }
std::shared_ptr<AST> Parser::parse_stmt_block() {

	auto stmt_block = StmtBlockAST();

	consume(Token::Type::LCURLY, "'{' expected");
	if (!consume(Token::Type::RCURLY)) {
		sym_table->enter_scope();
		while (!end_of_block()) {
			stmt_block.stmts.push_back(parse_stmt());
		}
		sym_table->pop_scope();
		consume(Token::Type::RCURLY, "'}' expected");
	}

	requiring_delimiter = 0;
	return std::make_shared<StmtBlockAST>(stmt_block);

}

u8 Parser::end_of_block() {
	return end() || (peek().type == Token::RCURLY || peek().type == Token::END);
}

u8 Parser::expecting_type() {
	return 
		expect(Token::Type::U0)
		|| expect(Token::Type::U8)
		|| expect(Token::Type::U16)
		|| expect(Token::Type::U32)
		|| expect(Token::Type::S32)
		|| expect(Token::Type::S64)
		|| expect(Token::Type::F32)
		|| expect(Token::Type::F64)
		|| expect(Token::Type::CHAR)
		|| expect(Token::Type::IDENTIFIER)
		|| expect(Token::Type::POINTER);
}

u8 Parser::expecting_expr(){
	// an expression can be the following (same as parsing a single!)
	// string, number, identifier, group, true, false, {
	return
		expect(Token::Type::STRING)
		|| expect(Token::Type::NUMBER)
		|| expect(Token::Type::IDENTIFIER)
		|| expect(Token::Type::LPAREN)
		|| expect(Token::Type::TRU)
		|| expect(Token::Type::FLSE)
		|| expect(Token::Type::LBRACKET)
		|| expect(Token::Type::LCURLY);
}

Type Parser::parse_type() {
	Type t;
	u8 ptr_indirection=0;
	while (consume(Token::Type::POINTER)){
		ptr_indirection += 1;
	}

	switch (next().type) {
	case Token::Type::U0: t = Type(Type::Types::U0); break;
	case Token::Type::U8: t = Type(Type::Types::U8); break;
	case Token::Type::U16: t = Type(Type::Types::U16); break;
	case Token::Type::U32: t = Type(Type::Types::U32); break;
	case Token::Type::S32: t = Type(Type::Types::S32); break;
	case Token::Type::S64: t = Type(Type::Types::S64); break;
	case Token::Type::F32: t = Type(Type::Types::F32); break;
	case Token::Type::F64: t = Type(Type::Types::F64); break;
	case Token::Type::CHAR: t = Type(Type::Types::CHAR); break;
	case Token::Type::IDENTIFIER: t = Type(Type::Types::INTERFACE); break;
	}
	// check if we are dealing with an array
	if (consume(Token::Type::LBRACKET)){
		if (expect(Token::Type::NUMBER)) {
			// known size and stack allocated
			Token size = next();
			t.is_arr = 1;
			t.arr_length = std::stoi(size.value);
		}
		else {
			// unknown size, we need to check during type checking if the array is initialised otherwise it is a pointer
			t.is_arr = 1;
			ptr_indirection++;
		}

		if(!consume(Token::Type::RBRACKET)){
			unit->error_handler.error("expected ] after array decleration",
				prev().index + prev().length + 1, prev().line, prev().index + prev().length + 1, prev().line);
			return Type(Type::Types::UNKNOWN);
		}
	}
	t.ptr_indirection = ptr_indirection;
	t.is_constant = parsing_constant_assignment;
	return t;
}

std::shared_ptr<AST> Parser::parse_define() {
	StmtDefineAST define_ast;
	define_ast.identifier = next();
	if(consume(Token::Type::QUICK_ASSIGN)){
		parsing_variable_assignment = 1;
		// at this point, we are expecting to parse an expression as without an expression the syntax is invalid

		define_ast.requires_type_inference = 1;
		// @TODO check an expression exists???
		define_ast.value = parse_expression();
		define_ast.is_initialised = 1;
		parsing_variable_assignment = 0;
	}
	else if(consume(Token::Type::COLON)){
		if (expecting_type()) {
			// if we have reached here, we are either dealing with x : u8, x : u8 = 1, x : u8 1
			// variable decleration, variable decleration & assignment or constant decleration
			define_ast.define_type = parse_type();
			define_ast.value = NULL;

			u8 expecting_expression = expecting_expr();
			u8 parsing_constant = expecting_expression;
			define_ast.is_constant = parsing_constant;
			parsing_constant_assignment = parsing_constant;
			if (expecting_expression || consume(Token::Type::ASSIGN)) {
				parsing_variable_assignment = 1;
				define_ast.is_initialised = 1;
				define_ast.value = parse_expression();
				parsing_variable_assignment = 0;
			}
			if (parsing_constant_assignment) parsing_constant_assignment = 0;
		}
		else {
			// if we reached here we MUST expect an inferred constant e.g. x : 1
			if (!expecting_expr()) {
				unit->error_handler.error("expected type after :",
					prev().index + prev().length + 1, prev().line, prev().index + prev().length + 1, prev().line);
				return std::make_shared<ErrorAST>();
			}
			parsing_constant_assignment = 1;
			define_ast.requires_type_inference = 1;
			define_ast.value = parse_expression();
			define_ast.is_initialised = 1;
			define_ast.is_constant = 1;
			parsing_constant_assignment = 0;
		}
	}
	return std::make_shared<StmtDefineAST>(define_ast);
}


std::shared_ptr<AST> Parser::parse_expression() { 
	return parse_assign();
}


std::shared_ptr<AST> Parser::parse_assign() { 
	auto higher_precedence = parse_pattern();
	if (consume(Token::ASSIGN)) {
		auto assign_value = parse_pattern();
		// we need to check if we are setting a variable, or an interface member
		switch (higher_precedence->type()) {
			case AST::ASTType::EXPR_INTER_GET: {
				auto member_get = std::dynamic_pointer_cast<ExprInterfaceGetAST>(higher_precedence);
				auto interface_value = member_get->value;
				auto member_token = member_get->member;
				return std::make_shared<StmtInterfaceAssignAST>(interface_value, member_token, assign_value);
			}
			default: {

				return std::make_shared<StmtAssignAST>(higher_precedence, assign_value);

				//@TODO use AST position information
				unit->error_handler.error("cannot assign to lhs", 0, 1, 0, 1);
				return std::make_shared<ErrorAST>();
			}
		}
	}
	return higher_precedence;
}

// e.g. x = 1, a, 2, b
// @TODO we need to be able to assign to patterns e.g. a, b, c = 1, 2, 3 so we need to swap the precedence
std::shared_ptr<AST> Parser::parse_pattern() {
	auto higher_precedence = parse_lor();
	// @TODO figure out how to check a pattern when no comma is used
	if (consume(Token::Type::COMMA)) {
		std::vector<std::shared_ptr<AST>> asts;
		asts.push_back(higher_precedence);
		while (!expect(Token::Type::END)) {
			asts.push_back(parse_lor());
			// consume comma if it exists e.g. a, b, c or a b c
			consume(Token::Type::COMMA);
		}
		auto pattern = ExprPatternAST(asts);
		return std::make_shared<ExprPatternAST>(pattern);
	}
	return higher_precedence;
}

std::shared_ptr<AST> Parser::parse_lor() { 
	auto higher_precedence = parse_land();
	return higher_precedence;
}
std::shared_ptr<AST> Parser::parse_land() {
	auto higher_precedence = parse_bor();
	return higher_precedence;
}
std::shared_ptr<AST> Parser::parse_bor() {
	auto higher_precedence = parse_band();
	return higher_precedence;
}
std::shared_ptr<AST> Parser::parse_band() {
	auto higher_precedence = parse_eq();
	return higher_precedence;
}
std::shared_ptr<AST> Parser::parse_eq() {
	auto higher_precedence = parse_comp();
	return higher_precedence;
}
std::shared_ptr<AST> Parser::parse_comp() {
	auto higher_precedence = parse_shift();
	return higher_precedence;
}
std::shared_ptr<AST> Parser::parse_shift() {
	auto higher_precedence = parse_pm();
	return higher_precedence;
}
std::shared_ptr<AST> Parser::parse_pm() {
	auto higher_precedence = parse_mdmr();
	if(expect(Token::Type::PLUS) || expect(Token::Type::MINUS)){
		auto op = next();
		auto rhs = parse_pm();
		auto pm = ExprBinAST(higher_precedence, rhs, op);
		return std::make_shared<ExprBinAST>(pm);
	}
	return higher_precedence;
}
std::shared_ptr<AST> Parser::parse_mdmr() {
	auto higher_precedence = parse_un();
	// @TODO need to lex /, // and ///
	if (expect(Token::Type::STAR) || expect(Token::Type::DIV) || expect(Token::Type::MOD)) {
		auto op = next();
		auto rhs = parse_mdmr();
		auto mdmr = ExprBinAST(higher_precedence, rhs, op);
		return std::make_shared<ExprBinAST>(mdmr);
	}
	return higher_precedence;
}

std::shared_ptr<AST> Parser::parse_un() {
	if (expect(Token::Type::POINTER) || expect(Token::Type::BANG) || expect(Token::Type::BAND)) {


		// @TODO optimise by having a for consume(POINTER) for example so we can chain operations to reduce recursiveness

		auto op = next();
		auto ast = parse_un();
		auto un = ExprUnAST(op, ast, ExprUnAST::Side::LEFT);
		return std::make_shared<ExprUnAST>(un);
	}
	return parse_cast();
}
std::shared_ptr<AST> Parser::parse_cast() {
	auto higher_precedence = parse_call();
	if (consume(Token::AS)) {
		// expecting type here
		if (!expecting_type()) {
			unit->error_handler.error("expected type after 'as', casting requires a type",
				prev().index + prev().length + 1, prev().line, prev().index + prev().length + 1, prev().line);
			return std::make_shared<ErrorAST>();
		}
		ExprCastAST c;
		c.to_type = parse_type();
		c.value = higher_precedence;
		return std::make_shared<ExprCastAST>(c);
	}
	return higher_precedence;
}
std::shared_ptr<AST> Parser::parse_call() {
	auto higher_precedence = parse_single();
	// check if next is var args
	if (consume(Token::Type::LPAREN)) {
		ExprCallAST call;
		call.callee = higher_precedence;
		if (!consume(Token::Type::RPAREN)) {
			call.args = parse_call();
			if (!consume(Token::Type::RPAREN)) {
				unit->error_handler.error("expected closing ')' after fn call",
					prev().index + prev().length + 1, prev().line, prev().index + prev().length + 1, prev().line);
				return std::make_shared<ErrorAST>();
			}
		}
		return std::make_shared<ExprCallAST>(call);
	}
	return higher_precedence;
}
std::shared_ptr<AST> Parser::parse_single(){ 
	// @TODO implement casts
	// @TODO implement groups e.g. (1+2) + (1+3)
	auto t = next();

	switch (t.type) {
		// array literal for now
		case Token::Type::LCURLY: {
			std::vector<std::shared_ptr<AST>> array_values;
			while (!expect(Token::Type::RCURLY)) {
				array_values.push_back(parse_single());
				if(!expect(Token::Type::RCURLY))
					consume(Token::Type::COMMA);
			}
			consume(Token::Type::RCURLY);
			return std::make_shared<ExprLiteralArrayAST>(Type(Type::Types::UNKNOWN), Type(Type::Types::UNKNOWN), array_values.size(), array_values);
		}
		case Token::Type::IDENTIFIER: {
			return std::make_shared<ExprVarAST>(t);
		}
		case Token::Type::NUMBER: {
			ExprLiteralAST lit_ast;
			lit_ast.t = Type(Type::Types::S32, 0);
			lit_ast.v.v.as_s32 = stoi(t.value);
			return std::make_shared<ExprLiteralAST>(lit_ast);
			break;
		}
		case Token::Type::STRING: {
			// @TODO arrays
			return std::make_shared<ExprLiteralAST>();
			break;
		}
		case Token::Type::TRU: {
			ExprLiteralAST lit_ast;
			lit_ast.t = Type(Type::Types::U8, 0);
			lit_ast.v.v.as_u8 = 1;
			return std::make_shared<ExprLiteralAST>(lit_ast);
		}
		case Token::Type::FLSE: {
			ExprLiteralAST lit_ast;
			lit_ast.t = Type(Type::Types::U8, 0);
			lit_ast.v.v.as_u8 = 0;
			return std::make_shared<ExprLiteralAST>(lit_ast);
		}
		case Token::Type::LPAREN: {

			// at this point, we are either parsing a group or a fn.
			// to tell the difference, if we consume a rparen we are obviously parsing a fn.
			// if not, we need to check what comes next, if it is a definition stmt, then we are parsing a fn,
			// if not then its a group

			// parsing fn
			if (consume(Token::Type::RPAREN)) {
				ExprFnAST fn_ast;
				FnSignature fn_sig;
				fn_ast.is_lambda = 1;
				// the fn can only be a lambda if it isn't being assigned to a constant
				// e.g. x : (){} is the only way a fn can be a lambda
				fn_ast.is_lambda = !parsing_constant_assignment;
				fn_sig.anonymous_identifier = "lambda";
				if (expecting_type()) {
					fn_sig.operation_types.push_back(parse_type());
					fn_sig.has_return = 1;
				}
				else
					fn_sig.operation_types.push_back(Type(Type::Types::U0));
				fn_ast.full_type = Type(Type::Types::FN, fn_sig);
				// we need to check if the fn has a body
				if (!consume(Token::Type::SEMI_COLON)) {
					fn_ast.has_body = 1;
					fn_ast.body = parse_stmt();
				}
				return std::make_shared<ExprFnAST>(fn_ast);
			}
			else {
				auto expression = parse_expression();
				if (!consume(Token::Type::RPAREN)) {
					unit->error_handler.error("expected ) as statement delimiter",
						prev().index + prev().length + 1, prev().line, prev().index + prev().length + 1, prev().line);
					return std::make_shared<ErrorAST>();
				}
				auto group = ExprGroupAST(expression);
				return std::make_shared<ExprGroupAST>(group);
			}
		}
		//case Token::Type::LBRACKET: return parse_interface();
	}
	unit->error_handler.error("unexpected code?",
		prev().index + prev().length + 1, prev().line, prev().index + prev().length + 1, prev().line);
	return std::make_shared<ErrorAST>(); 
}


std::shared_ptr<AST> Parser::parse_fn(){
	return NULL;
}

std::shared_ptr<AST> Parser::parse_interface() {


	ExprInterfaceAST expr_interface;

	expr_interface.anonymous_name = "tmp_anon_interface_name";

	consume(Token::LCURLY);

	// a list of definition
	while (!consume(Token::RCURLY)) {
		parse_stmt();
	}
	return std::make_shared<ExprInterfaceAST>(expr_interface);
}

