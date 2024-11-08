# -*- encoding: utf-8 -*-

import os
import sys
import shutil

xy_soc_ver_values = [a for a in range(7)]

xy_soc_ver_dir = '.xy_soc_ver_output'


def main():
    if not os.path.exists(xy_soc_ver_dir):
        return
    for v in xy_soc_ver_values:
        src_library_dir = os.path.join(
            xy_soc_ver_dir, f'XY_SOC_VER={v}', 'library')
        dst_library_dir = os.path.join('LIB', f'XY_SOC_VER={v}')
        src_loginfo_dir = os.path.join(
            xy_soc_ver_dir, f'XY_SOC_VER={v}', 'loginfo')
        dst_loginfo_dir = os.path.join('loginfo', f'XY_SOC_VER={v}')
        if os.path.exists(src_library_dir):
            if os.path.exists(dst_library_dir):
                shutil.rmtree(dst_library_dir)
            shutil.copytree(src_library_dir, dst_library_dir)

        if os.path.exists(src_loginfo_dir):
            if os.path.exists(dst_loginfo_dir):
                shutil.rmtree(dst_loginfo_dir)
            shutil.copytree(src_loginfo_dir, dst_loginfo_dir)
    shutil.rmtree(xy_soc_ver_dir)


if __name__ == '__main__':
    main()
