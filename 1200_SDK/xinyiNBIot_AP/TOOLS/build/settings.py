# -*- encoding: utf-8 -*-

import os
import sys

import json
import copy
import shutil

AVAILABLE_SECTIONS = ['.text', '.data', '.bss', '.rodata']
AVAILABLE_SOURCE_FILE_EXTENSIONS = ['.c', '.s', '.S']


class basicSettings:

    def __init__(self):
        pass


class compileSettings:

    __slots__ = ('path', 'mode', 'name', 'children',
                 'excludes', 'delete', 'full_path')

    def __init__(self, path, mode, name, delete, full_path):
        self.path = path.replace('\\', '/')
        self.mode = mode
        self.name = name  # used as library name, only available when mode is library
        self.delete = delete  # delete source after build
        self.full_path = full_path.replace('\\', '/')
        self.children = []
        self.excludes = []

    def add_child(self, child):
        self.children.append(child)

    def add_exclude(self, exclude):
        self.excludes.append(exclude)

    def __repr__(self):
        return f'path:{self.path}, full_path:{self.full_path}, mode:{self.mode}, children:{self.children}, excludes:{self.excludes}'


class compileSectionSettings:

    def __init__(self, path, full_path, inherited=True):
        self.path = path
        self.full_path = full_path
        self.inherited = inherited
        self.children = []
        self.sections = {}

    def add_child(self, child):
        self.children.append(child)

    def add_section(self, name, value):
        if not name in AVAILABLE_SECTIONS:
            return
        self.sections[name] = value

    def __repr__(self):
        return f'path:{self.path}, full_path:{self.full_path}, inherited:{self.inherited}, children:{self.children}, sections:{self.sections}'


class buildSettings:

    __slots__ = ('basic_settings', 'compile_settings',
                 'compile_section_settings')

    def __str__(self):
        return f'basic_settings:{self.basic_settings}, compile_settings:{self.compile_settings}, compile_section_settings:{self.compile_section_settings}'


class projectSettings:

    __slots__ = ('project', 'version', 'build_settings')

    def __str__(self):
        return f'project:{self.project}, version:{self.version}, build_settings:{self.build_settings}'


class CompileOption:

    def __init__(self, inherited=True):
        self.inherited = inherited

        self.rename_options = {}

    def add_rename_option(self, key, value):
        self.rename_options[key] = value

    @property
    def compile_options(self):
        return self.rename_options

    def __repr__(self):
        ret_str = ''
        for k, v in self.rename_options.items():
            ret_str += '{0}={1},'.format(k, v)
        if self.inherited:
            ret_str += 'inherited:true'
        else:
            ret_str += 'inherited:false'
        return ret_str


class Item:

    def __init__(self, path, compile_option=None):
        self.path = path
        self.name = os.path.basename(os.path.abspath(path))
        self.compile_option = compile_option

    def has_child(self):
        return False

    def __repr__(self):
        return 'path:{0}, name:{1}, compile_option:{2}'.format(self.path, self.name, self.compile_option)


class FileItem(Item):

    def __init__(self, path):
        super().__init__(path)


class DirItem(Item):

    def __init__(self, path):
        super().__init__(path)
        self.children = []

    def add_child(self, child):
        self.children.append(child)

    def has_child(self):
        return len(self.children) > 0


class XYCompileOptionsCollector:

    def __init__(self, base_dir):
        self.base_dir = base_dir

    def check_duplicate_compile_option(self, base_compile_option, sub_compile_option):
        if sub_compile_option is None:
            return True
        if sub_compile_option.text != None and sub_compile_option.text != base_compile_option.text:
            return False
        if sub_compile_option.rodata != None and sub_compile_option.rodata != base_compile_option.text:
            return False
        if sub_compile_option.data != None and sub_compile_option.data != base_compile_option.data:
            return False
        if sub_compile_option.bss != None and sub_compile_option.bss != base_compile_option.bss:
            return False
        return True

    def check_and_merge_compile_options(self, sub_item, base_compile_option, sub_compile_option):
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
        sub_item.compile_option = merge_compile_option

    def do_generate_file_structure(self, path, parent_item, compile_section_settings_list, result_dict):
        compile_option_dict = {}
        for compile_section_settings in compile_section_settings_list:
            inherited = compile_section_settings.inherited
            compile_option = CompileOption(inherited)
            for k, v in compile_section_settings.sections.items():
                if k != v:
                    compile_option.add_rename_option(k, v)
            compile_option_dict[compile_section_settings.path] = (
                compile_option, compile_section_settings)

        names = os.listdir(path)
        names.sort()

        for name in names:
            if name and name[0] == '.':
                continue
            child_path = os.path.join(path, name)
            if os.path.isdir(child_path):
                sub_item = DirItem(child_path)
                if name in compile_option_dict:
                    compile_option, compile_section_settings = compile_option_dict[name]
                    if compile_option.inherited:
                        self.check_and_merge_compile_options(
                            sub_item, parent_item.compile_option, compile_option)
                    else:
                        sub_item.compile_option = compile_option
                    self.do_generate_file_structure(
                        child_path, sub_item, compile_section_settings.children, result_dict)
                else:
                    sub_item.compile_option = copy.copy(
                        parent_item.compile_option)
                    self.do_generate_file_structure(
                        child_path, sub_item, [], result_dict)
            elif os.path.isfile(child_path):
                file_extension = os.path.splitext(child_path)[-1]
                if file_extension in AVAILABLE_SOURCE_FILE_EXTENSIONS:
                    sub_item = FileItem(child_path)
                    if name in compile_option_dict:
                        compile_option, compile_section_settings = compile_option_dict[name]
                        if compile_option.inherited:
                            self.check_and_merge_compile_options(
                                sub_item, parent_item.compile_option, compile_option)
                        else:
                            sub_item.compile_option = compile_option
                    else:
                        sub_item.compile_option = copy.copy(
                            parent_item.compile_option)
                    if sub_item.compile_option.compile_options:
                        temp_dict = {}
                        for k, v in sub_item.compile_option.compile_options.items():
                            temp_dict[k] = v
                        result_dict[child_path] = temp_dict

    def start_collecting(self, prj_settings):
        root_item = DirItem(self.base_dir)
        root_item.compile_option = CompileOption()
        result_dict = {}
        self.do_generate_file_structure(
            self.base_dir, root_item, prj_settings.build_settings.compile_section_settings.children, result_dict)
        return result_dict


class XYSettingsHandler:

    def __init__(self, base_dir, settings_file, output_dir):
        self.base_dir = os.path.abspath(base_dir)
        self.settings_file = settings_file
        self.output_dir = os.path.abspath(output_dir)
        if not os.path.exists(self.output_dir):
            os.makedirs(self.output_dir)
        self.my_compile_options_collector = XYCompileOptionsCollector(
            self.base_dir)
        self.cmake_template_file_path = os.path.join(
            self.base_dir, r'TOOLS/cmake/template/CMakeLists.txt')
        self.cmake_prj_settings_file_path = os.path.join(
            self.base_dir, r'TOOLS/cmake/prj_settings.cmake')
        # self.cmake_subdirs_file_path = os.path.join(
        #     self.base_dir, r'TOOLS/cmake/subdirs.cmake')

    def parse(self):
        with open(self.settings_file, 'r') as f:
            json_data = json.load(f)
            # print(json_data)

        def construct_basic_settings(basic_settings_dict):
            if not basic_settings_dict:
                return None
            basic_settings = basicSettings()
            return basic_settings

        def construct_compile_settings(compile_settings_dict, parent_path):
            if not compile_settings_dict:
                return None
            path = compile_settings_dict.get('path', '.').replace('\\', '/')
            mode = compile_settings_dict.get('mode', 'normal')
            name = compile_settings_dict.get('name', path)
            delete = compile_settings_dict.get('delete', False)
            cur_path = os.path.abspath(os.path.join(parent_path, path))
            compile_settings = compileSettings(
                path, mode, name, delete, cur_path)
            children_settings = compile_settings_dict.get('children', [])
            for child_settings in children_settings:
                child = construct_compile_settings(child_settings, cur_path)
                compile_settings.add_child(child)
            excludes = compile_settings_dict.get('excludes', [])
            for exclude in excludes:
                compile_settings.add_exclude(exclude.replace('/', '\\'))
            return compile_settings

        def construct_compile_section_settings(compile_section_settings_dict, parent_path):
            if not compile_section_settings_dict:
                return None
            path = compile_section_settings_dict.get('path', '.')
            inherited = compile_section_settings_dict.get('inherited', True)
            cur_path = os.path.join(parent_path, path)
            compile_section_settings = compileSectionSettings(
                path, cur_path, inherited)
            sections_settings = compile_section_settings_dict.get(
                'sections', {})
            for k, v in sections_settings.items():
                compile_section_settings.add_section(k, v)
            children_settings = compile_section_settings_dict.get(
                'children', [])
            for child_settings in children_settings:
                child = construct_compile_section_settings(
                    child_settings, cur_path)
                compile_section_settings.add_child(child)
            return compile_section_settings

        def construct_build_settings(build_settings_dict):
            if not build_settings_dict:
                return None
            build_settings = buildSettings()
            build_settings.basic_settings = construct_basic_settings(
                build_settings_dict.get('basicSettings'))
            build_settings.compile_settings = construct_compile_settings(
                build_settings_dict.get('compileSettings'), self.base_dir)
            build_settings.compile_section_settings = construct_compile_section_settings(
                build_settings_dict.get('compileSectionSettings'), self.base_dir)
            return build_settings

        def construct_prj_settings(project_settings_dict):
            prj_settings = projectSettings()
            prj_settings.project = project_settings_dict.get('project', 'cp')
            prj_settings.version = project_settings_dict.get(
                'version', '1.0.0')
            prj_settings.build_settings = construct_build_settings(
                project_settings_dict.get('buildSettings'))
            return prj_settings

        self.prj_settings = construct_prj_settings(json_data)
        # print(self.prj_settings)
        # print(dir(self.prj_settings))
        # if self.check_has_embedded_libraries():
        #     return
        self.compile_options_result = self.my_compile_options_collector.start_collecting(
            self.prj_settings)
        # print(self.compile_options_result)
        # self.convert_compile_settings()
        # for k, v in self.compile_settings_dict.items():
        #     print(k, v)
        self.get_compile_files()
        # print(self.consumed_dict)
        self.generate_cmake_variables()

    def check_has_embedded_libraries(self):
        prj_settings = self.prj_settings
        if prj_settings and prj_settings.build_settings and prj_settings.build_settings.compile_settings:
            compile_settings = prj_settings.build_settings.compile_settings
        else:
            return False

        def do_check(c_settings, in_library):
            if not c_settings:
                return False
            if c_settings.mode == 'library':
                if in_library:
                    return True
                for child_settings in c_settings.children:
                    if do_check(child_settings, True):
                        return True
                return False
            for child_settings in c_settings.children:
                if do_check(child_settings, False):
                    return True
            return False

        return do_check(compile_settings, compile_settings.mode == 'library')

    def convert_compile_settings(self):
        compile_settings_dict = {}

        def do_convert(base_dir, compile_settings_list):
            if not compile_settings_list:
                return
            for compile_settings in compile_settings_list:
                cur_path = os.path.join(base_dir, compile_settings.path)
                compile_settings_dict[cur_path] = compile_settings.mode
                do_convert(cur_path, compile_settings.children)

        do_convert(self.base_dir,
                   self.prj_settings.build_settings.compile_settings.children)
        self.compile_settings_dict = compile_settings_dict

    def get_compile_files(self):
        consumed_dict = {}

        def get_all_source_files_recursive(path, out_set):
            for root, _, files in os.walk(path):
                if root[0] == '.':
                    continue
                for file in files:
                    file_extension = os.path.splitext(file)[-1]
                    if file_extension in AVAILABLE_SOURCE_FILE_EXTENSIONS:
                        out_set.add(os.path.join(root, file))

        def get_all_source_files_recursive_with_excludes(path, out_set, excludes_set):
            for root, _, files in os.walk(path):
                if root[0] == '.':
                    continue
                if root in excludes_set:
                    continue
                for file in files:
                    file_extension = os.path.splitext(file)[-1]
                    file_path = os.path.join(root, file)
                    # if excludes_set:
                    #     print('excludes_set', excludes_set)
                    #     print(root, file_path)
                    if file_extension in AVAILABLE_SOURCE_FILE_EXTENSIONS and file_path not in excludes_set:
                        out_set.add(file_path)

        def do_get(path, compile_settings, consumed):
            if not compile_settings:
                get_all_source_files_recursive(path, consumed)
                return

            cur_left = set()
            cur_consumed = set()

            if compile_settings.mode == 'normal':
                if not compile_settings.children:
                    get_all_source_files_recursive(path, cur_consumed)
                else:
                    temp_set = set()
                    for child_settings in compile_settings.children:
                        temp_set.add(child_settings.path)
                        do_get(os.path.join(path, child_settings.path),
                               child_settings, cur_consumed)
                    names = os.listdir(path)
                    names.sort()

                    for name in names:
                        if name and name[0] == '.':
                            continue
                        if name in temp_set:
                            continue
                        child_path = os.path.join(path, name)
                        if os.path.isdir(child_path):
                            do_get(child_path, None, cur_consumed)
                        elif os.path.isfile(child_path):
                            file_extension = os.path.splitext(name)[-1]
                            if file_extension in AVAILABLE_SOURCE_FILE_EXTENSIONS:
                                cur_consumed.add(child_path)
            elif compile_settings.mode == 'library':
                excludes_set = set()
                if compile_settings.excludes:
                    for exclude in compile_settings.excludes:
                        excludes_set.add(os.path.join(path, exclude))
                if not compile_settings.children:
                    get_all_source_files_recursive_with_excludes(
                        path, cur_consumed, excludes_set)
                else:
                    temp_set = set()
                    for child_settings in compile_settings.children:
                        temp_set.add(child_settings.path)
                        do_get(os.path.join(path, child_settings.path),
                               child_settings, cur_consumed)
                    names = os.listdir(path)
                    names.sort()

                    for name in names:
                        if name and name[0] == '.':
                            continue
                        if name in temp_set:
                            continue
                        child_path = os.path.join(path, name)
                        if os.path.isdir(child_path):
                            do_get(child_path, None, cur_consumed)
                        elif os.path.isfile(child_path):
                            file_extension = os.path.splitext(name)[-1]
                            if file_extension in AVAILABLE_SOURCE_FILE_EXTENSIONS:
                                if child_path in excludes_set:
                                    cur_left.add(child_path)
                                else:
                                    cur_consumed.add(child_path)
                for exclude in excludes_set:
                    if os.path.isdir(exclude):
                        temp_set = set()
                        get_all_source_files_recursive(exclude, temp_set)
                        cur_left.update(temp_set)
                    elif os.path.isfile(exclude):
                        cur_left.add(exclude)
            consumed.update(cur_left)
            # print(path, cur_consumed)
            consumed_dict[path] = (compile_settings, cur_consumed)

        consumed = set()
        for child_settings in self.prj_settings.build_settings.compile_settings.children:
            do_get(os.path.join(self.base_dir, child_settings.path),
                   child_settings, consumed)
        names = os.listdir(self.base_dir)
        names.sort()
        for name in names:
            if name and name[0] == '.':
                continue
            child_path = os.path.join(self.base_dir, name)
            if child_path not in consumed_dict:
                if os.path.isdir(child_path):
                    do_get(child_path,  compileSettings(
                        name, 'normal', name, False, child_path), consumed)
        # print(consumed)
        # print(self.base_dir)
        # print(consumed_dict)
        if consumed:
            name = os.path.basename(self.base_dir)
            consumed_dict[self.base_dir] = (compileSettings(
                name, 'normal', name, False, self.base_dir), consumed)
        self.consumed_dict = consumed_dict

    def generate_cmake_variables(self):
        with open(self.cmake_prj_settings_file_path, 'w', encoding='utf-8') as f:
            consumed_dict = self.consumed_dict
            compile_options_result = self.compile_options_result
            compile_option_file_path = os.path.join(
                self.output_dir, '.compile_options')
            if os.path.exists(compile_option_file_path):
                os.remove(compile_option_file_path)
            for path, (compile_settings, consumed) in consumed_dict.items():
                is_root = False
                # if not consumed:
                #     continue
                mode = compile_settings.mode
                name = compile_settings.name
                delete = compile_settings.delete
                cmake_file_path = os.path.join(path, 'CMakeLists.txt')

                # relative_path = path[len(self.base_dir)+1:]
                if path == self.base_dir:
                    relative_path = os.path.basename(self.base_dir)
                    is_root = True
                else:
                    relative_path = os.path.relpath(path, self.base_dir)
                relative_path = relative_path.replace('\\', '/')

                config_relative_path = relative_path.replace('/', '_')
                config_relative_path = config_relative_path.replace('\\', '_')

                ordered_consumed_file_paths = [
                    file_path for file_path in consumed]
                ordered_consumed_file_paths.sort()
                temp_str = ' '.join([file_path.replace('\\', '/')
                                     for file_path in ordered_consumed_file_paths])
                if not os.path.exists(cmake_file_path):
                    shutil.copyfile(
                        self.cmake_template_file_path, cmake_file_path)
                f.write(
                    f'set(CONFIG_{config_relative_path}_SOURCES {temp_str})\n')

                if mode == 'normal':
                    temp_list = []
                    for file_path in ordered_consumed_file_paths:
                        if file_path in compile_options_result:
                            inner_list = [file_path]
                            for k, v in compile_options_result[file_path].items():
                                inner_list.append(f'{k}={v}')
                            temp_list.append(inner_list)
                    if temp_list:
                        with open(compile_option_file_path, 'a', encoding='utf-8') as cf:
                            for inner_list in temp_list:
                                cf.write(','.join(inner_list))
                                cf.write('\n')
                elif mode == 'library':
                    f.write(
                        f'set(CONFIG_{config_relative_path}_BUILT_LIBRARY TRUE)\n')
                    f.write(
                        f'set(CONFIG_{config_relative_path}_LIBRARY_NAME {name})\n')
                    f.write(
                        f'set(CONFIG_{config_relative_path}_LIBRARY_SOURCES_DELETE {"TRUE" if delete else "FALSE"})\n')

                    temp_list = []
                    # print(ordered_consumed_file_paths)
                    # for k, v in compile_options_result.items():
                    #     print(k, v)
                    for file_path in ordered_consumed_file_paths:
                        if file_path in compile_options_result:
                            inner_list = [file_path]
                            for k, v in compile_options_result[file_path].items():
                                inner_list.append(f'{k}={v}')
                            temp_list.append(inner_list)
                    if temp_list:
                        # print(temp_list)
                        library_compile_option_file_path = os.path.join(
                            self.output_dir, f'.compile_options.{name}').replace('\\', '/')
                        f.write(
                            f'set(CONFIG_{config_relative_path}_COMPILE_OPTION_FILE {library_compile_option_file_path})\n')
                        with open(library_compile_option_file_path, 'w', encoding='utf-8') as cf:
                            for inner_list in temp_list:
                                cf.write(','.join(inner_list))
                                cf.write('\n')
                if not is_root:
                    f.write(f'add_subdirectory({relative_path})\n')


def main():
    myXYSettingsHandler = XYSettingsHandler('.', '.prj_settings', '.ignore')
    myXYSettingsHandler.parse()


if __name__ == '__main__':
    main()
