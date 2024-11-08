# -*- encoding: utf-8 -*-
'''
@File    :   xykeil.py
@Time    :   2022/09/16 10:11:22
@Author  :   Wang Han
@Version :   1.0
@Contact :   wangh@xinyisemi.com
'''

import argparse
import os
import re
import shutil
import subprocess
import sys
import xml.etree.ElementTree as etree
from xml.etree.ElementTree import SubElement

from TOOLS.build.settings import XYSettingsHandler

BUILD_FILE_NAME = 'CMakeLists.txt'
PRJ_SETTINGS_FILE_NAME = '.prj_settings'
CMAKE_TOOLCHAIN_FILE = r'TOOLS/cmake/arm-gcc-toolchain.cmake'
KEIL_TOOLS_DIR = r'TOOLS/build/keil'

C_DEFINES_INFO_FILE = 'c_defines.txt'
INCLUDE_DIRECTORIES_INFO_FILE = 'include_directories.txt'
SOURCES_INFO_FILE = 'sources.txt'
LINK_OPTIONS_INFO_FILE = 'link_options.txt'

LINKER_SCRIPT_DIR = 'TOOLS/linkscript'

LIB_DIR = 'LIB'

TEMPLATE_UVPROJX_FILE = 'TOOLS/keil/template.uvprojx'
TEMPLATE_UVOPTX_FILE = 'TOOLS/keil/template.uvoptx'

C_MISC_CONTROLS = '--diag_suppress=66,188,1254,1295,A1609W --gnu'
ASM_MISC_CONTROLS = '--diag_suppress=1609'
LINKER_MISC_CONTROLS = '--datacompressor off --entry Reset_Handler --diag_suppress=6314'

SOURCE_FILES_MAPPING = {'startup_cm3.s': 'startup_cm3_keil.s'}

DEFINE_TO_SCT_MAPPING = (
    'FLASH_2M', 'template_2m.sct', 'template_4m.sct')

DEFINE_TO_PACK_PROG_MAPPING = (
    'FLASH_2M', 'm3ld_2m_keil.exe', 'm3ld_4m_keil.exe')


class XYMDK5ProjectConverter:

    def __init__(self, base_dir, output_dir):
        self.base_dir = os.path.abspath(base_dir)
        self.output_dir = os.path.abspath(output_dir)
        if os.path.exists(self.output_dir):
            shutil.rmtree(self.output_dir)
        os.makedirs(self.output_dir)
        self.cmake_build_dir = os.path.join(self.base_dir, '.build')
        self.ignore_dir = os.path.join(self.base_dir, '.ignore')
        self.cmake_toolchain_file = os.path.join(
            self.base_dir, CMAKE_TOOLCHAIN_FILE)

        self.c_defines = []
        self.include_directories = []
        self.sources = []
        self.irom_start = 0
        self.irom_size = 0
        self.iram_start = 0
        self.iram_size = 0

    def get_filetype(self, filename):
        ext = os.path.splitext(filename)[-1]
        if ext in ['.cpp', '.cxx']:
            return 8
        if ext in ['.c', '.C']:
            return 1
        if ext in ['.s', '.S']:
            return 2
        if ext == '.h':
            return 5
        if ext in ['.a', '.lib']:
            return 4
        if ext == '.o':
            return 3
        return 5

    def prettyXml(self, element, indent, newline, level=0):
        if element:
            if element.text == None or element.text.isspace():
                element.text = newline + indent * (level + 1)
            else:
                element.text = newline + indent * \
                    (level + 1) + element.text.strip() + \
                    newline + indent * (level + 1)
        # else:
            #element.text = newline + indent * (level + 1) + element.text.strip() + newline + indent * level
        temp = list(element)
        for subelement in temp:
            if temp.index(subelement) < (len(temp) - 1):
                subelement.tail = newline + indent * (level + 1)
            else:
                subelement.tail = newline + indent * level
            self.prettyXml(subelement, indent, newline, level=level + 1)

    def is_xy_project(self):
        if not os.path.exists(os.path.join(self.base_dir, BUILD_FILE_NAME)):
            return False
        if not os.path.exists(os.path.join(self.base_dir, PRJ_SETTINGS_FILE_NAME)):
            return False
        return True

    def clean_project(self):
        if os.path.exists(self.cmake_build_dir):
            shutil.rmtree(self.cmake_build_dir)
        if os.path.exists(self.ignore_dir):
            shutil.rmtree(self.ignore_dir)

    def parse_prj_settings(self):
        myXYSettingsHandler = XYSettingsHandler(
            self.base_dir, os.path.join(self.base_dir, PRJ_SETTINGS_FILE_NAME), self.ignore_dir)
        myXYSettingsHandler.parse()

    def check_prj_settings(self):
        original_prj_settings_file_path = os.path.abspath(
            os.path.join(self.base_dir, PRJ_SETTINGS_FILE_NAME))
        if not os.path.exists(original_prj_settings_file_path):
            raise Exception(f'cannot find {original_prj_settings_file_path}')
        original_mtime = os.path.getmtime(original_prj_settings_file_path)

        backed_prj_settings_file_path = os.path.abspath(
            os.path.join(self.ignore_dir, PRJ_SETTINGS_FILE_NAME))
        if not os.path.exists(backed_prj_settings_file_path):
            shutil.copy2(original_prj_settings_file_path,
                         backed_prj_settings_file_path)
            self.parse_prj_settings()
            return
        backed_mtime = os.path.getmtime(backed_prj_settings_file_path)

        if backed_mtime != original_mtime:
            shutil.copy2(original_prj_settings_file_path,
                         backed_prj_settings_file_path)
            self.parse_prj_settings()
            return

        with open(original_prj_settings_file_path, 'rb') as f:
            original_prj_settings_content = f.read()
        with open(backed_prj_settings_file_path, 'rb') as f:
            backed_prj_settings_content = f.read()
        if original_prj_settings_content != backed_prj_settings_content:
            shutil.copy2(original_prj_settings_file_path,
                         backed_prj_settings_file_path)
            self.parse_prj_settings()
            return

    def cmake_project(self):
        cmake_cmd = ['cmake', '-G', 'Unix Makefiles', '-S', self.base_dir, '-B', self.cmake_build_dir,
                     f'-DCMAKE_TOOLCHAIN_FILE={self.cmake_toolchain_file}', '-DSAVE_OTHER_INFO=1']
        with subprocess.Popen(cmake_cmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE) as p:
            # p.wait()
            stdout, stderr = p.communicate()
            if p.returncode != 0:
                print(stderr)
                return False
        return True

    def get_gcc_info(self):
        c_defines = self.c_defines
        with open(os.path.join(self.ignore_dir, C_DEFINES_INFO_FILE), 'r') as f:
            for line in f:
                c_defines.append(os.path.normpath(line.strip()))
        include_directories = self.include_directories
        with open(os.path.join(self.ignore_dir, INCLUDE_DIRECTORIES_INFO_FILE), 'r') as f:
            for line in f:
                include_directories.append(os.path.normpath(line.strip()))
        sources = self.sources
        with open(os.path.join(self.ignore_dir, SOURCES_INFO_FILE), 'r') as f:
            for line in f:
                sources.append(os.path.normpath(line.strip()))
        with open(os.path.join(self.ignore_dir, LINK_OPTIONS_INFO_FILE), 'r') as f:
            for line in f:
                line = line.strip()
                if line.startswith('-T') and line.endswith('.ld') and line[len('-T'):] != 'sections.ld':
                    memory_ld_file = line[len('-T'):]
                    break
        # print(self.c_defines)
        # print(self.include_directories)
        # print(self.sources)
        flash_prog = re.compile(
            '^\s*FLASH\s*\([xrw]+\)\s*:\s*ORIGIN\s*=\s*([a-zA-Z0-9]*),\s*LENGTH\s*=\s*([a-zA-Z0-9]*).*$')
        ram_prog = re.compile(
            '^\s*RAM\s*\([xrw]+\)\s*:\s*ORIGIN\s*=\s*([a-zA-Z0-9]*),\s*LENGTH\s*=\s*([a-zA-Z0-9]*).*$')
        with open(os.path.join(self.base_dir, LINKER_SCRIPT_DIR, memory_ld_file), 'r') as f:
            for line in f:
                line = line.strip()
                flash_match_obj = flash_prog.match(line)
                if flash_match_obj:
                    irom_start, irom_size = flash_match_obj.groups()
                    self.irom_start = int(irom_start, base=16 if irom_start.startswith(
                        '0x') or irom_start.startswith('0X') else 10)
                    self.irom_size = int(irom_size, base=16 if irom_size.startswith(
                        '0x') or irom_size.startswith('0X') else 10)
                ram_match_obj = ram_prog.match(line)
                if ram_match_obj:
                    iram_start, iram_size = ram_match_obj.groups()
                    self.iram_start = int(iram_start, base=16 if iram_start.startswith(
                        '0x') or iram_start.startswith('0X') else 10)
                    self.iram_size = int(iram_size, base=16 if iram_size.startswith(
                        '0x') or iram_size.startswith('0X') else 10)
        # print(
        #     f'0x{self.irom_start:08x} 0x{self.irom_size:x} 0x{self.iram_start:08x} 0x{self.iram_size:x}')

    def do_convert_to_mdk5_project(self, mdk5_project_name):
        project_uvprojx_path = os.path.join(
            self.output_dir, f'{mdk5_project_name}.uvprojx')
        project_uvprojx_dir = self.output_dir
        template_tree = etree.parse(os.path.join(
            self.base_dir, TEMPLATE_UVPROJX_FILE))

        root = template_tree.getroot()
        target_name_element = template_tree.find('Targets/Target/TargetName')
        target_name_element.text = mdk5_project_name
        target_output_name_element = template_tree.find(
            'Targets/Target/TargetOption/TargetCommonOption/OutputName')
        target_output_name_element.text = mdk5_project_name
        groups = template_tree.find('Targets/Target/Groups')
        groups.clear()

        name_group_element_dict = {}
        temp_base_dir_path_len = len(self.base_dir) + 1
        for source_file_path in self.sources:
            source_file_basename = os.path.basename(source_file_path)
            if source_file_basename in SOURCE_FILES_MAPPING:
                source_file_path = os.path.join(os.path.dirname(
                    source_file_path), SOURCE_FILES_MAPPING[source_file_basename])
            temp_paths = source_file_path[temp_base_dir_path_len:].split('\\')
            if len(temp_paths) >= 3:
                group_name = f'{temp_paths[0]}\\{temp_paths[1]}'
            else:
                group_name = temp_paths[0]
            if group_name in name_group_element_dict:
                group_element = name_group_element_dict[group_name]
                files_element = group_element.find('Files')
            else:
                group_element = SubElement(groups, 'Group')
                name_group_element_dict[group_name] = group_element
                group_name_element = SubElement(
                    group_element, 'GroupName')
                group_name_element.text = group_name
                files_element = SubElement(group_element, 'Files')
            file_element = SubElement(files_element, 'File')
            file_name_element = SubElement(file_element, 'FileName')
            file_name_element.text = temp_paths[-1]
            file_type_element = SubElement(file_element, 'FileType')
            file_type_element.text = str(self.get_filetype(temp_paths[-1]))
            file_path_element = SubElement(file_element, 'FilePath')
            file_path_element.text = os.path.relpath(
                source_file_path, project_uvprojx_dir)
        lib_dir = os.path.join(self.base_dir, LIB_DIR)
        if os.path.exists(lib_dir):
            lib_dir_basename = os.path.basename(lib_dir)
            names = os.listdir(lib_dir)
            lib_names = [
                name for name in names if os.path.splitext(name)[-1] == '.a']
            if lib_names:
                group_element = SubElement(groups, 'Group')
                group_name_element = SubElement(
                    group_element, 'GroupName')
                group_name_element.text = lib_dir_basename
                files_element = SubElement(group_element, 'Files')
                for lib_name in lib_names:
                    file_element = SubElement(files_element, 'File')
                    file_name_element = SubElement(file_element, 'FileName')
                    file_name_element.text = lib_name
                    file_type_element = SubElement(file_element, 'FileType')
                    file_type_element.text = str(self.get_filetype(lib_name))
                    file_path_element = SubElement(file_element, 'FilePath')
                    file_path_element.text = os.path.relpath(
                        os.path.join(lib_dir, lib_name), project_uvprojx_dir)

        target_arm_ads_element = template_tree.find(
            'Targets/Target/TargetOption/TargetArmAds')
        temp_include_directories = [os.path.relpath(
            include_directory_path, project_uvprojx_dir) for include_directory_path in self.include_directories]
        cads_various_controls_element = target_arm_ads_element.find(
            'Cads/VariousControls')
        c_include_path_element = cads_various_controls_element.find(
            'IncludePath')
        c_include_path_element.text = ';'.join(temp_include_directories)
        c_misc_controls_element = cads_various_controls_element.find(
            'MiscControls')
        c_misc_controls_element.text = C_MISC_CONTROLS
        c_define_element = cads_various_controls_element.find('Define')
        c_define_element.text = ' '.join(self.c_defines)

        on_chip_memories_element = target_arm_ads_element.find(
            'ArmAdsMisc/OnChipMemories')
        ocr_rvct4_element = on_chip_memories_element.find('OCR_RVCT4')
        ocr_rvct4_start_address_element = ocr_rvct4_element.find(
            'StartAddress')
        ocr_rvct4_start_address_element.text = f'0x{self.irom_start:08x}'
        ocr_rvct4_size_element = ocr_rvct4_element.find('Size')
        ocr_rvct4_size_element.text = f'0x{self.irom_size:x}'
        ocr_rvct9_element = on_chip_memories_element.find('OCR_RVCT9')
        ocr_rvct9_start_address_element = ocr_rvct9_element.find(
            'StartAddress')
        ocr_rvct9_start_address_element.text = f'0x{self.iram_start:08x}'
        ocr_rvct9_size_element = ocr_rvct9_element.find('Size')
        ocr_rvct9_size_element.text = f'0x{self.iram_size:x}'

        aads_various_controls_element = target_arm_ads_element.find(
            'Aads/VariousControls')
        a_include_path_element = aads_various_controls_element.find(
            'IncludePath')
        a_include_path_element.text = ';'.join(temp_include_directories)
        a_misc_controls_element = aads_various_controls_element.find(
            'MiscControls')
        a_misc_controls_element.text = ASM_MISC_CONTROLS
        a_define_element = aads_various_controls_element.find('Define')
        a_define_element.text = ' '.join(self.c_defines)

        ldads_element = target_arm_ads_element.find('LDads')
        l_scatter_file_element = ldads_element.find('ScatterFile')
        sct_define = DEFINE_TO_SCT_MAPPING[0]
        sct_file = DEFINE_TO_SCT_MAPPING[1]
        for c_define in self.c_defines:
            if sct_define in c_define:
                if '=' in c_define:
                    c_define_value = c_define.split('=')[-1]
                    if int(c_define_value.strip()) == 0:
                        sct_file = DEFINE_TO_SCT_MAPPING[2]
                break
        template_sct_path = os.path.join(
            self.base_dir, LINKER_SCRIPT_DIR, sct_file)
        project_sct_path = os.path.join(
            self.output_dir, f'{mdk5_project_name}.sct')
        shutil.copy2(template_sct_path, project_sct_path)
        l_scatter_file_element.text = os.path.relpath(
            project_sct_path, project_uvprojx_dir)
        l_misc_element = ldads_element.find('Misc')
        l_misc_element.text = LINKER_MISC_CONTROLS

        keil_tools_dir = os.path.join(self.output_dir, 'tools')
        shutil.copytree(os.path.join(
            self.base_dir, KEIL_TOOLS_DIR), keil_tools_dir)
        user_prog2_name_element = template_tree.find(
            'Targets/Target/TargetOption/TargetCommonOption/AfterMake/UserProg2Name')
        pack_define = DEFINE_TO_PACK_PROG_MAPPING[0]
        pack_prog = DEFINE_TO_PACK_PROG_MAPPING[1]
        for c_define in self.c_defines:
            if pack_define in c_define:
                if '=' in c_define:
                    c_define_value = c_define.split('=')[-1]
                    if int(c_define_value.strip()) == 0:
                        pack_prog = DEFINE_TO_PACK_PROG_MAPPING[2]
                break
        user_prog2_name_element.text = f'.\\{os.path.relpath(os.path.join(keil_tools_dir, pack_prog), self.output_dir)} {user_prog2_name_element.text}'

        self.prettyXml(root, '  ', '\n')
        with open(project_uvprojx_path, 'w', encoding='utf-8') as f:
            f.write('<?xml version="1.0" encoding="UTF-8" standalone="no" ?>\n')
            f.write(etree.tostring(root, encoding='utf-8',
                                   short_empty_elements=False).decode())

        self.do_convert_to_mdk5_project_uvoptx(mdk5_project_name)

    def do_convert_to_mdk5_project_uvoptx(self, mdk5_project_name):
        project_uvoptx_path = os.path.join(
            self.output_dir, f'{mdk5_project_name}.uvoptx')
        project_uvoptx_dir = self.output_dir
        template_tree = etree.parse(os.path.join(
            self.base_dir, TEMPLATE_UVOPTX_FILE))

        root = template_tree.getroot()
        target_name_element = template_tree.find('Target/TargetName')
        target_name_element.text = mdk5_project_name

        self.prettyXml(root, '  ', '\n')
        with open(project_uvoptx_path, 'w', encoding='utf-8') as f:
            f.write('<?xml version="1.0" encoding="UTF-8" standalone="no" ?>\n')
            f.write(etree.tostring(root, encoding='utf-8',
                                   short_empty_elements=False).decode())

    def convert_to_mdk5_project(self, mdk5_project_name=''):
        if not self.is_xy_project():
            print(
                f'{self.base_dir} is probably not a xinyi project, cannot convert it to a mdk5 project!')
            return
        self.clean_project()
        if not os.path.exists(self.ignore_dir):
            os.mkdir(self.ignore_dir)
        self.check_prj_settings()
        self.cmake_project()
        self.get_gcc_info()
        if not mdk5_project_name:
            mdk5_project_name = os.path.basename(self.base_dir)
        self.do_convert_to_mdk5_project(mdk5_project_name)

        self.clean_project()


def main():

    parser = argparse.ArgumentParser()

    parser.add_argument(
        '-b', help='project base directory, defaults to current directory.', type=str, default='.')
    parser.add_argument(
        '-o', help='output directory, defaults to ./MDK-ARM', type=str, default='./MDK-ARM')
    parser.add_argument(
        '-n', help='output project name, defaults to current directory\'s name.', type=str, default='')

    args = parser.parse_args()

    myXYMDK5ProjectConverter = XYMDK5ProjectConverter(args.b, args.o)
    myXYMDK5ProjectConverter.convert_to_mdk5_project(args.n)


if __name__ == '__main__':
    main()
