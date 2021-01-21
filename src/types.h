/*
James Clarke - 2021
*/

#pragma once

#include <variant>
#include <memory>
#include <map>
#include "common.h"
#include "token.h"

struct Type;
struct AST;

struct Type {

	const static char* debug_types[];


	enum class Types : u8{
		UNKNOWN,
		TYPE,    
		// A type is used at compile time as a type alias
		// e.g. x : f32; y : x = 1.233;
		// At runtime, the type contains type information (made up by a structure)
		// e.g. x : type = f32; printf("%s\n", x.name); will print f32
		// e.g. x : type = f32; printf("%s\n", x.size); will print 32
		NAMESPACE,
		U0,
		U8,
		S8,
		U16,
		S16,
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
		PATTERN		// sequence of types
	};
	
	Types m_type = Types::UNKNOWN;
	// if this type is a generic in a fn e.g. (x : $generic){}, here x would have type generic.
	// the compiler will then resolve this generic type at compile time
	u8 m_is_generic = 0;

	u8 m_constant = 0;
	// e.g. ^^^u8
	u8 m_ptr_indirection = 0;
	// e.g. u8[5]
	u8 m_is_arr = 0;
	u8 m_is_pattern = 0;
	u8 m_is_constant_str; // for when we have ""?
	u32 m_arr_length = 0;
	u8 m_is_arg = 0;


	u8 m_is_constant = 0;
	u8 m_is_global = 0;



	
	/*
	
	NAMESPACES
	
	This exposes all the identifiers in a given namespace.
	e.g. for a file main.kng with 3 variables: a, b & c.
	The m_identifiers variable would contain a, b & c
	*/
	std::vector<Token> m_namespace_identifiers;

	/*
	
	INTERFACES
	
	This is used to define what matching behaviour the user is trying to implement.

	say we have 2 interfaces

	a : {
		x : u8;
	}

	b : {
		x : u8;
		y : u8;
	}

	if we did a.match(b, LOOSE), this would return true, as b implements all the members of a.
	however if we did a.match(b, EXACT), this would return false as b implements more members than a.

	*/
	enum class InterfaceMatchType {
		LOOSE,
		EXACT,
	};

	Token m_interface_anonymous_identifier;
	std::vector<Type> m_interface_members;

	u8 matches_interface(Type& other, InterfaceMatchType match_type) {
		if (match_type == InterfaceMatchType::LOOSE) {
			// iterate through our members and check if the other types implement them
			for (s32 i = 0; i < m_interface_members.size(); i++) {
				u8 found = 0;
				for (s32 j = 0; j < other.m_interface_members.size(); j++)
					if (m_interface_members.at(0).matches(other.m_interface_members.at(j)))
						found = 1;
				// if the other type doesn't implement it then return false
				if (!found)
					return 0;
			}
		}
		else if (match_type == InterfaceMatchType::EXACT) {
			if (other.m_interface_members.size() != m_interface_members.size())
				return 0;
			for (s32 i = 0; i < m_interface_members.size(); i++) {
				if (!m_interface_members.at(i).matches(other.m_interface_members.at(i)))
					return 0;
			}
		}
		return 1;
	}

	/*
	
	FUNCTION

	*/

	Token m_fn_anonymous_identifier;
	// index 0 is the return type
	std::vector<Type> m_fn_operation_types;
	u8 m_fn_has_return = 0;


	// unlike interfaces, functions can only match if they both have the same arguments and return types
	u8 matches_fn(Type& other) {
		if (m_fn_operation_types.size() != other.m_fn_operation_types.size())
			return 0;
		for (s32 i = 0; i < m_fn_operation_types.size(); i++)
			if (!m_fn_operation_types.at(i).matches(other.m_fn_operation_types.at(i)))
				return 0;
		return 1;
	}

	/*
	
	PATTERN

	*/

	std::vector<Type> m_pattern_types;
	
	u8 matches_pattern(Type& other) {
		if (m_pattern_types.size() != other.m_pattern_types.size())
			return 0;
		for (s32 i = 0; i < m_pattern_types.size(); i++)
			if (!m_pattern_types.at(i).matches(other.m_pattern_types.at(i)))
				return 0;
		return 1;
	}


	Type(){}
	Type(Types t) : m_type(t){}

	static Type create_basic(Type::Types t);
	static Type create_array(Type::Types t, u32 length);
	static Type create_fn(u8 has_return, std::vector<Type> op_types);
	static Type create_interface(std::vector<Type> member_types);
	static Type create_pointer(Type::Types t, u32 ptr_indirection);
	static Type create_pattern(std::vector<Type> types);

	std::string to_json();

	// matches basic determines whether a type's type (e.g. u8, interface, etc) is the same
	u8 matches(Type other);

	// this is for when we have different number types that the compiler tries to cast implicitly
	// an example is y : u8 = 1, by default integers are s32 and so the compiler tries to cast 1 to u8
	u8 can_niave_cast(Type other);
	void cast(Type other);
	u8 is_number_type();
	u8 is_integer_type();
	u8 is_float_type();
	u8 is_interface();
	u8 is_fn();
	u8 is_array();
	u8 is_pattern();
	u8 is_pointer();
};

struct Value {

	std::string value;
	u8 as_u8() { return std::atoi(value.c_str()); }
	s8 as_s8() { return std::atoi(value.c_str()); }
	u16 as_u16() { return std::atoi(value.c_str()); }
	s16 as_s16() { return std::atoi(value.c_str()); }
	u32 as_u32() { return std::atoi(value.c_str()); }
	s32 as_s32() { return std::atoi(value.c_str()); }
	s64 as_s64() { return std::atoi(value.c_str()); }
	f32 as_f32() { return std::atof(value.c_str()); }
	f64 as_f64() { return std::atof(value.c_str()); }
	char as_char() { return value.at(0); }
	std::string as_string() { return value; }
};

struct SymTableEntry {
	void* optional_data;
	Type* type;
	u8 is_global = 0;
	u8 is_constant = 0;

	SymTableEntry(){}
	SymTableEntry(void* optional_data, Type* type, u8 is_global, u8 is_constant) 
		: optional_data(optional_data), type(type), is_global(is_global), is_constant(is_constant){}
};

template<typename T>
struct SymTable {
	// keep track of the latest entry
	std::pair<Token, SymTableEntry> latest_entry;
	std::map<s32, std::map<Token, SymTableEntry>> entries;
	s32 level = 0;

	SymTable(){}


	void dump() {
		kng_warn("dumping symtable at level {}", level);
		for (s32 i = level; i >= 0; i--) {
			kng_warn("level {}", i);
			for (auto& [key, value] : entries[i]) {
				kng_warn("level: {}, k: {}", i, key);
			}
		}
	}
	void add_symbol(Token entry_id, SymTableEntry entry = SymTableEntry()) {
		latest_entry = std::pair<Token, SymTableEntry>(entry_id, entry);
		entries[level][entry_id] = entry;
	}

	void set_symbol(Token entry_id, SymTableEntry entry) {
		latest_entry = std::pair<Token, SymTableEntry>(entry_id, entry);
		entries[level][entry_id] = entry;
	}

	u8 contains_symbol(Token entry_id) {
		for (s32 i = level; i >= 0; i--) {
			if (this->entries[i].count(entry_id) > 0) {
				return 1;
			}
		}
		return 0;
	}

	SymTableEntry get_symbol(Token entry_id) {
		for (s32 i = level; i >= 0; i--) {
			if (this->entries[i].count(entry_id) > 0) {
				return entries[i][entry_id];
			}
		}
		return SymTableEntry();
	}
	void enter_scope() { level++; };
	void pop_scope() { 
		// @TODO this doesn't actually clear the symbols at the current scope and causes
		// a "sym already defined" error, to fix clear the scope
		entries[level].clear();
		level--; 
	}
};

extern Type infer_type(std::shared_ptr<AST> ast);
extern Type niavely_cast_to_master_type(Type* lhs, Type* rhs);