# -*- encoding: utf-8 -*-
import os
import sys
import shutil
import subprocess
import argparse
import multiprocessing as mp
import platform

if getattr(sys, 'frozen', False):
    BASE_DIR = os.path.dirname(os.path.abspath(sys.executable))
    RUN_AS_EXE = True
else:
    BASE_DIR = os.path.dirname(os.path.abspath(__file__))
    RUN_AS_EXE = False
CMAKELISTS_FILE_NAME = 'CMakeLists.txt'
MENUCONFIG_RESULT_FILE = '.config'
ALLBINS_DIR = os.path.join(BASE_DIR, "Allbins")
COPY_IMG_PROJECTS = ('ap_boot', 'cp_boot', 'xinyiNBIot_AP', 'xinyiNBIot_CP')
CP_PROJECT_NAME = 'xinyiNBIot_CP'

DEFAULT_BUILD_SYSTEM = 'make'
SUPPORTED_BUILD_SYSTEM_AND_FILE = {'ninja': 'build.ninja', 'make': 'Makefile'}

IS_WINDOWS = True if platform.system().lower() == 'windows' else False

XY_LOGO = 'Xinyi NBIoT'


class XYException(Exception):
    pass


def get_all_projects():
    names = os.listdir(BASE_DIR)
    names.sort()
    project_dirs = []
    for name in names:
        if name and name[0] == '.':
            continue

        child_path = os.path.abspath(os.path.join(BASE_DIR, name))
        if os.path.isdir(child_path):
            if os.path.exists(os.path.join(child_path, CMAKELISTS_FILE_NAME)):
                project_dirs.append(child_path)
    return project_dirs


def do_project_build(project_dir, build_op, build_type, cmake_define_list):
    if RUN_AS_EXE:
        project_build_exe_file = os.path.join(project_dir, "xybuild.exe")
        if os.path.exists(project_build_exe_file):
            build_cmd = [f'{project_build_exe_file}',
                         f'{build_op}', '-b', f'{build_type}', '-p']
            if cmake_define_list:
                build_cmd.append('-D')
                build_cmd.extend(cmake_define_list)
        else:
            return
    else:
        project_build_py_file = os.path.join(project_dir, "xybuild.py")
        if os.path.exists(project_build_py_file):
            build_cmd = [f'{sys.executable}', f'{os.path.join(project_dir, "xybuild.py")}', f'{build_op}',
                         '-b', f'{build_type}', '-p']
            if cmake_define_list:
                build_cmd.append('-D')
                build_cmd.extend(cmake_define_list)
        else:
            return
    with subprocess.Popen(build_cmd, text=True, stdout=sys.stdout, stderr=sys.stderr) as p:
        # print(p.args)
        # stdout, stderr = p.communicate()
        p.wait()
        if p.returncode == 0:
            if build_op == 'all':
                project_name = os.path.basename(project_dir)
                project_out_dir = os.path.join(
                    ALLBINS_DIR, project_name)
                if os.path.exists(project_out_dir):
                    shutil.rmtree(project_out_dir)
                shutil.copytree(os.path.join(
                    project_dir, '.build', 'bin'), os.path.join(ALLBINS_DIR, project_name, 'bin'))
                shutil.copytree(os.path.join(
                    project_dir, '.build', 'elf'), os.path.join(ALLBINS_DIR, project_name, 'elf'))
                if project_name == CP_PROJECT_NAME:
                    shutil.copy2(os.path.join(
                        project_dir, '.build', 'loginfo.info'), os.path.join(ALLBINS_DIR, project_name))
                    shutil.copy2(os.path.join(
                        project_dir, '.build', 'loginfo.info'), ALLBINS_DIR)
                if project_name in COPY_IMG_PROJECTS:
                    file_names = os.listdir(
                        os.path.join(ALLBINS_DIR, project_name, 'bin'))
                    for file_name in file_names:
                        if os.path.splitext(file_name)[-1] == '.img':
                            shutil.copy2(os.path.join(
                                ALLBINS_DIR, project_name, 'bin', file_name), ALLBINS_DIR)


def clean_allbins(project_name):
    if project_name == CP_PROJECT_NAME:
        allbins_loginfo_file = os.path.join(ALLBINS_DIR, 'loginfo.info')
        if os.path.exists(allbins_loginfo_file):
            os.remove(allbins_loginfo_file)
    project_out_dir = os.path.join(ALLBINS_DIR, project_name)
    if os.path.exists(project_out_dir):
        project_out_bin_dir = os.path.join(project_out_dir, 'bin')
        if os.path.exists(project_out_bin_dir):
            file_names = os.listdir(project_out_bin_dir)
            for file_name in file_names:
                if os.path.splitext(file_name)[-1] == '.img':
                    outside_img_file_path = os.path.join(
                        ALLBINS_DIR, file_name)
                    if os.path.exists(outside_img_file_path):
                        os.remove(outside_img_file_path)
        shutil.rmtree(project_out_dir)


def build_operation(build_op, build_type, build_project_name, cmake_define_list=None):
    if not os.path.exists(ALLBINS_DIR):
        os.mkdir(ALLBINS_DIR)

    project_dirs = get_all_projects()
    if build_project_name == 'all':
        async_args = []
        for project_dir in project_dirs:
            async_args.append(
                (project_dir, build_op, build_type, cmake_define_list))
            if build_op in ('clear', 'clean'):
                clean_allbins(os.path.basename(project_dir))

        with mp.Pool(len(project_dirs)) as pool:
            result = pool.starmap(do_project_build, async_args)
            pool.close()
            pool.join()
    else:
        for project_dir in project_dirs:
            if os.path.basename(project_dir) == build_project_name:
                if build_op in ('clear', 'clean'):
                    clean_allbins(os.path.basename(project_dir))
                do_project_build(project_dir, build_op,
                                 build_type, cmake_define_list)
                break
        else:
            print(
                f'cannot find project {build_project_name}, stop building...')


def main():

    parser = argparse.ArgumentParser()

    parser.add_argument('operation', help='''Use all to compile SDK. Use clean/clear to clean SDK build files.''',
                        type=str, default='all', choices=['all', 'clean', 'clear'])
    parser.add_argument(
        '-b', help='Choose make or ninja.', type=str, default='make', choices=['make', 'ninja'])

    parser.add_argument(
        '-p', help='Choose project to build, or choose all to build all projects', type=str, default='all')

    parser.add_argument(
        '-D', help='pass define(s) to cmake', type=str, nargs='*', default=None)

    args = parser.parse_args()
    build_operation(args.operation, args.b, args.p, args.D)


if __name__ == '__main__':
    mp.freeze_support()
    main()
