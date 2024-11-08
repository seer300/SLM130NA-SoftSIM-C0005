# -*- coding: utf-8 -*-

import os
import sys

AVAILABLE_SECTIONS = ['.text', '.data', '.bss', '.rodata']
AVAILABLE_SOURCE_FILE_EXTENSIONS = ['.c', '.s', '.S']


class CompileOption:

    def __init__(self, inherited=True):
        self.inherited = inherited
        self.rename_options = {}

    def add_rename_option(self, key, value):
        self.rename_options[key] = value

    @property
    def compile_options(self):
        return self.rename_options


def check_and_merge_compile_options(base_compile_option, sub_compile_option):
    base_compile_options = base_compile_option.compile_options
    sub_compile_options = sub_compile_option.compile_options
    base_keys = base_compile_options.keys()
    sub_keys = sub_compile_options.keys()

    merge_compile_option = CompileOption()
    for key in base_keys:
        if key in sub_keys:
            merge_compile_option.add_rename_option(
                key, sub_compile_options[key])
        else:
            merge_compile_option.add_rename_option(
                key, base_compile_options[key])
    for key in sub_keys:
        if key not in base_keys:
            merge_compile_option.add_rename_option(
                key, sub_compile_options[key])
    return merge_compile_option


def convert(src, dst):
    config_paths = []
    config_dict = {}
    with open(src, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.rstrip()
            if not line:
                continue
            temp_list = line.split(',')
            current_path = os.path.abspath(temp_list[0])
            config_paths.append(current_path)
            config_dict[current_path] = (
                bool(temp_list[1].upper() == 'TRUE'), temp_list[2:])
    if not config_paths:
        with open(dst, 'w', encoding='utf-8'):
            pass
        return
    config_paths.sort()
    processed_paths = set()
    res_dict = {}

    def dfs_convert(current_path):
        if current_path in processed_paths:
            return
        if os.path.isfile(current_path):
            file_extension = os.path.splitext(current_path)[-1]
            if file_extension not in AVAILABLE_SOURCE_FILE_EXTENSIONS:
                return
        if current_path in config_dict:
            current_compile_option = CompileOption(
                config_dict[current_path][0])
            for section_config in config_dict[current_path][1]:
                temp_list = section_config.split('=')
                if temp_list[0] in AVAILABLE_SECTIONS:
                    current_compile_option.add_rename_option(
                        temp_list[0], temp_list[1])
        else:
            current_compile_option = CompileOption()
        if current_compile_option.inherited:
            parent_path = os.path.dirname(current_path)
            if parent_path in res_dict:
                parent_compile_option = res_dict[parent_path]
                current_compile_option = check_and_merge_compile_options(
                    parent_compile_option, current_compile_option)
        processed_paths.add(current_path)
        res_dict[current_path] = current_compile_option
        if os.path.isdir(current_path):
            names = os.listdir(current_path)
            names.sort()
            for name in names:
                if name and name[0] == '.':
                    continue
                sub_path = os.path.join(current_path, name)
                dfs_convert(sub_path)

    for config_path in config_paths:
        dfs_convert(config_path)
    res_file_list = []
    for k, v in res_dict.items():
        if os.path.isfile(k):
            if (v.compile_options):
                temp_list = [k]
                for sk, sv in v.compile_options.items():
                    temp_list.append(f'{sk}={sv}')
                res_file_list.append(','.join(temp_list))
    with open(dst, 'w', encoding='utf-8') as f:
        f.write('\n'.join(res_file_list))


def main():
    original_section_config_file = sys.argv[1]
    dst_section_config_file = sys.argv[2]
    convert(original_section_config_file, dst_section_config_file)


if __name__ == '__main__':
    main()
