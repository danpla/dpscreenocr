This directory contains Python scripts to generate language tables for
Tesseract data files.

get_data_list.py fetches names of Tesseract data files from one or
more GitHub repositories.

gen_info.py takes the list of names (e.g. from get_data_list.py) and
creates a JSON file with various information (like language name, ISO
639-3 code, etc.) based on ISO 639-3 code tables from
https://iso639-3.sil.org/code_tables/download_tables. Other scripts
can use this JSON for file generation. gen_c.py is an example of such
a script; it creates a language code table for C/C++ source.
