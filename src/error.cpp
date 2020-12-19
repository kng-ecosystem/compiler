#include "error.h"
#include "compiler.h"

std::string get_src_at_line(const std::string& src, u32 line) {
	auto src_lines = split_string_by_newline(src);
	return src_lines.at(line - 1);
}

std::vector<std::string> split_string_by_newline(const std::string& str){
	auto result = std::vector<std::string>{};
	auto ss = std::stringstream{ str };

	for (std::string line; std::getline(ss, line, '\n');)
		result.push_back(line);

	return result;
}

// select an area of the original src 
std::string select_area(std::string& original, u32 to, u32 from){
	return original.substr(to, from);
}

// used to select the area that is the problem
std::string select_problem_area(std::string& original, u32 p_start_index, u32 p_start_line, u32 p_end_index, u32 p_end_line) {
	std::stringstream ss;

	auto lines = split_string_by_newline(original);
	u32 min_line = (p_start_line - 1) >= 1 ? p_start_line - 1 : 1;
	u32 max_line = (p_start_line + 1) <= lines.size() ? p_start_line + 1 : p_start_line;

	for (int i = min_line; i <= max_line; i++)
		ss << lines.at(i-1) << "\n";
	return ss.str();
}

std::string build_pointer(u32 index){
	std::stringstream ss;
	for (u32 i = 0; i < index-1; i++)
		ss << " ";
	ss << "^";
	return ss.str();
}

void ErrorHandler::error(
	const std::string problem, u32 p_start_index, u32 p_start_line, u32 p_end_index, u32 p_end_line
) {


	auto problem_string = select_problem_area(
		compiler->compile_file.file_contents, p_start_index, p_start_line, p_end_index, p_end_line
		);

	problem_string = get_src_at_line(compiler->compile_file.file_contents, p_start_line);

	// @TODO calculate where the start of the line is on the error line
	warn("");
	warn("error on index {} line {} in {}", p_start_index, p_start_line, compiler->compile_file.filename);
	warn("");
	warn("{}", problem_string);
	warn("{}", build_pointer(p_end_index));
	warn("{}", problem);
	warn("");
}