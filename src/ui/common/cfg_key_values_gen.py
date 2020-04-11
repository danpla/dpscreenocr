#!/usr/bin/env python3
#
# Generate cfg_keys and cfg_default_values h/cpp from
# cfg_key_values.txt.

import enum
from collections import OrderedDict, namedtuple


Value = namedtuple('Value', 'type initializer')
Var = namedtuple('Var', 'type name value')


def load_key_values(filename):
    key_values = OrderedDict()

    with open(filename, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue

            parts = list(s.strip() for s in line.split('|', 2))
            num_parts = len(parts)
            if num_parts == 2:
                raise ValueError(
                    '"{}" has type without initializer'.format(line))

            value = None if num_parts == 1 else Value(*parts[1:])
            key_values[parts[0]] = value

    return key_values


def gen_var_name(key, prefix='', postfix=''):
    name = prefix

    upcase = prefix != ''
    for c in key:
        if c == '_':
            upcase = True
        elif upcase:
            name += c.upper()
            upcase = False
        else:
            name += c

    return name + postfix


def gen_cfg_key_vars(keys):
    result = []

    for key in keys:
        result.append(
            Var(
                'const char*',
                gen_var_name(key, 'cfgKey'),
                '"' + key + '"'))

    return result


def gen_cfg_default_value_vars(key_values):
    result = []

    for key, value in key_values.items():
        if value is None:
            continue

        result.append(
            Var(
                value.type,
                gen_var_name(key, 'cfgDefaultValue'),
                value.initializer))

    return result


@enum.unique
class FileType(enum.Enum):
    HEADER = 0
    IMPLEMENTATION = 1


def get_file_ext(file_type):
    return '.h' if file_type == FileType.HEADER else '.cpp'


def gen_file_header(
        filename_root, file_type,
        header_includes, implementation_includes):
    s = ''

    if file_type == FileType.HEADER:
        s += '\n#pragma once\n'

    includes = None
    if file_type == FileType.HEADER:
        includes = header_includes
    else:
        includes = []
        includes.append(filename_root + get_file_ext(FileType.HEADER))
        if implementation_includes is not None:
            includes.extend(implementation_includes)

    if includes:
        s += '\n'

        for include in includes:
            s += '#include "{}"\n'.format(include)

    return s + '\n\n'


def format_var(var, file_type):
    var_str = ''

    if file_type == FileType.HEADER:
        var_str += 'extern '

    var_str += var.type + ' const ' + var.name

    if file_type == FileType.IMPLEMENTATION:
        var_str += ' =\n    {}'.format(var.value)

    return var_str + ';'


def write_source(
        filename_root, variables, file_type,
        header_includes, implementation_includes):
    with open(
            filename_root + get_file_ext(file_type), 'w',
            encoding='utf-8', newline='\n') as f:
        f.write(
            '\n/* This file was automatically generated. '
            'Do not edit. */\n')

        f.write(
            gen_file_header(
                filename_root, file_type,
                header_includes=header_includes,
                implementation_includes=implementation_includes))

        for var in variables:
            f.write(format_var(var, file_type))
            f.write('\n')


def write_sources(
        filename_root, variables, *,
        header_includes=None, implementation_includes=None):
    for file_type in FileType:
        write_source(
            filename_root, variables, file_type,
            header_includes, implementation_includes)


def main():
    key_values = load_key_values('cfg_key_values.txt')

    write_sources('cfg_keys', gen_cfg_key_vars(key_values.keys()))
    write_sources(
        'cfg_default_values', gen_cfg_default_value_vars(key_values),
        header_includes=['dpso/dpso.h'])


if __name__ == '__main__':
    main()
