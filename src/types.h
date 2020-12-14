#pragma once

#include <memory>
#include <map>
#include "common.h"

struct Type;
struct AST;

// the signature of an interface is made up of the type of its members
struct InterfaceSignature{
	std::vector<Type> members;
};

struct FnSignature {
	// operation types are the types of the params and the return value
	std::vector<Type> operation_types;
};

struct Type {
	enum Types{
		U8,
		U16,
		U32,
		U64,
		I32,
		I64,
		F32,
		F64,
		CHAR,
		STRING,
		INTERFACE,
		SEQ,		// sequence of types
		ANY,		// _
	};

	Types t;
	InterfaceSignature interface_signature;
	FnSignature fn_signature;

	Type(){}
	Type(Types t) : t(t){}

	// matches basic determines whether a type's type (e.g. u8, interface, etc) is the same
	u8 matches_basic(Type other);
	// matches deep determines whether a type's full type signature matches (e.g. do the members match etc)
	u8 matches_deep(Type other);
};

struct SymTable;

struct SymTableEntry {

	enum EntryType {
		TYPE,
		SYM_TABLE,
	};

	u8 entry_type;
	// @TODO these 2 should be in a union
	Type type;
	std::shared_ptr<SymTable> sym_table;
};

struct SymTable {
	std::unordered_map<std::string, SymTableEntry> entries;
};

inline extern Type u8_type();
extern Type infer_type(std::shared_ptr<AST> ast);