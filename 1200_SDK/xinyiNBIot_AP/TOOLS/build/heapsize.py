# -*- encoding: utf-8 -*-

import os
import struct
import sys
import time

from elftools.elf.elffile import ELFFile
from elftools.elf.enums import ENUM_RELOC_TYPE_ARM
from elftools.elf.relocation import RelocationSection
from elftools.elf.sections import Symbol

def process_elf(elf_filename):
    with open(elf_filename, 'rb') as elf_f:
        elffile = ELFFile(elf_f)
        elf_symtab = elffile.get_section_by_name('.symtab')

        for elf_symbol in elf_symtab.iter_symbols():
            if elf_symbol.name == "_Ram_Total":
                _Ram_Total = elf_symbol['st_value']
            if elf_symbol.name == "_Heap_Begin":
                _Heap_Begin = elf_symbol['st_value']
            if elf_symbol.name == "_Heap_Limit":
                _Heap_Limit = elf_symbol['st_value']
            if elf_symbol.name == "_Flash_Addr":
                _Flash_Addr = elf_symbol['st_value']
            if elf_symbol.name == "_Flash_Total":
                _Flash_Total = elf_symbol['st_value']
            if elf_symbol.name == "_BAK_MEM_FOTA_FLASH_BASE":
                _Fota_Flash_Base = elf_symbol['st_value']

    return _Ram_Total, _Heap_Limit - _Heap_Begin, _Flash_Addr, _Flash_Total, _Fota_Flash_Base

def process_so(so_filename):
    
    with open(so_filename, 'rb') as so_f:
        sofile = ELFFile(so_f)
        so_got_size = 0
        so_got = sofile.get_section_by_name('.got')
        if so_got:
            so_got_size = so_got['sh_size']

        so_data_section = sofile.get_section_by_name('.data')
        so_data_size = so_data_section['sh_size']

        so_bss_section = sofile.get_section_by_name('.bss')
        so_bss_size = so_bss_section['sh_size']

        so_rodata_section = sofile.get_section_by_name('.rodata')
        so_rodata_size = so_rodata_section['sh_size']

        return so_got_size + so_data_size + so_bss_size + so_rodata_size
    
def process_ingore_file(filename):
    with open(filename, "r") as f:
        for line in f:
            if r'SO_AVAILABLE DYN_FOTA_FLASH_BASE' in line:
                return int(line.split(":")[1], 16)
    return 0

def main():
    # print(' '.join(sys.argv))
    ram_total, heap_size, _Flash_Addr, _Flash_Total, _Fota_Flash_Base = process_elf(sys.argv[1])
    cp_used_apram = int(sys.argv[2], 10) * 0x400
    fota_backup_size = 0x1000 
    so_size = 0

    if len(sys.argv) > 3:
        so_size = process_so(sys.argv[3])
        ret = process_ingore_file(sys.argv[4])
        if ret:
            _Fota_Flash_Base = ret

    fota_size = _Flash_Total - (_Fota_Flash_Base - _Flash_Addr) - fota_backup_size
    print(f"FLASH space (AP bin+OTA package): 0x{_Flash_Total:x}, Address for OTA package storage: 0x{_Fota_Flash_Base:x}, Length: 0x{fota_size:x}")
    if fota_size < (150 * 1024):
        print(r"WARNING!!!OTA package space < 150K bytes, Cannot guarantee FOTA successfully!")
    Remain_heap_size = heap_size - cp_used_apram - so_size
    print(f"Total SRAM space: 0x{ram_total + 0x80:x}, Lended by CP: 0x{cp_used_apram:x}(or log_size), Have used :0x{ram_total + 0x80 - Remain_heap_size:x}. SRAM space(Code+Heap) reserved for user :0x{Remain_heap_size:x}")
    if Remain_heap_size < (8 * 1024):
        print(r"WARNING!!!Heap space not enough, maybe halt!")

if __name__ == '__main__':
    main()
