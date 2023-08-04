#!/usr/bin/env python3

from hashlib import sha256
import argparse
import json
import os
import urllib.request


def fetch_data(url):
    with urllib.request.urlopen(url) as response:
        return response.read()


def save_json(obj, file_path):
    with open(
        file_path, 'w', encoding='utf-8', newline='\n') as f:
            json.dump(obj, f, indent=2, sort_keys=True)


def fetch_file_infos(repo_id):
    url = (
        'https://api.github.com/repos/tesseract-ocr/{}/contents'
        '?ref={}').format(*repo_id.split(':', 1))

    repo_contents = json.loads(fetch_data(url).decode())

    result = []

    for item in repo_contents:
        root, ext = os.path.splitext(item['path'])
        if root == 'equ' or root == 'osd' or ext != '.traineddata':
            continue

        result.append({
            'code': root,
            'sha256': None,
            'size': item['size'],
            'url': item['download_url'],
        })

    return result


def generate(out_file_path, repo_id):
    cache_file_path = os.path.join(
        os.path.dirname(out_file_path),
        os.path.splitext(os.path.basename(__file__))[0]
            + '_cache.json')

    try:
        with open(cache_file_path, encoding='utf-8') as f:
            cache = json.load(f)
    except FileNotFoundError:
        cache = {}

    if repo_id not in cache:
        cache[repo_id] = fetch_file_infos(repo_id)
        save_json(cache, cache_file_path)

    file_infos = cache[repo_id]

    file_infos_without_sha256 = [
        i for i in file_infos if not i['sha256']]

    num_missing = len(file_infos_without_sha256)
    if num_missing > 0:
        num_bytes_to_download = sum(
            i['size'] for i in file_infos_without_sha256)

        print(
            'Computing SHA-256 for {} file{}; '
            '{} MB to download.'.format(
                num_missing,
                '' if num_missing == 1 else 's',
                round(num_bytes_to_download / 1000 / 1000, 2)))

    for i, file_info in enumerate(file_infos_without_sha256, 1):
        print(
            '{:3}/{} {}'.format(i, num_missing, file_info['url']))

        file_info['sha256'] = sha256(
            fetch_data(file_info['url'])).hexdigest()

        save_json(cache, cache_file_path)

    save_json(file_infos, out_file_path)


def main():
    parser = argparse.ArgumentParser(
        description=(
            'Generate the Tesseract languages info file for a remote '
            'server based on data from GitHub.'),
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument(
        'repo',
        help=(
            'Repository name in NAME:TAG format, e.g., '
            '"tessdata_fast:4.1.0".'))
    parser.add_argument('out_file', help='Output file')

    args = parser.parse_args()

    generate(args.out_file, args.repo)


if __name__ == '__main__':
    main()
