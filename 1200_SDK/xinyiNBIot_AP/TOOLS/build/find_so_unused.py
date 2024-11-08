# -*- encoding: utf-8 -*-

import os
import sys
import time
from collections import defaultdict

from elftools.elf.elffile import ELFFile
from elftools.elf.enums import ENUM_RELOC_TYPE_ARM
from elftools.elf.relocation import RelocationSection

skip_elf_reloc_names = set(['_printf_float'])
skip_so_func_names = set(['dummy_front'])


class XYException(Exception):
    pass


class ElfFunctionInfo:

    def __init__(self, die, name, file, line, is_global=True):
        self.die = die
        self.name = name
        self.file = file
        self.line = line
        self.is_global = is_global
        self.callee_dict = {}

    def add_callee(self, name, func_info):
        self.callee_dict[name] = func_info

    def __str__(self):
        return self.__repr__()

    def __repr__(self):
        return f'function name: {self.name}, file: {self.file}, line: {self.line}, is_global: {self.is_global}'


def find_so_unused(elf_filename, so_filename):
    # print('find_so_unused files:', elf_filename, so_filename)
    with open(elf_filename, 'rb') as elf_f, open(so_filename, 'rb') as so_f:
        elffile = ELFFile(elf_f)
        sofile = ELFFile(so_f)

        so_reldyn = sofile.get_section_by_name('.rel.dyn')
        if not isinstance(so_reldyn, RelocationSection):
            print(f'{so_filename} has no .rel.dyn section')
            return False

        elf_reldyn = elffile.get_section_by_name('.rel.dyn')
        if not isinstance(elf_reldyn, RelocationSection):
            print(f'{elf_filename} has no .rel.dyn section')
            return False

        elf_dynsym = elffile.get_section_by_name('.dynsym')

        elf_jump_slot_name_list = []
        for elf_reloc in elf_reldyn.iter_relocations():
            if elf_reloc['r_info_type'] == ENUM_RELOC_TYPE_ARM['R_ARM_JUMP_SLOT']:
                elf_symbol = elf_dynsym.get_symbol(elf_reloc['r_info_sym'])
                if elf_symbol.name not in skip_elf_reloc_names:
                    elf_jump_slot_name_list.append(elf_symbol.name)
                # print(elf_reloc)
        # print(elf_jump_slot_name_list)

        so_dwarfinfo = sofile.get_dwarf_info()

        cu_funcs_dict = {}
        used_cu_funcs_dict = {}

        for so_cu in so_dwarfinfo.iter_CUs():
            used_cu_funcs_dict[so_cu] = set()
            top_die = so_cu.get_top_DIE()
            top_die_full_path = top_die.get_full_path()
            name_func_info_dict = {}
            cu_funcs_dict[so_cu] = name_func_info_dict
            # print('top_die name', top_die_full_path)
            line_program = so_dwarfinfo.line_program_for_CU(so_cu)
            # print(line_program.header)
            include_directories = line_program.header.include_directory
            file_entries = line_program.header.file_entry
            for die in top_die.iter_children():
                if die.tag == 'DW_TAG_subprogram':
                    if 'DW_AT_declaration' in die.attributes:
                        continue
                    if 'DW_AT_name' not in die.attributes:
                        continue
                    if 'DW_AT_linkage_name' in die.attributes:
                        name = die.attributes['DW_AT_linkage_name'].value.decode(
                        )
                    else:
                        name = die.attributes['DW_AT_name'].value.decode()
                    file_entry = file_entries[die.attributes['DW_AT_decl_file'].value-1]
                    file = os.path.join(include_directories[file_entry['dir_index']-1].decode(
                    ), file_entry['name'].decode())
                    line = die.attributes['DW_AT_decl_line'].value
                    func_info = ElfFunctionInfo(
                        die, name, file, line, 'DW_AT_external' in die.attributes)
                    name_func_info_dict[name] = func_info
        # print(cu_funcs_dict)

        for so_cu, name_func_info_dict in cu_funcs_dict.items():
            for name, func_info in name_func_info_dict.items():
                for die in func_info.die.iter_children():
                    if die.tag == 'DW_TAG_GNU_call_site':
                        if 'DW_AT_abstract_origin' not in die.attributes:
                            continue
                        callee_die = so_dwarfinfo.get_DIE_from_refaddr(
                            die.attributes['DW_AT_abstract_origin'].value + so_cu.cu_offset)
                        if 'DW_AT_name' not in callee_die.attributes:
                            continue
                        if 'DW_AT_linkage_name' in callee_die.attributes:
                            callee_name = callee_die.attributes['DW_AT_linkage_name'].value.decode(
                            )
                        else:
                            callee_name = callee_die.attributes['DW_AT_name'].value.decode(
                            )
                        if callee_name == name:
                            continue
                        is_global = 'DW_AT_external' in callee_die.attributes
                        if callee_name in name_func_info_dict:
                            func_info.add_callee(
                                callee_name, name_func_info_dict[callee_name])
                            continue
                        if is_global:
                            for _, other_name_func_info_dict in cu_funcs_dict.items():
                                if name_func_info_dict != other_name_func_info_dict:
                                    if callee_name in other_name_func_info_dict:
                                        func_info.add_callee(
                                            callee_name, other_name_func_info_dict[callee_name])
                                        break
                            continue

        def dfs_mark_used_funcs(name, func_info):
            if name in used_cu_funcs_dict[func_info.die.cu]:
                return
            used_cu_funcs_dict[func_info.die.cu].add(name)
            for callee_name, callee_func_info in func_info.callee_dict.items():
                dfs_mark_used_funcs(callee_name, callee_func_info)

        for elf_jump_slot_name in elf_jump_slot_name_list:
            for so_cu, name_func_info_dict in cu_funcs_dict.items():
                if elf_jump_slot_name in name_func_info_dict:
                    dfs_mark_used_funcs(
                        elf_jump_slot_name, name_func_info_dict[elf_jump_slot_name])

        unused_cu_funcs_dict = defaultdict(list)
        for so_cu, name_func_info_dict in cu_funcs_dict.items():
            top_die = so_cu.get_top_DIE()
            top_die_full_path = top_die.get_full_path()
            for name, func_info in name_func_info_dict.items():
                if name in skip_so_func_names:
                    continue
                if name not in used_cu_funcs_dict[so_cu]:
                    unused_cu_funcs_dict[top_die_full_path].append(
                        (name, func_info.line))

        if unused_cu_funcs_dict:
            print('possible unused functions:')
            indent = ' '*4
            for full_path, line_name_list in unused_cu_funcs_dict.items():
                line_name_list.sort(key=lambda x: x[1])
                print(f"{indent}{full_path}:")
                for name, line in line_name_list:
                    print(f"{indent*2}{name}, line {line}")
        else:
            print('no unused functions have been found.')


def main():
    if len(sys.argv) < 3:
        print('no enough arguments...')
        sys.exit(-1)
    elf_path = sys.argv[1]
    so_path = sys.argv[2]
    start_time = time.perf_counter()
    find_so_unused(elf_path, so_path)
    end_time = time.perf_counter()
    print(f"find_so_unused, {end_time-start_time:.3f}s taken!")


if __name__ == '__main__':
    main()
