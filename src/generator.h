/*
James Clarke - 2021
*/

#pragma once

#include <memory>

#include "common.h"
#include "ast.h"

struct CompilationUnit;

struct CodeGen : public ASTVisitor {


	enum class GeneratorResult {
		SUCCESS,
		FAIL,
	};

	GeneratorResult m_result;

	virtual void generate() = 0;
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
	virtual void* visit_expr_fn_ast(ExprFnAST* expr_fn_ast) = 0;
	virtual void* visit_expr_cast_ast(ExprCastAST* expr_cast_ast) = 0;
	virtual void* visit_expr_var_ast(ExprVarAST* expr_var_ast) = 0;
	virtual void* visit_expr_interface_get_ast(ExprGetAST* expr_interface_get_ast) = 0;
	virtual void* visit_expr_bin_ast(ExprBinAST* expr_bin_ast) = 0;
	virtual void* visit_expr_un_ast(ExprUnAST* expr_un_ast) = 0;
	virtual void* visit_expr_group_ast(ExprGroupAST* expr_group_ast) = 0;
	virtual void* visit_expr_literal_ast(ExprLiteralAST* expr_literal_ast) = 0;
	virtual void* visit_expr_literal_array_ast(ExprLiteralArrayAST* expr_literal_array_ast) = 0;

};

#include "llvm/Support/TargetSelect.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/IR/BasicBlock.h"

struct LLVMCodeGen : public CodeGen {


	enum class FetchedType {
		FN,
		VARIABLE,
		VALUE,
	};


	struct LLVMSymEntry {
		void* llvm_instruction;
		Type type;
	};


	std::shared_ptr<AST> m_ast;
	CompilationUnit* m_unit;
	std::unique_ptr<llvm::LLVMContext> m_context;
	std::unique_ptr<llvm::IRBuilder<>> m_builder;
	std::shared_ptr<llvm::Module> m_module;

	// every time we visit an expression, we need to set this type so we can convert it to a value
	FetchedType m_fetched_type;
	llvm::Value* m_fetched_value;
	// set the exit block when entering a new block
	llvm::BasicBlock* m_exit_block;

	// used for types and instructions
	SymTable<LLVMSymEntry> m_sym_table;
	LLVMCodeGen(){}
	LLVMCodeGen(std::shared_ptr<AST> ast, CompilationUnit* unit) : m_ast(ast), m_unit(unit) {}
	virtual void generate();
	void make_runtime();
	void optimise();
	llvm::Type* convert_type(Type type);
	// call this if we need the fetched_value as a value ready to be used.
	// e.g. convert StoreInstr to LoadInstr to use a variable
	llvm::Value* convert_fetched_to_value();
	virtual void* visit_program(ProgramAST* program_ast);
	virtual void* visit_stmt_block(StmtBlockAST* stmt_block_ast);
	virtual void* visit_stmt_expression(StmtExpressionAST* stmt_expression_ast);
	virtual void* visit_stmt_define(StmtDefineAST* stmt_define_ast);
	virtual void* visit_stmt_interface_define(StmtInterfaceDefineAST* stmt_interface_define_ast);
	virtual void* visit_stmt_assign(StmtAssignAST* stmt_assign_ast);
	virtual void* visit_stmt_interface_assign(StmtInterfaceAssignAST* stmt_interface_assign_ast);
	virtual void* visit_stmt_return(StmtReturnAST* stmt_return_ast);
	virtual void* visit_stmt_continue_ast(StmtContinueAST* stmt_continue_ast);
	virtual void* visit_stmt_break_ast(StmtBreakAST* stmt_break_ast);
	virtual void* visit_stmt_if_ast(StmtIfAST* stmt_if_ast);
	virtual void* visit_stmt_loop_ast(StmtLoopAST* stmt_loop_ast);
	virtual void* visit_expr_inter_ast(ExprInterfaceAST* expr_interface_ast);
	virtual void* visit_expr_fn_ast(ExprFnAST* expr_fn_ast);
	virtual void* visit_expr_cast_ast(ExprCastAST* expr_cast_ast);
	virtual void* visit_expr_call_ast(ExprCallAST* expr_call_ast);
	virtual void* visit_expr_var_ast(ExprVarAST* expr_var_ast);
	virtual void* visit_expr_interface_get_ast(ExprGetAST* expr_interface_get_ast);
	virtual void* visit_expr_bin_ast(ExprBinAST* expr_bin_ast);
	virtual void* visit_expr_un_ast(ExprUnAST* expr_un_ast);
	virtual void* visit_expr_group_ast(ExprGroupAST* expr_group_ast);
	virtual void* visit_expr_literal_ast(ExprLiteralAST* expr_literal_ast);
	virtual void* visit_expr_literal_array_ast(ExprLiteralArrayAST* expr_literal_array_ast);
};

/*
struct CLRCodeGen;

struct Generator {
	std::shared_ptr<AST> ast;
	CompilationUnit* unit;
	Generator(){}
	Generator(std::shared_ptr<AST> ast, CompilationUnit* unit) : ast(ast), unit(unit){}
	virtual std::shared_ptr<CodeGen> generate() = 0;
};



#include <llvm/Support/TargetSelect.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
struct LLVMGenerator : public Generator {
	std::unique_ptr<llvm::LLVMContext> llvm_context;
	std::unique_ptr<llvm::IRBuilder<>> llvm_builder;
	std::shared_ptr<llvm::Module> llvm_module;
	LLVMGenerator(std::shared_ptr<AST> ast, CompilationUnit* unit) : Generator(ast, unit) {}
	virtual std::shared_ptr<CodeGen> generate();
};

struct CLRGenerator : public Generator {
	CLRGenerator(std::shared_ptr<AST> ast, CompilationUnit* unit) : Generator(ast, unit) {}
	virtual std::shared_ptr<CodeGen> generate();
};
*/