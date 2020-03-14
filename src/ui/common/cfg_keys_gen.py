#!/usr/bin/env python3
#
# Generate cfg_keys.h/cpp from cfg_keys.txt.

import enum


_KEYS_FILE = 'cfg_keys'


def _load_keys(filename):
    keys = []

    with open(filename, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if line:
                keys.append(line)

    return keys


@enum.unique
class FileType(enum.Enum):
    HEADER = 0
    IMPLEMENTATION = 1


def _get_file_ext(file_type):
    return '.h' if file_type == FileType.HEADER else '.cpp'


def _get_file_header(filename_root, file_type):
    if file_type == FileType.HEADER:
        return '\n#pragma once\n\n\n'
    else:
        return '\n#include "{}{}"\n\n\n'.format(
            filename_root, _get_file_ext(FileType.HEADER))


_VAR_NAME_PREFIX = 'cfgKey'


def _get_var_name(key_name):
    name = _VAR_NAME_PREFIX

    upcase = True
    for c in key_name:
        if c == '_':
            upcase = True
        elif upcase:
            assert c != '_'
            name += c.upper()
            upcase = False
        else:
            name += c

    return name


def _format_var(key_name, file_type):
    var = ''

    if file_type == FileType.HEADER:
        var += 'extern '

    var += 'const char* const ' + _get_var_name(key_name)

    if file_type == FileType.IMPLEMENTATION:
        var += ' =\n    "{}"'.format(key_name)

    var += ';'

    return var


def _write_source(filename_root, key_names, file_type):
    with open(
            filename_root + _get_file_ext(file_type), 'w',
            encoding='utf-8', newline='\n') as f:
        f.write(
            '\n/* This file was automatically generated. '
            'Do not edit. */\n')

        f.write(_get_file_header(filename_root, file_type))

        for key_name in key_names:
            f.write(_format_var(key_name, file_type))
            f.write('\n')


def main():
    key_names = _load_keys(_KEYS_FILE + '.txt')

    _write_source(_KEYS_FILE, key_names, FileType.HEADER)
    _write_source(_KEYS_FILE, key_names, FileType.IMPLEMENTATION)


if __name__ == '__main__':
    main()
