# -*- encoding: utf-8 -*-

import os
import sys


def main():
    for root, dirs, files in os.walk(os.path.abspath('.')):
        if not os.path.basename(root).startswith('.'):
            for filename in files:
                if filename.endswith('.mk') or filename == 'make.ini':
                    print(f'remove {os.path.join(root, filename)}')
                    os.remove(os.path.join(root, filename))


if __name__ == '__main__':
    main()
