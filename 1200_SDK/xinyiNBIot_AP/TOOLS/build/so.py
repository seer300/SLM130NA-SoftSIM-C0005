# -*- encoding: utf-8 -*-

import os
import struct
import sys
import time

from elftools.elf.elffile import ELFFile
from elftools.elf.enums import ENUM_RELOC_TYPE_ARM
from elftools.elf.relocation import RelocationSection
from elftools.elf.sections import Symbol

skip_symbol_names = set(['_printf_float'])


class XYException(Exception):
    pass


class MySymbol(Symbol):
    '''pass in a dict-like entry to fake a symbol'''


def process(elf_filename, so_filename, so_data_end_addr, so_text_addr1, max_so_text_size, ban_write_flash, ban_write_flash_remain_len, so_text_path, elf_flash_bin_path, elf_flash_bin_exec_addr, elf_ram_bin_path, elf_ram_bin_exec_addr):
    def my_align(x, alignment):
        return (x + alignment - 1) & (~(alignment - 1))

    print('Processing files:', elf_filename, so_filename)

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

        so_dynsym = sofile.get_section_by_name('.dynsym')
        so_symtab = sofile.get_section_by_name('.symtab')

        elf_dynsym = elffile.get_section_by_name('.dynsym')
        elf_symtab = elffile.get_section_by_name('.symtab')

        elf_symbol_set = set()
        for elf_symbol in elf_symtab.iter_symbols():
            if elf_symbol['st_info']['bind'] in ['STB_GLOBAL', 'STB_WEAK'] and elf_symbol['st_info']['type'] in ['STT_FUNC', 'STT_OBJECT'] and elf_symbol['st_shndx'] != 'SHN_UNDEF':
                elf_symbol_set.add(elf_symbol.name)

        with open(elf_flash_bin_path, 'rb') as temp_f:
            elf_flash_bin_data = bytearray(temp_f.read())
        with open(elf_ram_bin_path, 'rb') as temp_f:
            elf_ram_bin_data = bytearray(temp_f.read())
        # elf_flash_bin_size = len(elf_flash_bin_data)
        # elf_flash_bin_exec_end_addr = elf_flash_bin_exec_addr + elf_flash_bin_size
        elf_ram_bin_size = len(elf_ram_bin_data)
        elf_ram_bin_exec_end_addr = elf_ram_bin_exec_addr + elf_ram_bin_size
        elf_final_ram_bin_load_addr = my_align(
            elf_flash_bin_exec_addr+len(elf_flash_bin_data), 4096)
        dyninfo_one_item_length = 12
        dynitem_num = 12
        dyninfo_data_size = dynitem_num * dyninfo_one_item_length

        so_got = sofile.get_section_by_name('.got')
        if so_got:
            so_got_data = so_got.data()
            so_got_start_addr = so_got['sh_addr']
            so_got_size = so_got['sh_size']
            so_got_end_addr = so_got_start_addr + so_got_size
            new_so_got1_data = bytearray(so_got_size)
            new_so_got2_data = bytearray(so_got_size)
        else:
            so_got_size = 0

        so_got_plt = sofile.get_section_by_name('.got.plt')
        so_got_plt_start_addr = so_got_plt['sh_addr']
        so_got_plt_size = so_got_plt['sh_size']
        new_so_got_plt1_data = bytearray(so_got_plt.data())
        new_so_got_plt2_data = bytearray(so_got_plt.data())

        so_text_section = sofile.get_section_by_name('.text')
        so_data_section = sofile.get_section_by_name('.data')
        so_data_section_data = so_data_section.data()
        so_bss_section = sofile.get_section_by_name('.bss')
        so_bss_section_data = so_bss_section.data()
        so_rodata_section = sofile.get_section_by_name('.rodata')
        so_rodata_section_data = so_rodata_section.data()
        so_usable_sections = [(so_data_section, so_data_section_data), (
            so_bss_section, so_bss_section_data), (so_rodata_section, so_rodata_section_data)]
        # so_text_section_index = sofile.get_section_index('.text')
        so_data_section_index = sofile.get_section_index('.data')
        so_bss_section_index = sofile.get_section_index('.bss')
        # so_rodata_section_index = sofile.get_section_index('.rodata')
        so_got_modified = False

        if ban_write_flash:
            if max_so_text_size < ban_write_flash_remain_len:
                raise XYException(f"error ban_write_flash open, cp_used_apram should > {ban_write_flash_remain_len} ")
            so_text_addr1 += ban_write_flash_remain_len * 1024
            max_so_text_size -= ban_write_flash_remain_len * 1024
               
        so_text_size = os.path.getsize(so_text_path)
        if so_text_size + so_got_plt_size <= max_so_text_size:
            pass
        else:
            raise XYException(f"error SO RAM 0x{so_text_size:x} + 0x{so_got_plt_size:x} size excceed 0x{max_so_text_size:x}")
        
        so_golb_dat_dict = {}
        so_relative_dict = {}
        so_abs32_list = []
        so_jump_slot_dict = {}
        for so_reloc in so_reldyn.iter_relocations():
            so_sym = so_dynsym.get_symbol(so_reloc['r_info_sym'])
            if so_reloc['r_info_type'] == ENUM_RELOC_TYPE_ARM['R_ARM_GLOB_DAT']:
                so_golb_dat_dict[so_sym.name] = so_reloc
            elif so_reloc['r_info_type'] == ENUM_RELOC_TYPE_ARM['R_ARM_RELATIVE']:
                so_relative_dict[so_reloc['r_offset']] = so_reloc
            elif so_reloc['r_info_type'] == ENUM_RELOC_TYPE_ARM['R_ARM_ABS32']:
                so_abs32_list.append(so_reloc)
            elif so_reloc['r_info_type'] == ENUM_RELOC_TYPE_ARM['R_ARM_JUMP_SLOT']:
                so_jump_slot_dict[so_dynsym.get_symbol(
                    so_reloc['r_info_sym']).name] = so_reloc

        elf_dynbss = elffile.get_section_by_name('.dynbss')
        if not elf_dynbss:
            elf_dynbss_start_addr = elf_dynbss_size = elf_dynbss_end_addr = 0
            elf_dynbss_data = b''
        else:
            elf_dynbss_start_addr = elf_dynbss['sh_addr']
            elf_dynbss_size = elf_dynbss['sh_size']
            elf_dynbss_end_addr = elf_dynbss_start_addr + elf_dynbss_size
            elf_dynbss_data = bytearray(elf_dynbss_size)

        for elf_symbol in elf_symtab.iter_symbols():
            if elf_symbol.name == "_Ram_dynbss_exec_addr":
                _Ram_dynbss_exec_addr = elf_symbol['st_value']
                
        so_text_addr2 = my_align(elf_final_ram_bin_load_addr +
                                 dyninfo_data_size + _Ram_dynbss_exec_addr - elf_ram_bin_exec_addr + elf_dynbss_size, 4)


        elf_got_plt = elffile.get_section_by_name('.got.plt')
        elf_got_plt_start_addr = elf_got_plt['sh_addr']
        elf_got_plt_size = elf_got_plt['sh_size']
        new_elf_got_plt1_data = bytearray(elf_got_plt.data())
        new_elf_got_plt2_data = bytearray(elf_got_plt.data())

        # elf_data_section_index = elffile.get_section_index('.data')
        # elf_bss_section_index = elffile.get_section_index('.bss')
        elf_so_info_dict = {}
        elf_so_info_reverse_addr_dict = {}
        elf_copy_dict = {}
        elf_jump_slot_dict = {}
        for elf_reloc in elf_reldyn.iter_relocations():
            if elf_reloc['r_info_type'] == ENUM_RELOC_TYPE_ARM['R_ARM_COPY']:
                symbol = elf_dynsym.get_symbol(elf_reloc['r_info_sym'])
                elf_symbol_set.remove(symbol.name)
                elf_copy_dict[symbol.name] = elf_reloc
                for so_symbol in so_symtab.get_symbol_by_name(symbol.name):
                    if so_symbol['st_info']['bind'] in ['STB_GLOBAL', 'STB_WEAK']:
                        break
                else:
                    raise XYException(
                        f'cannot find {symbol.name} in {so_filename}')
                so_symbol_section_index = so_symbol['st_shndx']
                if so_symbol_section_index == so_data_section_index:
                    elf_so_info_dict[symbol.name] = (
                        so_symbol['st_value'], symbol['st_value'], so_symbol['st_size'])
                    elf_so_info_reverse_addr_dict[so_symbol['st_value']
                                                  ] = symbol['st_value']
                elif so_symbol_section_index == so_bss_section_index:
                    elf_so_info_dict[symbol.name] = (
                        -1, symbol['st_value'], so_symbol['st_size'])
                    elf_so_info_reverse_addr_dict[so_symbol['st_value']
                                                  ] = symbol['st_value']
                else:  # so rodata
                    elf_so_info_dict[symbol.name] = (
                        so_symbol['st_value'], symbol['st_value'], so_symbol['st_size'])
                    elf_so_info_reverse_addr_dict[so_symbol['st_value']
                                                  ] = symbol['st_value']
            elif elf_reloc['r_info_type'] == ENUM_RELOC_TYPE_ARM['R_ARM_JUMP_SLOT']:
                elf_jump_slot_dict[elf_dynsym.get_symbol(
                    elf_reloc['r_info_sym']).name] = elf_reloc

        for so_symbol in so_symtab.iter_symbols():
            if so_symbol['st_info']['bind'] in ['STB_GLOBAL', 'STB_WEAK'] and so_symbol['st_info']['type'] in ['STT_FUNC', 'STT_OBJECT'] and so_symbol['st_shndx'] != 'SHN_UNDEF':
                if so_symbol.name in elf_symbol_set:
                    raise XYException(
                        f"symbol {so_symbol.name} appears multiple times!")

        so_rodata_size = so_rodata_section['sh_size']
        so_data_size = so_data_section['sh_size']
        so_bss_size = so_bss_section['sh_size']
        so_rodata_size_aligned = my_align(
            so_rodata_size, so_data_section['sh_addralign'])
        so_data_size_aligned = my_align(
            so_data_size, so_bss_section['sh_addralign'])
        so_bss_size_aligned = so_bss_size
        new_so_data_bin_size = so_rodata_size_aligned+so_data_size_aligned
        new_so_data_bin_data = bytearray(new_so_data_bin_size)
        struct.pack_into(f"<{so_rodata_size}s",
                         new_so_data_bin_data, 0, so_rodata_section_data)
        struct.pack_into(f"<{so_data_size}s", new_so_data_bin_data,
                         so_rodata_size_aligned, so_data_section_data)

        new_so_data_bin_start_addr = new_so_rodata_start_addr = so_data_end_addr - \
            so_rodata_size_aligned - so_data_size_aligned - so_bss_size_aligned
        new_so_rodata_end_addr = new_so_rodata_start_addr + so_rodata_size_aligned
        new_so_data_start_addr = new_so_rodata_end_addr
        new_so_data_end_addr = new_so_data_start_addr + so_data_size_aligned
        new_so_bss_start_addr = new_so_data_end_addr
        # new_so_bss_end_addr = new_so_bss_start_addr + so_bss_size_aligned
        # new_so_data_bin_end_addr = new_so_bss_end_addr

        new_so_got_start_addr = new_so_data_bin_start_addr - so_got_size

        def get_new_so_data_addr(so_old_addr):
            if so_rodata_section['sh_addr'] <= so_old_addr < so_rodata_section['sh_addr'] + so_rodata_section['sh_size']:
                return new_so_rodata_start_addr + so_old_addr - so_rodata_section['sh_addr']
            elif so_data_section['sh_addr'] <= so_old_addr < so_data_section['sh_addr'] + so_data_section['sh_size']:
                return new_so_data_start_addr + so_old_addr - so_data_section['sh_addr']
            elif so_bss_section['sh_addr'] <= so_old_addr < so_bss_section['sh_addr'] + so_bss_section['sh_size']:
                return new_so_bss_start_addr + so_old_addr - so_bss_section['sh_addr']
            else:
                raise XYException(f"invalid so addr 0x{so_old_addr:x}")

        elf_modified_addrs = set()
        # print('so R_ARM_RELATIVE')
        for so_addr, so_reloc in so_relative_dict.items():
            if so_got_start_addr <= so_addr < so_got_end_addr:
                old_got_item_so_addr, = struct.unpack_from(
                    '<I', so_got_data, so_addr - so_got_start_addr)
                if so_text_section['sh_addr'] <= old_got_item_so_addr < so_text_section['sh_addr'] + so_text_section['sh_size']:
                    so_got_modified = True
                    struct.pack_into('<I', new_so_got1_data, so_addr -
                                     so_got_start_addr, old_got_item_so_addr + so_text_addr1)
                    struct.pack_into('<I', new_so_got2_data, so_addr -
                                     so_got_start_addr, old_got_item_so_addr + so_text_addr2)
                elif so_rodata_section['sh_addr'] <= old_got_item_so_addr < so_rodata_section['sh_addr'] + so_rodata_section['sh_size']:
                    struct.pack_into('<I', new_so_got1_data, so_addr - so_got_start_addr,
                                     new_so_rodata_start_addr + old_got_item_so_addr - so_rodata_section['sh_addr'])
                    struct.pack_into('<I', new_so_got2_data, so_addr - so_got_start_addr,
                                     new_so_rodata_start_addr + old_got_item_so_addr - so_rodata_section['sh_addr'])
                elif so_data_section['sh_addr'] <= old_got_item_so_addr < so_data_section['sh_addr'] + so_data_section['sh_size']:
                    struct.pack_into('<I', new_so_got1_data, so_addr - so_got_start_addr,
                                     new_so_data_start_addr + old_got_item_so_addr - so_data_section['sh_addr'])
                    struct.pack_into('<I', new_so_got2_data, so_addr - so_got_start_addr,
                                     new_so_data_start_addr + old_got_item_so_addr - so_data_section['sh_addr'])
                elif so_bss_section['sh_addr'] <= old_got_item_so_addr < so_bss_section['sh_addr'] + so_bss_section['sh_size']:
                    struct.pack_into('<I', new_so_got1_data, so_addr - so_got_start_addr,
                                     new_so_bss_start_addr + old_got_item_so_addr - so_bss_section['sh_addr'])
                    struct.pack_into('<I', new_so_got2_data, so_addr - so_got_start_addr,
                                     new_so_bss_start_addr + old_got_item_so_addr - so_bss_section['sh_addr'])
                else:
                    raise XYException(
                        f"invalid so addr 0x{old_got_item_so_addr:x}")
            else:
                for section, section_data in so_usable_sections:
                    if section['sh_addr'] <= so_addr < section['sh_addr'] + section['sh_size']:
                        old_item_so_addr, = struct.unpack_from(
                            '<I', section_data, so_addr - section['sh_addr'])
                        break
                else:
                    raise XYException(f"invalid so addr 0x{so_addr:x}")
                if so_addr in elf_so_info_reverse_addr_dict:
                    new_so_data_addr = elf_so_info_reverse_addr_dict[so_addr]
                    elf_modified_addrs.add(new_so_data_addr)
                else:
                    new_so_data_addr = get_new_so_data_addr(so_addr)
                if old_item_so_addr in elf_so_info_reverse_addr_dict:
                    new_item_so_addr = elf_so_info_reverse_addr_dict[old_item_so_addr]
                else:
                    new_item_so_addr = get_new_so_data_addr(old_item_so_addr)
                # print(
                #     f"modify offset 0x{new_so_data_addr:08x} from 0x{old_item_so_addr:x} to 0x{new_item_so_addr:x}")

                if new_so_rodata_start_addr <= new_so_data_addr < new_so_rodata_end_addr:
                    struct.pack_into('<I', new_so_data_bin_data, new_so_data_addr -
                                     new_so_rodata_start_addr, new_item_so_addr)
                elif new_so_data_start_addr <= new_so_data_addr < new_so_data_end_addr:
                    struct.pack_into('<I', new_so_data_bin_data, new_so_data_addr -
                                     new_so_data_start_addr + so_rodata_size_aligned, new_item_so_addr)
                else:
                    # print(f"modify elf offset 0x{new_so_data_addr:08x}")
                    if elf_ram_bin_exec_addr <= new_so_data_addr < elf_ram_bin_exec_end_addr:
                        struct.pack_into(
                            '<I', elf_ram_bin_data, new_so_data_addr - elf_ram_bin_exec_addr, new_item_so_addr)
                    elif elf_dynbss_start_addr <= new_so_data_addr < elf_dynbss_end_addr:
                        struct.pack_into(
                            '<I', elf_dynbss_data, new_so_data_addr - elf_dynbss_start_addr, new_item_so_addr)
                    else:
                        raise XYException(
                            f"invalid elf addr 0x{new_so_data_addr:x}")

        # print('so R_ARM_GLOB_DAT')
        for name, so_reloc in so_golb_dat_dict.items():
            so_addr = so_reloc['r_offset']
            symbol = so_dynsym.get_symbol(so_reloc['r_info_sym'])
            symbol_addr = symbol['st_value']
            if symbol_addr == 0:
                for so_symbol in elf_symtab.get_symbol_by_name(symbol.name):
                    if so_symbol['st_info']['bind'] in ['STB_GLOBAL', 'STB_WEAK']:
                        break
                else:
                    raise XYException(
                        f'cannot find {symbol.name} in {elf_filename}')
                new_so_data_addr1 = new_so_data_addr2 = so_symbol['st_value']
            elif symbol_addr in elf_so_info_reverse_addr_dict:
                new_so_data_addr1 = new_so_data_addr2 = elf_so_info_reverse_addr_dict[
                    symbol_addr]
            elif so_text_section['sh_addr'] <= symbol_addr < so_text_section['sh_addr'] + so_text_section['sh_size']:
                so_got_modified = True
                new_so_data_addr1 = symbol_addr + so_text_addr1
                new_so_data_addr2 = symbol_addr + so_text_addr2
            else:
                new_so_data_addr1 = new_so_data_addr2 = get_new_so_data_addr(
                    symbol_addr)
            if so_got_start_addr <= so_addr < so_got_end_addr:
                # print(
                #     f"modify so got offset 0x{so_addr - so_got_start_addr:x} to 0x{new_so_data_addr1:x} and 0x{new_so_data_addr2:x}")
                struct.pack_into('<I', new_so_got1_data,
                                 so_addr - so_got_start_addr, new_so_data_addr1)
                struct.pack_into('<I', new_so_got2_data,
                                 so_addr - so_got_start_addr, new_so_data_addr2)
            else:
                raise XYException(f"invalid so addr 0x{so_addr:x}")

        modified_data_list = []
        # print('so R_ARM_ABS32')
        for so_reloc in so_abs32_list:
            so_addr = so_reloc['r_offset']
            symbol = so_dynsym.get_symbol(so_reloc['r_info_sym'])
            symbol_addr = symbol['st_value']
            is_changed_data = False
            if symbol_addr == 0:
                for so_symbol in elf_symtab.get_symbol_by_name(symbol.name):
                    if so_symbol['st_info']['bind'] in ['STB_GLOBAL', 'STB_WEAK']:
                        break
                else:
                    raise XYException(
                        f'cannot find {symbol.name} in {so_filename}')
                new_so_data_addr = so_symbol['st_value']
            elif symbol_addr in elf_so_info_reverse_addr_dict:
                new_so_data_addr = elf_so_info_reverse_addr_dict[symbol_addr]
            elif so_text_section['sh_addr'] <= symbol_addr < so_text_section['sh_addr'] + so_text_section['sh_size']:
                new_so_data_addr = symbol_addr + so_text_addr1
                new_so_data_addr2 = symbol_addr + so_text_addr2
                is_changed_data = True
            else:
                new_so_data_addr = get_new_so_data_addr(symbol_addr)
            if so_addr in elf_so_info_reverse_addr_dict:
                new_so_data_from_addr = elf_so_info_reverse_addr_dict[so_addr]
                elf_modified_addrs.add(new_so_data_from_addr)
            else:
                new_so_data_from_addr = get_new_so_data_addr(so_addr)
            if is_changed_data:
                modified_data_list.append(new_so_data_from_addr)
            # print(
            #     f"modify offset 0x{new_so_data_from_addr:x} to 0x{new_so_data_addr:x}")
            if new_so_rodata_start_addr <= new_so_data_from_addr < new_so_rodata_end_addr:
                struct.pack_into('<I', new_so_data_bin_data, new_so_data_from_addr -
                                 new_so_rodata_start_addr, new_so_data_addr)
            elif new_so_data_start_addr <= new_so_data_from_addr < new_so_data_end_addr:
                struct.pack_into('<I', new_so_data_bin_data, new_so_data_from_addr -
                                 new_so_data_start_addr + so_rodata_size_aligned, new_so_data_addr)
            else:
                # print(f"modify elf offset 0x{new_so_data_from_addr:08x}")
                if elf_ram_bin_exec_addr <= new_so_data_from_addr < elf_ram_bin_exec_end_addr:
                    struct.pack_into(
                        '<I', elf_ram_bin_data, new_so_data_from_addr - elf_ram_bin_exec_addr, new_so_data_addr)
                elif elf_dynbss_start_addr <= new_so_data_from_addr < elf_dynbss_end_addr:
                    struct.pack_into(
                        '<I', elf_dynbss_data, new_so_data_from_addr - elf_dynbss_start_addr, new_so_data_addr)
                else:
                    raise XYException(
                        f"invalid elf addr 0x{new_so_data_from_addr:x}")

        # print('elf R_ARM_COPY')
        for name, (so_addr, elf_addr, size) in elf_so_info_dict.items():
            if so_addr == -1:
                # print(f"elf clear bss {name} at 0x{elf_addr:x}, size {size}")
                if elf_ram_bin_exec_addr <= elf_addr < elf_ram_bin_exec_end_addr:
                    for i in range(elf_addr - elf_ram_bin_exec_addr, elf_addr - elf_ram_bin_exec_addr + size):
                        elf_ram_bin_data[i] = 0
                elif elf_dynbss_start_addr <= elf_addr < elf_dynbss_end_addr:
                    for i in range(elf_addr - elf_dynbss_start_addr, elf_addr - elf_dynbss_start_addr + size):
                        elf_dynbss_data[i] = 0
                else:
                    raise XYException(f"invalid elf addr 0x{elf_addr:x}")
            else:
                if elf_addr in elf_modified_addrs:
                    continue
                for section, section_data in so_usable_sections:
                    if section['sh_addr'] <= so_addr < section['sh_addr'] + section['sh_size']:
                        value, = struct.unpack_from(
                            f"<{size}s", section_data, so_addr - section['sh_addr'])
                        break
                else:
                    raise XYException(f"invalid so addr 0x{so_addr:x}")
                # print(
                #     f"elf copy data {name} from 0x{so_addr:x} to 0x{elf_addr:x}, size {size}")
                if elf_ram_bin_exec_addr <= elf_addr < elf_ram_bin_exec_end_addr:
                    struct.pack_into(
                        f'<{size}s', elf_ram_bin_data, elf_addr - elf_ram_bin_exec_addr, value)
                elif elf_dynbss_start_addr <= elf_addr < elf_dynbss_end_addr:
                    struct.pack_into(
                        f'<{size}s', elf_dynbss_data, elf_addr - elf_dynbss_start_addr, value)
                else:
                    raise XYException(f"invalid elf addr 0x{elf_addr:x}")

        # print('so R_ARM_JUMP_SLOT')
        for name, reloc in so_jump_slot_dict.items():
            so_symbol = so_dynsym.get_symbol(reloc['r_info_sym'])
            if so_symbol['st_value'] == 0:  # function in elf
                for elf_symbol in elf_symtab.get_symbol_by_name(name):
                    if elf_symbol['st_info']['bind'] in ['STB_GLOBAL', 'STB_WEAK']:
                        break
                else:
                    raise XYException(f'cannot find {name} in {elf_filename}')
                # print(
                #     f"in so, function {name}, modify .got.plt at 0x{reloc['r_offset']:x} to elf 0x{elf_symbol['st_value']:x}")
                struct.pack_into('<I', new_so_got_plt1_data,
                                 reloc['r_offset'] - so_got_plt_start_addr, elf_symbol['st_value'])
                struct.pack_into('<I', new_so_got_plt2_data,
                                 reloc['r_offset'] - so_got_plt_start_addr, elf_symbol['st_value'])
            else:
                # print(
                #     f"in so, function {name}, modify .got.plt at 0x{reloc['r_offset']:x} to so 0x{so_symbol['st_value']+so_text_addr1:x} and 0x{so_symbol['st_value']+so_text_addr2:x}")
                struct.pack_into('<I', new_so_got_plt1_data,
                                 reloc['r_offset'] - so_got_plt_start_addr, so_symbol['st_value']+so_text_addr1)
                struct.pack_into('<I', new_so_got_plt2_data,
                                 reloc['r_offset'] - so_got_plt_start_addr, so_symbol['st_value']+so_text_addr2)
        so_text_size = os.path.getsize(so_text_path)
        if so_text_size + so_got_plt_size > max_so_text_size:
            raise XYException(
                f"{so_text_size + so_got_plt_size} size excceed 0x{max_so_text_size:x}")

        # print('elf R_ARM_JUMP_SLOT')
        for name, reloc in elf_jump_slot_dict.items():
            if name in skip_symbol_names:
                continue
            so_symbols = so_symtab.get_symbol_by_name(name)
            if not so_symbols:
                raise XYException(f'cannot find {name} in {so_filename}')
            for so_symbol in so_symbols:
                if so_symbol['st_info']['bind'] in ['STB_GLOBAL', 'STB_WEAK']:
                    break
            else:
                raise XYException(f'cannot find {name} in {so_filename}')
            # print(
            #     f"in elf, function {name}, modify .got.plt at 0x{reloc['r_offset']:x} to 0x{so_symbol['st_value']+so_text_addr1:x} and 0x{so_symbol['st_value']+so_text_addr2:x}")
            struct.pack_into('<I', new_elf_got_plt1_data,
                             reloc['r_offset'] - elf_got_plt_start_addr, so_symbol['st_value']+so_text_addr1)
            struct.pack_into('<I', new_elf_got_plt2_data,
                             reloc['r_offset'] - elf_got_plt_start_addr, so_symbol['st_value']+so_text_addr2)

        elf_got_plt = elffile.get_section_by_name('.got.plt')
        elf_got_plt_start_addr = elf_got_plt['sh_addr']
        elf_got_plt_size = elf_got_plt['sh_size']
        elf_got_plt_offset = elf_got_plt_start_addr-elf_ram_bin_exec_addr
        for i in range(elf_got_plt_size):
            elf_ram_bin_data[elf_got_plt_offset+i] = new_elf_got_plt1_data[i]

        '''
        dyn_info.bin struct data
         _ _ _ _ _ _ _
        |             
        |  ram.bin  
        |_ _ _ _ _ _ _
        |             
        |  so text    
        |_ _ _ _ _ _ _
        |
        |  so .got.plt2
        |_ _ _ _ _ _ _
        |
        |  so data
        |_ _ _ _ _ _ _
        |
        |  so bss 
        |_ _ _ _ _ _ _
        |
        |  elf .got.plt1
        |_ _ _ _ _ _ _
        |
        |  elf .got.plt2
        |_ _ _ _ _ _ _
        |
        |  so .got.plt1
        |_ _ _ _ _ _ _
        |
        |  so .got1 
        |_ _ _ _ _ _ _
        |
        |  so .got2 
        |_ _ _ _ _ _ _
        |
        |  so data change info
        |_ _ _ _ _ _ _

        '''

        current_ram_bin_load_addr = elf_final_ram_bin_load_addr
        dyninfo_data = bytearray(dyninfo_data_size)
        current_ram_bin_load_addr += dyninfo_data_size
        offset = 0
        offset += dyninfo_one_item_length

        struct.pack_into('<III', dyninfo_data, offset, current_ram_bin_load_addr,
                         elf_ram_bin_exec_addr, _Ram_dynbss_exec_addr - elf_ram_bin_exec_addr + elf_dynbss_size)
        print(
            f"ram.bin, 0x{current_ram_bin_load_addr:x}, 0x{elf_ram_bin_exec_addr:x}, 0x{_Ram_dynbss_exec_addr - elf_ram_bin_exec_addr + elf_dynbss_size:x}")
        current_ram_bin_load_addr += my_align(_Ram_dynbss_exec_addr - elf_ram_bin_exec_addr + elf_dynbss_size, 4)  
        offset += dyninfo_one_item_length

        so_text_load_addr = current_ram_bin_load_addr
        struct.pack_into('<III', dyninfo_data, offset,
                         current_ram_bin_load_addr, so_text_addr1, so_text_size)
        print(
            f"so_text, 0x{current_ram_bin_load_addr:x}, 0x{so_text_addr1:x}, 0x{so_text_size:x}")
        current_ram_bin_load_addr += my_align(so_text_size, 4)
        offset += dyninfo_one_item_length

        struct.pack_into('<III', dyninfo_data, offset,
                         current_ram_bin_load_addr, so_text_addr1 + my_align(so_text_size, 4), so_got_plt_size)
        print(
            f"so .got.plt, 0x{current_ram_bin_load_addr:x}, 0x{so_text_addr1 + my_align(so_text_size, 4):x}, 0x{so_got_plt_size:x}")
        current_ram_bin_load_addr += my_align(so_got_plt_size, 4)
        offset += dyninfo_one_item_length

        struct.pack_into('<III', dyninfo_data, offset,
                         current_ram_bin_load_addr, new_so_got_start_addr + so_got_size, new_so_data_bin_size)
        print(
            f"so_data, 0x{current_ram_bin_load_addr:x}, 0x{new_so_got_start_addr + so_got_size:x}, 0x{new_so_data_bin_size:x}")
        current_ram_bin_load_addr += my_align(new_so_data_bin_size, 4)
        offset += dyninfo_one_item_length

        struct.pack_into('<III', dyninfo_data, offset,
                         current_ram_bin_load_addr, new_so_got_start_addr + so_got_size + new_so_data_bin_size, so_bss_size)
        print(
            f"so bss, 0x{current_ram_bin_load_addr:x}, 0x{new_so_got_start_addr + so_got_size + new_so_data_bin_size:x}, 0x{so_bss_size:x}")
        offset += dyninfo_one_item_length

        elf_got_plt1_load_addr = current_ram_bin_load_addr
        struct.pack_into('<III', dyninfo_data, offset,
                         current_ram_bin_load_addr, 0, elf_got_plt_size)
        print(
            f"elf .got.plt1, 0x{current_ram_bin_load_addr:x}, 0, 0x{elf_got_plt_size:x}")
        current_ram_bin_load_addr += my_align(elf_got_plt_size, 4)
        offset += dyninfo_one_item_length

        elf_got_plt2_load_addr = current_ram_bin_load_addr
        struct.pack_into('<III', dyninfo_data, offset,
                         current_ram_bin_load_addr, 0, elf_got_plt_size)
        print(
            f"elf .got.plt2, 0x{current_ram_bin_load_addr:x}, 0, 0x{elf_got_plt_size:x}")
        current_ram_bin_load_addr += my_align(elf_got_plt_size, 4)
        offset += dyninfo_one_item_length

        so_got_plt1_load_addr = current_ram_bin_load_addr
        struct.pack_into('<III', dyninfo_data, offset, current_ram_bin_load_addr,
                         so_text_addr1 + so_text_size, so_got_plt_size)
        print(
            f"so .got.plt1, 0x{current_ram_bin_load_addr:x}, 0x{so_text_addr1 + so_text_size:x}, 0x{so_got_plt_size:x}")
        current_ram_bin_load_addr += my_align(so_got_plt_size, 4)
        offset += dyninfo_one_item_length

        so_got1_load_addr = current_ram_bin_load_addr
        struct.pack_into('<III', dyninfo_data, offset,
                         current_ram_bin_load_addr, new_so_got_start_addr, so_got_size)
        print(
            f"so .got1, 0x{current_ram_bin_load_addr:x}, 0x{new_so_got_start_addr:x}, 0x{so_got_size:x}")
        current_ram_bin_load_addr += my_align(so_got_size, 4)
        offset += dyninfo_one_item_length

        so_got2_load_addr = current_ram_bin_load_addr
        struct.pack_into('<III', dyninfo_data, offset,
                         current_ram_bin_load_addr, new_so_got_start_addr, so_got_size)
        print(
            f"so .got2, 0x{current_ram_bin_load_addr:x}, 0x{new_so_got_start_addr:x}, 0x{so_got_size:x}")
        current_ram_bin_load_addr += my_align(so_got_size, 4)
        offset += dyninfo_one_item_length

        # 1 addr, 4 bytes per addr for each item of modified_data_list
        so_data_change_info_size = 4 * len(modified_data_list)
        so_data_change_info_exec_addr = new_so_got_start_addr - so_data_change_info_size
        struct.pack_into('<III', dyninfo_data, offset,
                         current_ram_bin_load_addr, so_data_change_info_exec_addr, so_data_change_info_size)
        print(
            f"so data_change_info, 0x{current_ram_bin_load_addr:x}, 0x{so_data_change_info_exec_addr:x}, 0x{so_data_change_info_size:x}")
        current_ram_bin_load_addr += my_align(so_data_change_info_size, 4)
        offset += dyninfo_one_item_length

        # magic numbers for secondary boot to know if shared library is in ram.bin
        struct.pack_into('<III', dyninfo_data, 0, 0xABABABAB,
                         0xDEDEDEDE, my_align(current_ram_bin_load_addr, 0x1000))

        # elf_final_ram_bin_size = current_ram_bin_load_addr - elf_final_ram_bin_load_addr

        const_variables_to_be_modified = [
            ('SO_AVAILABLE', 1),
            ('HEAP_END', so_data_change_info_exec_addr),
            ('SO_TEXT_LOAD_ADDR', so_text_load_addr),
            ('SO_TEXT_EXEC_ADDR1', so_text_addr1),
            ('SO_TEXT_EXEC_ADDR2', so_text_addr2),
            ('SO_TEXT_SIZE', so_text_size),
            ('ELF_GOT_PLT1_LOAD_ADDR', elf_got_plt1_load_addr),
            ('ELF_GOT_PLT2_LOAD_ADDR', elf_got_plt2_load_addr),
            ('SO_GOT_PLT1_LOAD_ADDR', so_got_plt1_load_addr),
            ('SO_GOT_PLT_SIZE', so_got_plt_size),
            ('SO_GOT_MODIFIED', 1 if so_got_modified else 0),
            ('SO_GOT1_LOAD_ADDR', so_got1_load_addr),
            ('SO_GOT_EXEC_ADDR', new_so_got_start_addr),
            ('SO_GOT2_LOAD_ADDR', so_got2_load_addr),
            ('SO_GOT_SIZE', so_got_size),
            ('DYN_FOTA_FLASH_BASE', my_align(current_ram_bin_load_addr, 0x1000)),
            ('SO_DATA_CHANGE_INFO_ADDR', so_data_change_info_exec_addr),
            ('SO_DATA_CHANGE_INFO_COUNT', len(modified_data_list))
        ]
        print('SO_AVAILABLE DYN_FOTA_FLASH_BASE:{:x}'.format(my_align(current_ram_bin_load_addr, 0x1000)))

        for elf_sym_name, value in const_variables_to_be_modified:
            elf_symbols = elf_symtab.get_symbol_by_name(elf_sym_name)
            if not elf_symbols:
                raise XYException(f'cannot find {elf_sym_name} in elf')
            for elf_symbol in elf_symbols:
                if elf_symbol['st_info']['bind'] in ['STB_GLOBAL', 'STB_WEAK']:
                    break
            else:
                raise XYException(f'cannot find {elf_sym_name} in elf')
            struct.pack_into(
                '<I', elf_ram_bin_data, elf_symbol['st_value'] - elf_ram_bin_exec_addr, value)

        if modified_data_list:
            modified_data = bytearray(so_data_change_info_size)
            modified_data_offset = 0
            for data_addr in modified_data_list:
                struct.pack_into('<I', modified_data,
                                 modified_data_offset, data_addr)
                modified_data_offset += 4

        # new_ram_bin_path = os.path.join(
        #     os.path.dirname(elf_ram_bin_path), 'new_ram.bin')
        new_ram_bin_path = elf_ram_bin_path
        # os.replace(elf_ram_bin_path, os.path.join(
        #     os.path.dirname(elf_ram_bin_path), 'old_ram.bin'))

        def my_fill4(wf, length):
            if length % 4 != 0:
                wf.write(b'\xFF' * (my_align(length, 4) - length))

        def my_fill(wf, length):
                wf.write(b'\xFF' * length)

        with open(new_ram_bin_path, 'wb') as temp_f:
            temp_f.write(dyninfo_data)
            temp_f.write(elf_ram_bin_data)
            my_fill(temp_f, _Ram_dynbss_exec_addr - len(elf_ram_bin_data) - elf_ram_bin_exec_addr)
            temp_f.write(elf_dynbss_data)
            my_fill4(temp_f, elf_dynbss_size)
            with open(so_text_path, 'rb') as so_text_f:
                so_text_data = so_text_f.read()
                temp_f.write(so_text_data)
            temp_f.write(new_so_got_plt2_data)
            temp_f.write(new_so_data_bin_data)
            my_fill4(temp_f, new_so_data_bin_size)
            temp_f.write(new_elf_got_plt1_data)
            temp_f.write(new_elf_got_plt2_data)
            temp_f.write(new_so_got_plt1_data)
            if so_got:
                temp_f.write(new_so_got1_data)
                temp_f.write(new_so_got2_data)
            if modified_data_list:
                temp_f.write(modified_data)

        print(f"add-symbol-file {so_filename} -s .text 0x{so_text_addr1:x} -s .rodata 0x{new_so_got_start_addr + so_got_size:x} -s .data 0x{new_so_got_start_addr + so_got_size + so_rodata_size_aligned:x} -s .bss 0x{new_so_got_start_addr + so_got_size + new_so_data_bin_size:x}")
        print(f"add-symbol-file {so_filename} -s .text 0x{so_text_addr2:x} -s .rodata 0x{new_so_got_start_addr + so_got_size:x} -s .data 0x{new_so_got_start_addr + so_got_size + so_rodata_size_aligned:x} -s .bss 0x{new_so_got_start_addr + so_got_size + new_so_data_bin_size:x}")

    return True


def main():
    if len(sys.argv) < 11:
        print('no enough arguments...')
        sys.exit(-1)
    print(' '.join(sys.argv))
    elf_path = sys.argv[1]
    so_path = sys.argv[2]
    heap_end_addr = int(sys.argv[3], 16)
    so_ram_addr = int(sys.argv[4], 16)
    so_ram_size = int(sys.argv[5], 16)
    ban_write_flash = int(sys.argv[6], 10)
    ban_write_flash_remain_len = int(sys.argv[7], 10)
    so_text_path = sys.argv[8]
    flash_bin_path = sys.argv[9]
    flash_bin_exec_addr = int(sys.argv[10], 16)
    ram_bin_path = sys.argv[11]
    ram_bin_exec_addr = int(sys.argv[12], 16)
    start_time = time.perf_counter()
    ret = process(elf_path, so_path, heap_end_addr, so_ram_addr, so_ram_size, ban_write_flash, ban_write_flash_remain_len, so_text_path,
                  flash_bin_path, flash_bin_exec_addr, ram_bin_path, ram_bin_exec_addr)
    # if not ret:
    #     sys.exit(-1)
    end_time = time.perf_counter()
    print(f"so analyzing done, {end_time-start_time:.3f}s taken!")


if __name__ == '__main__':
    main()
