# -*- encoding: utf-8 -*-

import configparser
import re
import struct
import os
import sys
import hmac
from enum import Enum, IntEnum, unique


class XinyiDevelop(Enum):
    align_bin = True
    bin_alignment = 4
    bin_fill = '0xFF'

    load_addr_alignment = '0x1000'


class XinyiGlobal(Enum):
    HMAC_KEY = 'XY1100'
    HASH_ALGORITHM = 'SHA1'
    MAGIC_NUM = 5847
    VERSION = 1
    ALLINONE_NAME = 'cp.img'


@unique
class IMAGE_CORE(IntEnum):
    unknown = 0
    arm = 1
    dsp = 2
    nv = 3
    secondary_boot_prime = 4
    secondary_boot_backup = 5
    fast_arm = 6
    fast_dsp = 7
    fast_nv = 8
    fast_secondary_boot_prime = 9
    fast_secondary_boot_backup = 10
    secondary_boot_prime_flash = 16
    secondary_boot_backup_flash = 17
    user_nv = 20
    rf_nv = 21


@unique
class IMAGE_TYPE(IntEnum):
    unknown = 0
    hex_type = 1
    bin_type = 2


class MyImageInfo:
    def __init__(self, image_core_str, image_load_addr_str, image_exec_addr_str, image_name_str, image_type_str, force_exist_str):
        self.image_core = int(IMAGE_CORE[image_core_str])
        self.image_load_addr = int(image_load_addr_str, 16)
        self.image_exec_addr = int(image_exec_addr_str, 16)
        self.image_name = image_name_str
        self.image_type = int(IMAGE_TYPE[image_type_str + '_type'])
        self.force_exist = True if force_exist_str.lower() == 'true' else False


class MyXYPacker:

    def __init__(self, ini_file):
        self.parse_conf(ini_file)

    def parse_conf(self, ini_file):
        conf = configparser.ConfigParser()
        conf.read(ini_file)
        self.HMAC_KEY = conf.get('global', 'HMAC_KEY',
                                 fallback=XinyiGlobal.HMAC_KEY.value)
        self.HASH_ALGORITHM = conf.get(
            'global', 'HASH_ALGORITHM', fallback=XinyiGlobal.HASH_ALGORITHM.value)
        self.MAGIC_NUM = conf.getint(
            'global', 'MAGIC_NUM', fallback=XinyiGlobal.MAGIC_NUM.value)
        self.VERSION = conf.getint(
            'global', 'VERSION', fallback=XinyiGlobal.VERSION.value)
        self.ALLINONE_NAME = conf.get(
            'global', 'ALLINONE_NAME', fallback=XinyiGlobal.ALLINONE_NAME.value)

        self.align_bin = conf.getboolean(
            'develop', 'align_bin', fallback=XinyiDevelop.align_bin.value)
        self.bin_alignment = conf.getint(
            'develop', 'bin_alignment', fallback=XinyiDevelop.bin_alignment.value)
        self.bin_fill = int(conf.get('develop', 'bin_fill',
                                     fallback=XinyiDevelop.bin_fill.value), 16)

        self.load_addr_alignment = int(conf.get(
            'develop', 'load_addr_alignment', fallback=XinyiDevelop.load_addr_alignment.value), 16)

        self.image_infos = self.get_image_info(conf)

    def get_image_info(self, conf):
        image_sections = [section for section in conf.sections(
        ) if re.match(r'^image\d+$', section)]

        image_infos = []
        for image_section in image_sections:
            image_info = MyImageInfo(conf.get(image_section, 'image_core'), conf.get(image_section, 'image_load_addr'),
                                     conf.get(image_section, 'image_exec_addr'), conf.get(
                                         image_section, 'image_name'), conf.get(image_section, 'image_type'),
                                     conf.get(image_section, 'force_exist'))
            image_infos.append(image_info)

        return image_infos

    def generate_allinone_file(self, image_infos, allinone_file):
        allinone_bin_content = struct.pack('<L', len(image_infos))
        total_hash = hmac.new(self.HMAC_KEY.encode(),
                              digestmod=self.HASH_ALGORITHM)
        with open(allinone_file, 'wb+') as allinone_fp:
            for image_info in image_infos:
                image_hash = hmac.new(
                    self.HMAC_KEY.encode(), digestmod=self.HASH_ALGORITHM)
                image_hash.update(image_info.image_bin_content)
                image_sha_digest = image_hash.digest()

                allinone_bin_content += struct.pack('<LLLLL', image_info.image_core, image_info.image_load_addr,
                                                    image_info.image_exec_addr, image_info.image_type, len(image_info.image_name))
                allinone_bin_content += image_info.image_name.encode()
                allinone_bin_content += struct.pack('<L',
                                                    len(image_info.image_bin_content))
                allinone_bin_content += image_sha_digest

            for image_info in image_infos:
                allinone_bin_content += image_info.image_bin_content

            total_hash.update(allinone_bin_content)
            total_sha_digest = total_hash.digest()
            allinone_fp.write(struct.pack('<LL', self.MAGIC_NUM, self.VERSION))
            allinone_fp.write(total_sha_digest)
            allinone_fp.write(allinone_bin_content)

    def start_packing(self, bins_dir):
        prev_load_addr = 0
        prev_length = 0
        for image_info in self.image_infos:
            image_path = os.path.join(bins_dir, image_info.image_name)
            if image_info.force_exist and not os.path.exists(image_path):
                print(f' cannot find f{image_info.image_name}')
                return False
            if image_info.image_load_addr == 0:
                temp_addr = prev_load_addr + prev_length
                image_info.image_load_addr = (
                    temp_addr + self.load_addr_alignment - 1) & (~(self.load_addr_alignment - 1))
            with open(image_path, 'rb') as f:
                image_bin_content = f.read()
            if self.align_bin:
                image_bin_content_length = len(image_bin_content)
                remainder = image_bin_content_length % self.bin_alignment
                if remainder != 0:
                    fill_len = self.bin_alignment - remainder
                    fill_byte = self.bin_fill.to_bytes(1, byteorder='little')
                    image_info.image_bin_content = image_bin_content + fill_byte * fill_len
                else:
                    image_info.image_bin_content = image_bin_content
            prev_load_addr = image_info.image_load_addr
            prev_length = len(image_info.image_bin_content)
        self.generate_allinone_file(
            self.image_infos, os.path.join(bins_dir, self.ALLINONE_NAME))
        return True


def main():

    if len(sys.argv) < 3:
        print('no enough arguments, exit...')
        sys.exit()

    bins_dir = sys.argv[1]
    ini_file = sys.argv[2]

    print('start generating allinone')
    if MyXYPacker(ini_file).start_packing(bins_dir):
        print('allinone generated')


if __name__ == '__main__':
    main()
