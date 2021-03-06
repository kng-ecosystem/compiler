/*
James Clarke - 2021
*/

#include "lexer.h"
#include "compiler.h"

Lexer::Lexer(std::string& file_contents, CompilationUnit* unit){
	this->m_unit = unit;
	this->m_src = file_contents;
}

TokenList Lexer::scan() {
	while (!end()) {
		// do stuff here
		skip_whitespace();
		auto current = next();
		switch (current) {
			// @TODO these are sequential i.e. \r\n is a newline
			case '\n': m_index = 0; m_line++; resetSavePoint(); break;
			case '\r': m_index = 0; m_line++; resetSavePoint(); break;
			case '#': token(Token::Type::HASH); break;
			case '@': token(Token::Type::DIRECTIVE); break;
			case '+': token(Token::Type::PLUS); break;
			case '-': token(Token::Type::MINUS); break;
			case '*': token(Token::Type::STAR); break;
			case '(': token(Token::Type::LPAREN); break;
			case ')': token(Token::Type::RPAREN); break;
			case '[': token(Token::Type::LBRACKET); break;
			case ']': token(Token::Type::RBRACKET); break;
			case '{': token(Token::Type::LCURLY); break;
			case '}': token(Token::Type::RCURLY); break;
			//case '_': token(Token::Type::UNDERSCORE); break;
			case ';': token(Token::Type::SEMI_COLON); break;
			case '^': token(Token::Type::POINTER); break;
			case ',': token(Token::Type::COMMA); break;

			case '&': decide(Token::Type::BAND, Token::Type::LAND); break;
			case '|': decide(Token::Type::BOR, Token::Type::LOR); break;
			case '!': decide(Token::Type::BANG, Token::Type::NEQ); break;
			case '=': decide(Token::Type::ASSIGN, Token::Type::EQUALS); break;
			case ':': decide(Token::Type::COLON, Token::Type::QUICK_ASSIGN); break;

			case '.': decide(Token::Type::DOT, Token::Type::DOUBLEDOT, Token::Type::TRIPLEDOT); break;
			case '>': decide(Token::Type::GREATER, Token::Type::GEQ, Token::Type::RSHIFT); break;
			case '<': decide(Token::Type::LESS, Token::Type::LEQ, Token::Type::LSHIFT); break;

			case '/': do_comment(); break;

			default: {
				if (is_letter(current) || current=='_') {
					do_word(current);
				}
				else if (is_digit(current)) {
					do_number(current);
				}else if (is_string(current)){
					do_string(current);
				}
			}
		}
	}

	token(Token::Type::END);
	return TokenList(m_tokens);
}

void Lexer::token(Token::Type type) {
	token(type, "");
}

void Lexer::token(Token::Type type, std::string value) {
	Token tok;
	// we subtract 1 because 1,1, is actually index 0 and line 0
	tok.m_index = this->m_index_save_point;
	tok.m_line = this->m_line_save_point;
	tok.m_length = this->m_index - this->m_index_save_point;
	tok.m_type = type;
	tok.m_value = value;


	auto pos = Token::Position(
		m_index_save_point,
		m_index,
		m_line_save_point,
		m_line);

	tok.m_position = pos;

	m_tokens.push_back(tok);
	resetSavePoint();
}

void Lexer::decide(Token::Type t1, Token::Type t2){
	// all double tokens end with '=' which is convenient!
	if (!end() && peek() == '=') {
		next();
		return token(t2);
	}
	if (!end() && prev()==peek()) {
		next();
		return token(t2);
	}
	else 
		token(t1);

}

// @TODO this doesn't work
void Lexer::decide(Token::Type t1, Token::Type t2, Token::Type t3){
	if (!end() && peek() != '=' && peek() != prev()) {
		return token(t1);
	}

	if (!end() && peek() == '=') {
		advance(1);
		return token(t2);
	}
	// e.g. >> or ||
	if (!end() && prev() == peek()) {
		advance(1);
		return token(t3);
	}
}

void Lexer::skip_whitespace(){
	while (peek() == ' ' || peek() == '\t') {
	    next();
		resetSavePoint();
	}
}

void Lexer::resetSavePoint() {
	m_index_save_point = m_index;
	m_line_save_point = m_line;
}

u8 Lexer::consume(char c) {
	if (peek() == c) {
		next();
		return 1;
	}
	return 0;
}

char Lexer::prev(){
	return peek(-1);
}

char Lexer::peek(){
	return m_src.at(m_current);
}

char Lexer::peek(u32 amount) {
	return m_src.at(m_current + amount);
}

char Lexer::peek_ahead() {
	return m_src.at(m_current + 1);
}

char Lexer::next(){
	return advance(1);
}

char Lexer::advance(u32 amount){
	m_index +=amount;
	char c = m_src.at(m_current);
	m_current += amount;
	return c;
}

u8 Lexer::end() {
	return m_current > m_src.size()-1 || m_src.size()==0;
}

u8 Lexer::is_letter(char c){
	return isalpha(c);
}

u8 Lexer::is_digit(char c){
	return isdigit(c);
}

u8 Lexer::is_string(char c) {
	return c == '"' || c == '\'';
}


u8 Lexer::check_keyword(std::string rest, Token::Type t) {
	int i = 0;
	for (i; i < rest.size() && !end(); i++)
		if (peek(i) != rest.at(i))
			return 0;
	if (!(m_current + i > m_src.length() - 1) && (is_letter(peek(i)) || is_digit(peek(i)) || peek(i) == '_'))
		return 0;
	advance((u32)rest.size());
	token(t);
	return 1;
}

void Lexer::do_word(char start){

	u8 found_keyword = 0;

	switch (start) {
		case 'a': {
			found_keyword = check_keyword("s", Token::Type::AS); break;
			if (!found_keyword) found_keyword = check_keyword("nd", Token::Type::LAND);
		}
		case 'b': found_keyword = check_keyword("reak", Token::Type::BREAK); break;
		case 'c': {
			found_keyword = check_keyword("har", Token::Type::CHAR);
			if(!found_keyword) found_keyword = check_keyword("ontinue", Token::Type::CONTINUE); 
			break;
		}
		case 'd': found_keyword = check_keyword("efer", Token::Type::DEFER); break;
		case 'e': found_keyword = check_keyword("lse", Token::Type::ELSE); break;
		case 'f': {
			found_keyword = check_keyword("32", Token::Type::F32);
			if (!found_keyword) found_keyword = check_keyword("64", Token::Type::F64);
			if (!found_keyword) found_keyword = check_keyword("or", Token::Type::FOR);
			if(!found_keyword) found_keyword = check_keyword("alse", Token::Type::FLSE);
			if (!found_keyword) found_keyword = check_keyword("n", Token::Type::FN);
			break;
		}
		case 'i': {
			found_keyword = check_keyword("f", Token::Type::IF); 
			if (!found_keyword) found_keyword = check_keyword("nterface", Token::Type::INTERFACE);
			if (!found_keyword) found_keyword = check_keyword("n", Token::Type::INN);
			if (!found_keyword) found_keyword = check_keyword("nclude", Token::Type::INCLUDE);
			if (!found_keyword) found_keyword = check_keyword("mport", Token::Type::IMPORT);
			break;
		}
		case 'm': {
			found_keyword = check_keyword("odule", Token::Type::MODULE); break;
		}
		case 'o': {
			found_keyword = check_keyword("r", Token::Type::LOR); break;
		}
		case 'r': {
			found_keyword = check_keyword("et", Token::Type::RETURN);
			if (!found_keyword) found_keyword = check_keyword("un", Token::Type::RUN); 
			break;
		}
		case 's': {
			found_keyword = check_keyword("8", Token::Type::S8);
			if (!found_keyword) found_keyword = check_keyword("32", Token::Type::S32);
			if (!found_keyword) found_keyword = check_keyword("65", Token::Type::S64);
			if (!found_keyword) found_keyword = check_keyword("tring", Token::Type::STRING);
			break;
		}
		case 't': {
			found_keyword = check_keyword("rue", Token::Type::TRU);
			if (!found_keyword) found_keyword = check_keyword("ype", Token::Type::TYPE);
			if (!found_keyword) found_keyword = check_keyword("ypeof", Token::Type::TYPEOF);
			break;
		}
		case 'u': {
			found_keyword = check_keyword("8", Token::Type::U8);
			if (!found_keyword) found_keyword = check_keyword("16", Token::Type::U16);
			if (!found_keyword) found_keyword = check_keyword("32", Token::Type::U32);
			if (!found_keyword) found_keyword = check_keyword("64", Token::Type::U64);
			if (!found_keyword) found_keyword = check_keyword("se", Token::Type::USE);
			break;
		}
		case 'x': found_keyword = check_keyword("or", Token::Type::LAND); break;
	}

	if (!found_keyword) {
		// get the first word character
		std::stringstream ss;
		ss << start;
		while (!end() && (is_letter(peek()) || is_digit(peek()) || peek() == '_'))
			ss << next();
		token(Token::Type::IDENTIFIER, ss.str());
	}
}

void Lexer::do_number(char start){
	std::stringstream ss;
	ss << start;
	while (!end() && (is_digit(peek()) || peek() == '.' || peek() == '_'))
		ss << next();
	token(Token::Type::NUMBER, ss.str());
}

void Lexer::do_string(char start) {
	std::stringstream ss;
	while (!end() && peek() != start) {
		auto c = next();
		// check for escape sequences
		if (c == '\\') {
			switch (next()) {
				case 'n':  {ss << '\n'; break;}
				case 'r':  {ss << '\r'; break;}
				case '\'': {ss << '\''; break;}
				case '\t': {ss << '\t'; break;}
			}
		}
		else {
			ss << c;
		}
	}
	// consume past the delimiter
	next();
	token(Token::Type::STRING_LIT, ss.str());
}

void Lexer::do_comment(){
	if (consume('/')) {
		while (peek() != '\n' && peek() != '\r')
			next();
	}
	else if (consume('*')) {
		start:
		while (!consume('*')) next();
		if (!consume('/')) {
			next();
			goto start;
		}
	}
	else {
		token(Token::Type::DIV);
	}
}

void Lexer::do_documentation(){
}


void Lexer::do_escape(){
}