# -*- encoding: utf-8 -*-

import os
import shutil
import subprocess
import sys

xy_soc_ver_values = [a for a in range(7)]

xybuild_allbins_dir = 'Allbins'

xy_soc_ver_output_dir = '.xy_soc_ver_output'


def execute_cmd(cmd, sucessful_msg=''):
    with subprocess.Popen(cmd, cwd=os.path.dirname(os.path.abspath(__file__)), text=True, stdout=sys.stdout, stderr=sys.stderr) as p:
        print(' '.join(p.args))
        p.wait()
        if p.returncode != 0:
            raise Exception(f'failed to execute command {cmd}')
        else:
            if sucessful_msg:
                print(sucessful_msg)


def xy_soc_ver_build():
    if os.path.exists(xy_soc_ver_output_dir):
        names = os.listdir(xy_soc_ver_output_dir)
        for name in names:
            temp_path = os.path.join(xy_soc_ver_output_dir, name)
            shutil.rmtree(temp_path)
    else:
        os.mkdir(xy_soc_ver_output_dir)
    xybuild_clean_cmd = [f'{sys.executable}', 'xybuild.py', 'clean']
    for v in xy_soc_ver_values:
        xybuild_all_cmd = [f'{sys.executable}',
                           'xybuild.py', 'all', '-D', f'XY_SOC_VER={v}']
        execute_cmd(xybuild_clean_cmd)
        execute_cmd(xybuild_all_cmd)
        cur_output_dir = os.path.join(xy_soc_ver_output_dir, f'XY_SOC_VER={v}')
        shutil.copytree(xybuild_allbins_dir, cur_output_dir)


def main():
    xy_soc_ver_build()


if __name__ == '__main__':
    main()
