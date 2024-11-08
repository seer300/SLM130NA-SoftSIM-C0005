# -*- encoding: utf-8 -*-
import os
import sys
import shutil
import subprocess
import argparse

if getattr(sys, 'frozen', False):
    PROJECT_BASE_DIR = os.path.dirname(os.path.abspath(sys.executable))
    RUN_AS_EXE = True
else:
    PROJECT_BASE_DIR = os.path.dirname(os.path.abspath(__file__))
    RUN_AS_EXE = False
CMAKE_BUILD_DIR = os.path.join(PROJECT_BASE_DIR, '.build')
IGNORE_DIR = os.path.join(PROJECT_BASE_DIR, '.ignore')
CMAKE_TOOLCHAIN_FILE = os.path.join(
    PROJECT_BASE_DIR, r'TOOLS/cmake/arm-gcc-toolchain.cmake')

DEFAULT_BUILD_SYSTEM = 'make'
SUPPORTED_BUILD_SYSTEM_AND_FILE = {'ninja': 'build.ninja', 'make': 'Makefile'}

XY_LOGO = 'Xinyi NBIoT'


class XYException(Exception):
    pass


def execute_cmd(cmd, sucessful_msg=''):
    with subprocess.Popen(cmd, text=True, stdout=sys.stdout, stderr=sys.stderr) as p:
        print(' '.join(p.args))
        # stdout, stderr = p.communicate()
        p.wait()
        if p.returncode != 0:
            raise XYException(f'failed to execute command {cmd}')
        else:
            if sucessful_msg:
                print(sucessful_msg)


def special_build_operation(build_target):
    cmake_cmd = ['cmake', '--build', CMAKE_BUILD_DIR, '--target', build_target]
    execute_cmd(cmake_cmd)


def make_operation(build_op, is_concurrent, cmake_define_list):
    if not os.path.exists(os.path.join(CMAKE_BUILD_DIR, 'Makefile')):
        if RUN_AS_EXE:
            cmake_cmd = ['cmake', '-G', 'Unix Makefiles', '-S', PROJECT_BASE_DIR,
                         '-B', CMAKE_BUILD_DIR, f'-DCMAKE_TOOLCHAIN_FILE={CMAKE_TOOLCHAIN_FILE}']
        else:
            cmake_cmd = ['cmake', '-G', 'Unix Makefiles', '-S', PROJECT_BASE_DIR, '-B', CMAKE_BUILD_DIR,
                         f'-DPYTHON={sys.executable}', f'-DCMAKE_TOOLCHAIN_FILE={CMAKE_TOOLCHAIN_FILE}']
        if cmake_define_list:
            cmake_cmd.extend(f'-D{define}' for define in cmake_define_list)
        execute_cmd(cmake_cmd)
    cpu_count_times = 1 if is_concurrent else 2
    cmake_cmd = ['cmake', '--build', CMAKE_BUILD_DIR, '-j', str(os.cpu_count(
    ) * cpu_count_times), '--target', build_op, '--', '--no-print-directory']
    try:
        execute_cmd(cmake_cmd)
    except Exception as e:
        special_build_operation('restore_log_string')
        raise(e)
    special_build_operation('restore_log_string')


def ninja_operation(build_op, is_concurrent, cmake_define_list):
    if not os.path.exists(os.path.join(CMAKE_BUILD_DIR, 'build.ninja')):
        if RUN_AS_EXE:
            cmake_cmd = ['cmake', '-G', 'Ninja', '-S', PROJECT_BASE_DIR,
                         '-B', CMAKE_BUILD_DIR, f'-DCMAKE_TOOLCHAIN_FILE={CMAKE_TOOLCHAIN_FILE}']
        else:
            cmake_cmd = ['cmake', '-G', 'Ninja', '-S', PROJECT_BASE_DIR, '-B', CMAKE_BUILD_DIR,
                         f'-DPYTHON={sys.executable}', f'-DCMAKE_TOOLCHAIN_FILE={CMAKE_TOOLCHAIN_FILE}']
        if cmake_define_list:
            cmake_cmd.extend(f'-D{define}' for define in cmake_define_list)
        execute_cmd(cmake_cmd)
    cpu_count_times = 1 if is_concurrent else 2
    cmake_cmd = ['cmake', '--build', CMAKE_BUILD_DIR, '-j',
                 str(os.cpu_count() * cpu_count_times), '--target', build_op]
    try:
        execute_cmd(cmake_cmd)
    except Exception as e:
        special_build_operation('restore_log_string')
        raise(e)
    special_build_operation('restore_log_string')


def build_operation(build_op, build_type, is_concurrent, cmake_define_list=None):
    if build_op in ('clear', 'clean'):
        if os.path.exists(CMAKE_BUILD_DIR):
            # try:
            #     if os.path.exists(os.path.join(CMAKE_BUILD_DIR, SUPPORTED_BUILD_SYSTEM_AND_FILE[build_type])):
            #         special_build_operation('reset_log_id')
            # except Exception as e:
            #     pass
            shutil.rmtree(CMAKE_BUILD_DIR)
        if os.path.exists(IGNORE_DIR):
            shutil.rmtree(IGNORE_DIR)
        print(os.path.basename(PROJECT_BASE_DIR), 'cleared')
        return
    if not os.path.exists(IGNORE_DIR):
        os.mkdir(IGNORE_DIR)
    if not os.path.exists(CMAKE_BUILD_DIR):
        os.mkdir(CMAKE_BUILD_DIR)
    if build_type == 'ninja':
        ninja_operation(build_op, is_concurrent, cmake_define_list)
    elif build_type == 'make':
        make_operation(build_op, is_concurrent, cmake_define_list)


def main():

    parser = argparse.ArgumentParser()

    parser.add_argument('operation', help='''Use all to compile SDK. Use clean/clear to clean SDK build files.''',
                        type=str, default='all', choices=['all', 'clean', 'clear'])
    parser.add_argument(
        '-b', help='Choose make or ninja.', type=str, default='make', choices=['make', 'ninja'])
    parser.add_argument(
        '-p', help='build concurrently with other projects.', action='store_true')
    parser.add_argument(
        '-D', help='pass define(s) to cmake', type=str, nargs='*')

    args = parser.parse_args()

    build_operation(args.operation, args.b, args.p, args.D)


if __name__ == '__main__':
    main()
