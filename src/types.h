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
	std::string anonymous_identifier;
	std::vector<Type> operation_types;
};

struct Pattern {

};

struct Type {

	const static char* debug_types[];

	enum Types{
		UNKNOWN,
		U0,
		U8,
		U16,
		U32,
		U64,
		S32,
		S64,
		F32,
		F64,
		CHAR,
		STRING,
		FN,
		INTERFACE,
		PATTERN,		// sequence of types
	};
	Types t = UNKNOWN;
	// e.g. ^u8
	u8 ref = 0;
	// e.g. u8[5]
	u8 arr = 0;
	u8 pattern = 0;
	u32 arr_length = 0;
	std::vector<Type> patterns;
	InterfaceSignature interface_signature;
	FnSignature fn_signature;

	Type(){}
	Type(Types t) : t(t){}
	Type(Types t, u8 ref) : t(t), ref(ref){}
	Type(Types t, u8 arr, u32 arr_length) : t(t), arr(arr), arr_length(arr_length) {}
	Type(Types t, u8 pattern, std::vector<Type> types) : t(t), pattern(pattern), patterns(patterns) {}
	Type(Types t, FnSignature fn_sig) : t(t), fn_signature(fn_sig) {}

	std::string to_json();

	// matches basic determines whether a type's type (e.g. u8, interface, etc) is the same
	u8 matches_basic(Type other);

	// matches deep determines whether a type's full type signature matches (e.g. do the members match etc)
	u8 matches_deep(Type other);
};

struct Value {
	union v {
		u8  as_u8;
		u16 as_u16;
		u32 as_u32;
		s32 as_s32;
		s64 as_s64;
		f32 as_f32;
		f64 as_f64;
	} v;
};

template<typename T>
struct SymTable {
	std::map<u32, std::unordered_map<std::string, T>> entries;
	u32 level = 0;

	SymTable(){}

	void add_symbol(std::string identifier, T entry) {
		entries[level][identifier] = entry;
	}
	T get_symbol(std::string identifier) {
		// @TODO all levels
		for (u32 i = level; i >= 0; i--) {
			if (this->entries[i].count(identifier) != 0)
				return entries[i][identifier];
		}
		return NULL;
	}
	void enter_scope() { level++; };
	void pop_scope() { level--; }
};

extern Type infer_type(std::shared_ptr<AST> ast);