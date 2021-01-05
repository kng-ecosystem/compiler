/*
James Clarke - 2021
*/

#include <fstream>
#include <sstream>
#include "compiler.h"
#include "error.h"
#include "lexer.h"
#include "parser.h"
#include "types.h"
#include "generator.h"
#include "typechecking.h"

CompilationUnit::CompilationUnit(CompileFile compile_file, Compiler* compiler)
	: compile_file(compile_file), compiler(compiler), importer(&compiler->importer), compile_options(compiler->options){
	this->error_handler = ErrorHandler(this);
}

CompileFile::CompileFile(std::string& path) {
	this->file_path = path;
	// open a test file
	std::ifstream f;
	f.open(path);
	std::stringstream buffer;
	buffer << f.rdbuf();
	file_contents = buffer.str();
}

void Compiler::compile(std::string& path, CompileOptions options) {

	this->options = options;

	kng_log("kng compiler v0_1");

	auto t1 = std::chrono::high_resolution_clock::now();

	importer = Importer(this);

	auto unit = importer.include(path, path);
	unit->compile();

	auto t2 = std::chrono::high_resolution_clock::now();
	auto time = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
	kng_log("=============== SUCCESS :) =================");
	kng_log("compiled {} file(s) and {} line(s) in {} ms.", importer.n_units, importer.n_lines, time);
	kng_log("============================================");
}

u8 CompilationUnit::compile() {
	compile_to_bin();
	return 1;
}

TokenList CompilationUnit::compile_to_tokens() {
	// lexical analysis
	Lexer l(compile_file.file_contents, this);
	auto tokens = l.scan();
	if (compile_options.debug_emission_flags & EMIT_TOKEN_DEBUG)
		kng_log("lexer debug {}:\n{}", compile_file.file_path, tokens.to_json());
	return tokens;
}

std::shared_ptr<AST> CompilationUnit::compile_to_ast() {
	auto tokens = compile_to_tokens();
	// parsing to an ast
	Parser p(tokens, this);
	auto ast = p.parse();

	TypeChecker t(ast, this);
	ast = t.check();

	if (compile_options.debug_emission_flags & EMIT_AST_DEBUG)
		kng_log("parser debug {}:\n{}", compile_file.file_path, ast->to_json());
	return ast;
}

void CompilationUnit::compile_to_bin() {
	auto ast = compile_to_ast();
	if (error_handler.how_many>0)
		return;
	LLVMCodeGen generator(ast, this);
	generator.generate();
}

u8 Importer::valid_import_path(std::string& path) {
	// (for each of these check if the path contains a main.kng)

	// first check the path relative to the current file

	// then check the path relative to the kng install e.g. c:/kng/lib/examples/

	// then check the internet
	// e.g. #include "https://www.github.com/kng/lib/examples/"

	// then check absolute
	return 1;
}

Importer::DepStatus Importer::valid_include_path(std::string& current_path, std::string& path) {
	// we need to check for circular dependencies
	// first get the target dependencies' dependencies
	auto to_imports_dependencies = unit_dependencies[path];
	// then check if we are inside that
	if (std::find(to_imports_dependencies.begin(), to_imports_dependencies.end(), current_path) != to_imports_dependencies.end())
		return DepStatus::CYCLIC_DEP;

	// first check the path relative to the current file

	// then check the path relative to the kng install e.g. c:/kng/lib/...

	// then check the internet
	// e.g. #include "https://www.github.com/kng/lib/example.kng"

	// then check absolute

	return DepStatus::OK;
}

u8 Importer::already_included(std::string& current_path, std::string& path) {
	return units.count(path)>0;
}

std::shared_ptr<CompilationUnit> Importer::import(std::string& path) {
	CompileFile f(path);
	auto unit = std::make_shared<CompilationUnit>(f, compiler);
	return unit;
}

std::shared_ptr<CompilationUnit> Importer::include(std::string& current_path, std::string& path) {
	CompileFile f(path);
	auto unit = std::make_shared<CompilationUnit>(f, compiler);
	n_units++;
	n_lines += count_lines(f.file_contents);
	units[path] = unit;
	// get the current unit
	unit_dependencies[current_path].push_back(path);
	return unit;
}