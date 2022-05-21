#!/usr/bin/env python3

import argparse
import json
import os
import urllib.request


def make_url(repo_name, repo_tag):
    return (
        'https://api.github.com/repos/tesseract-ocr'
        '/{}/contents?ref={}').format(repo_name, repo_tag)


def fetch_names(repo_name, repo_tag):
    result = set()

    url = make_url(repo_name, repo_tag)
    with urllib.request.urlopen(url) as response:
        infos = json.loads(response.read().decode())
        for info in infos:
            root, ext = os.path.splitext(info['name'])
            if ext == '.traineddata':
                result.add(root)

    return result


def main():
    parser = argparse.ArgumentParser(
        description=(
            'Get list of Tesseract data files from GitHub repos.'))

    parser.add_argument(
        'out_file',
        type=argparse.FileType('w', encoding='utf-8'),
        help='Output file ("-" for stdout).')

    parser.add_argument(
        'repos',
        nargs='+',
        help=(
            'Repository name in NAME:TAG format, e.g. '
            '"tessdata_fast:4.1.0".'))

    args = parser.parse_args()

    names = set()

    for repo in args.repos:
        repo_name, repo_tag = repo.split(':', 1)
        names.update(fetch_names(repo_name, repo_tag))

    args.out_file.writelines('{}\n'.format(n) for n in sorted(names))


if __name__ == '__main__':
    main()
