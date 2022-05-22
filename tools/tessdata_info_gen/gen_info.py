#!/usr/bin/env python3
"""Retrieve information about Tesseract data names.

This file is usable both as a program and a module. In either case,
you need iso-639-3.tab and and iso-639-3_Name_Index.tab from:

    https://iso639-3.sil.org/code_tables/download_tables
"""

from collections import namedtuple
import argparse
import csv
import json
import os


_ISO_639_3_TABLE = 'iso-639-3.tab'
_ISO_639_3_NAME_INDEX_TABLE = 'iso-639-3_Name_Index.tab'
_ISO_TABLES_CSV_DIALECT = 'excel-tab'


class _Iso6393TableId:
    ID_639_3 = 'Id'
    ID_639_2B = 'Part2B'
    ID_639_1 = 'Part1'
    NAME = 'Ref_Name'


class _Iso6393NameIndexTableId:
    ID_639_3 = 'Id'
    NAME = 'Print_Name'
    INVERTED_NAME = 'Inverted_Name'


class _IsoMappings:
    def __init__(self, iso_639_3_tables_dir):
        self.iso_639_3_ref_name = {}
        self.iso_639_3_names = {}
        self.iso_639_2b_to_3 = {}
        self.iso_639_3_to_1 = {}

        self._load_iso_639_3_db(
            os.path.join(iso_639_3_tables_dir, _ISO_639_3_TABLE))
        self._load_iso_639_3_name_index_db(
            os.path.join(
                iso_639_3_tables_dir, _ISO_639_3_NAME_INDEX_TABLE))
        self._cleanup_names()

    def _load_iso_639_3_db_row(self, row):
        iso_639_3_id = row[_Iso6393TableId.ID_639_3]

        self.iso_639_3_ref_name[iso_639_3_id] = (
            row[_Iso6393TableId.NAME])
        self.iso_639_3_names[iso_639_3_id] = []

        iso_639_2b_id = row[_Iso6393TableId.ID_639_2B]
        if iso_639_2b_id:
            self.iso_639_2b_to_3[iso_639_2b_id] = iso_639_3_id

        self.iso_639_3_to_1[iso_639_3_id] = (
            row[_Iso6393TableId.ID_639_1])

    def _load_iso_639_3_db(self, file_path):
        with open(file_path, encoding='utf-8', newline='') as f:
            for row in csv.DictReader(
                    f, dialect=_ISO_TABLES_CSV_DIALECT):
                self._load_iso_639_3_db_row(row)

    def _load_iso_639_3_name_index_db_row(self, row):
        name = row[_Iso6393NameIndexTableId.NAME]

        inverted_name = row[_Iso6393NameIndexTableId.INVERTED_NAME]
        inverted_name = inverted_name.replace(' (macrolanguage)', '')

        iso_639_3_id = row[_Iso6393NameIndexTableId.ID_639_3]
        names = self.iso_639_3_names[iso_639_3_id]

        # Consider the reference name to be the primary one. Still,
        # sometimes the reference is the root name, e.g. "Khmer"
        # rather than "Central Khmer".
        if name == self.iso_639_3_ref_name[iso_639_3_id]:
            names.insert(0, inverted_name)
        else:
            names.append(inverted_name)

    def _load_iso_639_3_name_index_db(self, file_path):
        with open(file_path, encoding='utf-8', newline='') as f:
            for row in csv.DictReader(
                    f, dialect=_ISO_TABLES_CSV_DIALECT):
                self._load_iso_639_3_name_index_db_row(row)

    def _cleanup_names(self):
        def is_part_of_any(name, l):
            for n in l:
                if n != name and name in n:
                    return True

            return False

        # Remove generic name if we already have a more specific one.
        # E.g. we don't need "Gaelic" if there is "Gaelic, Scottish".
        for code, names in self.iso_639_3_names.items():
            self.iso_639_3_names[code] = [
                n for n in names if not is_part_of_any(n, names)]


Info = namedtuple(
    'Info', 'names name_infos iso_639_3 iso_639_1 data_name')


_SPECIAL_CASES = {
    # frk is actually Germain Fraktur rather than Frankish. See:
    # https://github.com/tesseract-ocr/tessdata_best/issues/68
    # https://github.com/tesseract-ocr/tessdata/issues/49
    # https://github.com/tesseract-ocr/langdata/issues/61
    'frk': Info(['German'], ['Fraktur'], 'deu', 'de', 'frk')
}


_INFO_STR = {
    'cyrl': 'Cyrillic',
    'frak': 'Fraktur',
    'latn': 'Latin',
    'old': 'old',
    'sim': 'simplified',
    'tra': 'traditional',
    'vert': 'vertical',
}


class InfoDb:
    def __init__(self, iso_639_3_tables_dir):
        """Create DB.

        iso_639_3_tables_dir is a path to a directory containing
        iso-639-3.tab and and iso-639-3_Name_Index.tab from
        https://iso639-3.sil.org/code_tables/download_tables
        """
        self._iso_mappings = _IsoMappings(iso_639_3_tables_dir)

    def get_info(self, data_name, silent=True):
        """Get info about the given data name as Info object."""
        special = _SPECIAL_CASES.get(data_name)
        if special:
            if not silent:
                print(
                    'Special case: {} remapped to {}'.format(
                        data_name, special.iso_639_3, special.names))
            return special

        name_id, *info_ids = data_name.split('_')

        iso_639_3_id = name_id
        names = self._iso_mappings.iso_639_3_names.get(iso_639_3_id)
        if names is None:
            # Try to remap the name from ISO 639-2B code to ISO 639-3.
            # For example, the name of the Chinese data file is "chi"
            # rather than "zho".
            iso_639_3_id = self._iso_mappings.iso_639_2b_to_3.get(
                name_id, '')
            if iso_639_3_id:
                if not silent:
                    print(
                        'ISO 639-2/B {} remapped to ISO 639-3 {}'
                        ' ({})'.format(
                            name_id, iso_639_3_id, data_name))
                names = self._iso_mappings.iso_639_3_names[
                    iso_639_3_id]
            else:
                if not silent:
                    print('No ISO 639-3 name for', name_id)
                names = [name_id]

        name_infos = []
        for info_id in info_ids:
            info_str = _INFO_STR.get(info_id)
            if info_str:
                name_infos.append(info_str)
            elif not silent:
                print(
                    'Unknown info id "{}" in {}'.format(
                        info_id, data_name))

        return Info(
            names,
            name_infos,
            iso_639_3_id,
            self._iso_mappings.iso_639_3_to_1.get(iso_639_3_id, ''),
            data_name)


def _main():
    parser = argparse.ArgumentParser(
        description=(
            'Generate a JSON file containing information about the '
            'given Tesseract data names. The program requires  ISO '
            '639-3 code tables from '
            'https://iso639-3.sil.org/code_tables/download_tables.'),
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument(
        'names_file',
        type=argparse.FileType('r', encoding='utf-8'),
        help='File to read names from ("-" for stdin).')
    parser.add_argument(
        'out_file',
        type=argparse.FileType('w', encoding='utf-8'),
        help='Output JSON file ("-" for stdout).')

    parser.add_argument(
        '--ignored-names',
        metavar='NAME',
        nargs='+',
        default=['equ', 'osd'],
        help='Names to ignore.')

    parser.add_argument(
        '--tables-dir',
        default='.',
        help=(
            'Directory containing iso-639-3.tab and '
            'iso-639-3_Name_Index.tab'))

    parser.add_argument(
        '--silent',
        action='store_true',
        help='Don\'t print anything.')

    args = parser.parse_args()

    db = InfoDb(args.tables_dir)

    infos = []
    for line in args.names_file:
        name = line.strip()
        if not name or name in args.ignored_names:
            continue

        infos.append(db.get_info(name, args.silent)._asdict())

    json.dump(infos, args.out_file, indent=2, sort_keys=True)


if __name__ == '__main__':
    _main()
