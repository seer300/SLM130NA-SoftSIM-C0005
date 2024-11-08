# -*- encoding: utf-8 -*-

import os
import struct
import sys
import time

from elftools.elf.elffile import ELFFile
from elftools.elf.enums import ENUM_RELOC_TYPE_ARM
from elftools.elf.relocation import RelocationSection
from elftools.elf.sections import Symbol

class XYException(Exception):
    pass

class Sym:
    def __init__(self, func_name, bind, file):
        self.file = file
        self.func_name = func_name
        self.bind = bind

    def __str__(self):
        # 使用 print 打印该对象是，会调用该函数
        return 'func_name: {:s}, bind: {:s}, file: {:s}'.format(self.func_name, self.bind, self.file)

class Lib_Sym_Check:
    def __init__(self, file):
        self.file_set = list()
        self.__get_file_set(file)

    def __get_file_set(self, file):
        with open(file, '+r') as f:
            for line in f:
                if line.endswith('\n'):
                    self.file_set.append(line[0:-1])
                else:
                    self.file_set.append(line)

    def sym_check(self):
        sym_dict = {}
        sym_list = []
        for file in self.file_set:
            with open(file, 'rb') as elf_f:
                elffile = ELFFile(elf_f)
                elf_symtab = elffile.get_section_by_name('.symtab')

                for elf_symbol in elf_symtab.iter_symbols():
                    if elf_symbol['st_info']['bind'] in ['STB_GLOBAL', 'STB_WEAK'] and elf_symbol['st_info']['type'] in ['STT_FUNC'] and elf_symbol['st_shndx'] != 'SHN_UNDEF':
                        if sym_dict.get(elf_symbol.name) == None:
                            sym_dict[elf_symbol.name] = Sym(elf_symbol.name, elf_symbol['st_info']['bind'], os.path.basename(file)[0:-2])
                        else:
                            sym_list.append(sym_dict[elf_symbol.name])
                            sym_list.append(Sym(elf_symbol.name, elf_symbol['st_info']['bind'], os.path.basename(file)[0:-2]))            
        for sym in sym_list:
            print(sym)

        if len(sym_list):
            raise XYException(f"error lib {sys.argv[1]} func appears multiple times! ")
       
def main():
    print(f"lib {sys.argv[1]} analyzing")
    start_time = time.perf_counter()
    lib = Lib_Sym_Check(sys.argv[2])
    lib.sym_check()
    end_time = time.perf_counter()
    print(f"lib analyzing done, {end_time-start_time:.3f}s taken!")

if __name__ == '__main__':
    main()
