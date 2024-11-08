# -*- coding: utf-8 -*-

import os
import sys
import subprocess
import shutil
import struct
import time
import multiprocessing as mp

from elftools.elf.elffile import ELFFile
from elftools.elf.constants import SH_FLAGS


class XYException(Exception):
    pass


class MyXYSectionRenamer:

    def __init__(self, base_dir, output_dir, compile_options_file_path, cc_bin_dir, cc_prefix, *cc_default_option):
        self.base_dir = os.path.abspath(base_dir)
        self.output_dir = os.path.abspath(output_dir)
        self.compile_options_file_path = os.path.abspath(
            compile_options_file_path)
        self.cc_bin_dir = os.path.abspath(cc_bin_dir) if cc_bin_dir else ''
        self.cc_default_option = cc_default_option
        self.cc_prefix = cc_prefix
        if not os.path.exists(self.output_dir):
            raise XYException('no object directory found')

    def do_rename_sections(self, c_path, options_list):
        if not options_list:
            return
        # print(c_path, options_list)
        options_list_t = []
        for option in options_list:
            old_compile_option, new_compile_option = option.split('=')
            options_list_t.append((old_compile_option, new_compile_option))

        o_path = os.path.join(
            self.output_dir, c_path[len(self.base_dir) + 1:] + '.o').replace('\\', '/')
        # shutil.copy(o_path, o_path + 'o')
        # o_path = o_path + 'o'
        # print(os.path.abspath(o_path))
        if not os.path.exists(o_path):
            # print('{} not found'.format(os.path.abspath(o_path)))
            return

        for old_compile_option, new_compile_option in options_list_t:
            #print(old_compile_option, new_compile_option)
            if old_compile_option != new_compile_option:
                break
        else:
            return

        section_names = []
        with open(o_path, 'rb') as f:
            elffile = ELFFile(f)
            for s in elffile.iter_sections():
                if not s.is_null() and s['sh_flags'] & SH_FLAGS.SHF_ALLOC != 0 and s['sh_size'] > 0:
                    section_names.append(s.name)
        # print(section_names)
        objcopy_cmd = os.path.join(self.cc_bin_dir, self.cc_prefix +
                                   'objcopy') if self.cc_bin_dir else self.cc_prefix + 'objcopy'
        objcopy_args = ['-p']
        if self.cc_default_option:
            for cc_option in self.cc_default_option:
                objcopy_args.append(cc_option)
        for old_compile_option, new_compile_option in options_list_t:
            if old_compile_option == '.text':
                # compile_options['.literal'] = new_compile_option.replace(
                #     r'.text', r'.literal')
                options_list_t.append(
                    ('.literal', new_compile_option.replace(r'.text', r'.literal')))
                break
        need_rename = False
        for old_compile_option, new_compile_option in options_list_t:
            # print(old_compile_option, new_compile_option)
            if old_compile_option == new_compile_option:
                continue
            for section_name in section_names:
                if section_name.startswith(old_compile_option):
                    objcopy_args.append('--rename-section')
                    objcopy_args.append(
                        f'{section_name}={new_compile_option + section_name[len(old_compile_option):]}')
                    # print('{0}={1}'.format(
                    #     elfSectionHeader.name, new_compile_option + elfSectionHeader.name[len(old_compile_option):]))
                    need_rename = True
        if not need_rename:
            return
        objcopy_args.append(o_path)
        temp_o_path = o_path + '.temp'
        objcopy_args.append(temp_o_path)
        # print(objcopy_args)
        objcopy_args_file = o_path + '.objcopy_args'
        # print(objcopy_args_file)
        # print(objcopy_cmd)
        with open(objcopy_args_file, 'w') as f:
            f.write('\n'.join(objcopy_args))
        with subprocess.Popen([objcopy_cmd, f'@{objcopy_args_file}'], text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE) as p:
            # print(p.args)
            stdout, stderr = p.communicate()
            # print(stderr)
            os.remove(objcopy_args_file)
            # print(temp_o_path, o_path)
            os.replace(temp_o_path, o_path)

    def rename_sections(self):
        if not os.path.exists(self.compile_options_file_path):
            return
        async_args = []
        with open(self.compile_options_file_path, 'r') as f:
            for line in f:
                line = line.strip()
                temp_list = line.split(',')
                c_path = temp_list[0]
                if os.path.commonpath([c_path, self.base_dir]) == self.base_dir:
                    async_args.append((c_path, temp_list[1:]))
        with mp.Pool(min(mp.cpu_count(), 32)) as pool:
            result = pool.starmap(self.do_rename_sections, async_args)
            pool.close()
            pool.join()


def main():
    # print('rename sections', sys.argv)
    try:
        mp.freeze_support()
        top_dir = sys.argv[1]
        object_dir = sys.argv[2]
        compile_options_file_path = sys.argv[3]

        start_time = time.perf_counter()
        myXYSectionRenamer = MyXYSectionRenamer(
            top_dir, object_dir, compile_options_file_path, '', 'arm-none-eabi-')
        myXYSectionRenamer.rename_sections()
        end_time = time.perf_counter()
        print(f'{end_time - start_time:.3f} taken!')
    except Exception as e:
        print(e)
        sys.exit(-1)


if __name__ == '__main__':
    main()
