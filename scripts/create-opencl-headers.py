import os
import sys

header_file_fmt = "{name}_ocl.hpp"
header_string = (
"#ifndef {definition}_OCL_HPP\n"
"#define {definition}_OCL_HPP\n"
"#include <opencv2/core/ocl.hpp>\n"
"const cv::ocl::ProgramSource& {module}_{name}_ocl() {{\n"
"static cv::ocl::ProgramSource source(\"{module}\", \"{name}\", \"{kernel}\", \"\");\n"
"return source;\n"
"}}\n"
"#endif\n"
)

def clear_between(string, del1, del2):
	pos1 = string.find(del1)
	if pos1 < 0:
		return string
	pos2 = string[pos1:].find(del2) + pos1
	if pos2 < 0:
		return string
	return string.replace(string[pos1:pos2+len(del2)], "")

def clear_all(string, del1, del2):
	while True:
		cleared = clear_between(string, del1, del2)
		if string == cleared:
			return string
		string = cleared

def clear_repeating(string, tok):
	while True:
		cleared = string.replace(tok+tok, tok)
		if string == cleared:
			return string
		string = cleared

def compress(code):
	code = clear_all(code, "/*", "*/")
	code = clear_all(code, "//", "\n")
	code = code.replace("\n", "\\n")
	code = code.replace("\t", "")
	code = code.replace("\"", "\\\"")
	code = clear_repeating(code, " ")
	code = clear_repeating(code, "\\n")
	return code

def create_header_file(kernel_path, header_path):
	with open(kernel_path) as fp:
		kernel = compress(fp.read())
	base = os.path.splitext(os.path.basename(kernel_path))[0]
	module, name = base.split("_")
	data = header_string.format(
		definition=base.upper(),
		module=module,
		name=name,
		kernel=kernel
	)
	with open(header_path, "w") as fp:
		fp.write(data)

def create_headers(kernel_dir, header_dir):
	for kernel_file in os.listdir(kernel_dir):
		kernel_path = os.path.join(kernel_dir, kernel_file)
		if os.path.isfile(kernel_path) and kernel_file.endswith(".cl"):
			header_file = header_file_fmt.format(name=os.path.splitext(kernel_file)[0])
			header_path = os.path.join(header_dir, header_file)
			create_header_file(kernel_path, header_path)
			print("-- Created {}".format(header_file))

if __name__ == "__main__":
	if len(sys.argv) != 3:
		print("Usage: {} <kernel_dir> <header_dir>".format(sys.argv[0]))
		sys.exit(1)
	os.makedirs(sys.argv[2], exist_ok=True)
	create_headers(sys.argv[1], sys.argv[2])
