import os

Import("env")

# Install custom packages from the PyPi registry
env.Execute("$PYTHONEXE -m pip install htmlmin")

import htmlmin


def read_file(file):
    with open(file) as f:
        return f.readlines()


def write_file(file, content):
    with open(file, "w") as f:
        f.write(content)


def pre_process_html(html, css):
    processed = ""

    for line in html:
        line = line.replace('<link rel="stylesheet" href="style.css">',
                            f'<style>{css}</style>')

        for i in range(0, len(line)):
            char = line[i]

            if char == "\r" or char == "\n":
                continue
            elif char == "%" and line[i + 1] == ";":
                processed += "%%"
            elif char == "\t":
                processed += " "
            else:
                processed += char
    return processed


def post_process_html(html):
    processed = ""
    for i in range(0, len(html)):
        char = html[i]

        if i == 0 or not (processed[len(processed) - 1] == ' ' and char == ' '):
            processed += char

    return processed


def main():
    header_path = "src/generated/html"
    os.makedirs(header_path, exist_ok=True)

    html_path = "src/network/html"
    html_files = os.listdir(html_path)
    html_files = [os.path.join(html_path, file) for file in html_files]

    css_path = os.path.join(html_path, "style.css")
    css = " ".join(read_file(css_path))

    for file in html_files:
        if not file.endswith("html"):
            continue

        text = read_file(file)
        pre_processed = pre_process_html(text, css)
        html = htmlmin.minify(pre_processed, remove_comments=True,
                              remove_empty_space=True,
                              remove_all_empty_space=True,
                              reduce_empty_attributes=True,
                              reduce_boolean_attributes=True,
                              remove_optional_attribute_quotes=False)
        post_processed = post_process_html(html)
        file_name = os.path.basename(file).replace(".html", "")
        header_filename = file_name + ".hpp"
        header_content = f'static constexpr char HTML_{file_name.upper()}' \
                         f'[] PROGMEM = R"({post_processed})";'

        write_file(os.path.join(header_path, header_filename), header_content)


main()
