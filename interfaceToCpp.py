import os

def html_to_cpp_string(file_path, header_file_path):
    try:
        with open(file_path, 'r') as file, open(header_file_path, 'w') as header_file:
            header_file.write('// Automatically generated file\n')
            header_file.write('#include <WebServer.h>;\n')
            header_file.write('String interface() {\n')
            header_file.write('    String htmlContent = "";\n')
            for line in file:
                escaped_line = line.replace("\"", "\\\"").replace("\n", "")
                header_file.write(f"    htmlContent += \"{escaped_line}\\n\";\n")
            header_file.write("    return htmlContent;\n")
            header_file.write("}")
    except IOError as e:
        print(f"Error: {str(e)}")

html_to_cpp_string(os.path.join("src", "interface.html"), os.path.join("src", "interface.h"))

