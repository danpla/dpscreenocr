#!/usr/bin/env python3

import argparse


TIMESTAMP = '2000-12-30 12:15:35'
TEXT = (
    'Lorem ipsum dolor sit amet, consectetur adipiscing elit. '
    'Quisque sed justo urna. Sed porttitor arcu massa. Nulla gravida '
    'pretium semper. Pellentesque pretium risus sit amet augue '
    'pretium, nec euismod velit tempus. Ut tristique justo vitae '
    'varius vehicula. Praesent ut felis sem. Nam consectetur nec '
    'purus et venenatis.\n'
    '\n'
    'Aenean luctus, velit at tincidunt viverra, sem turpis venenatis '
    'massa, eu bibendum est erat non lorem. Integer fringilla ipsum '
    'non nibh mollis efficitur. Aliquam porttitor erat vulputate '
    'eros porttitor, tempus aliquam dolor gravida. Sed non ex urna. '
    'Aliquam in mollis nisl, sit amet maximus sem. Etiam rutrum nisl '
    'id lorem tempor scelerisque. Nam euismod finibus risus sit amet '
    'molestie. Mauris facilisis id dolor sit amet ullamcorper. In '
    'porttitor egestas nisi ut vestibulum.\n'
    '\n'
    'Suspendisse convallis venenatis mattis. Integer eget ornare '
    'tortor. Aliquam ultrices eros eget urna congue dapibus. Nullam '
    'mauris quam, dignissim sed ligula vitae, interdum luctus est. '
    'Integer eget arcu porta, ullamcorper velit quis, commodo lacus. '
    'Sed suscipit vehicula urna et laoreet. Donec eu orci sed enim '
    'suscipit aliquet vitae vitae sapien. Sed imperdiet consequat '
    'nisi sed consectetur. Interdum et malesuada fames ac ante ipsum '
    'primis in faucibus. Fusce eu faucibus velit, eu auctor leo.\n'
    '\n'
    'Duis quis augue dui. Mauris a imperdiet odio. Quisque in '
    'vehicula leo, id vulputate magna. Quisque mattis sagittis dui, '
    'non posuere ligula viverra eget. Morbi blandit cursus '
    'pellentesque. Nullam semper finibus ornare. In porttitor, ipsum '
    'nec hendrerit sollicitudin, lorem diam efficitur mauris, nec '
    'convallis mauris neque non risus. Donec euismod hendrerit est, '
    'eu varius erat facilisis eu. In semper odio mauris, sed '
    'consectetur ipsum mattis nec. Donec accumsan accumsan est, ac '
    'aliquam quam. Pellentesque habitant morbi tristique senectus et '
    'netus et malesuada fames ac turpis egestas. Proin a enim sed '
    'orci sodales convallis sed at risus. Nulla aliquam dui nec orci '
    'elementum, id mollis leo molestie. Nulla euismod ut lacus sed '
    'tempor. Vivamus neque arcu, lobortis luctus mauris non, pretium '
    'eleifend diam.\n'
    '\n'
    'Nam volutpat aliquet vehicula. Donec tristique pretium orci '
    'vitae facilisis. Pellentesque et est ac nunc aliquet efficitur. '
    'Maecenas cursus venenatis egestas. Quisque tempus, turpis '
    'elementum maximus tincidunt, justo magna feugiat massa, nec '
    'laoreet neque sem in erat. In hac habitasse platea dictumst. '
    'Phasellus pharetra porta ante, eget rhoncus orci facilisis vel.'
)


def main():
    parser = argparse.ArgumentParser(
        description='Generate a history file for testing.')

    parser.add_argument(
        '-o', '--out-file', default='history.txt',
        help='Output file.')

    parser.add_argument(
        '-n', '--num-entries', type=int, default=1000,
        help='Number of history entries.')

    args = parser.parse_args()

    with open(args.out_file, 'w', encoding='utf-8', newline='') as f:
        for n in range(1, args.num_entries + 1):
            if n > 1:
                f.write('\f\n')

            f.write(TIMESTAMP)
            f.write('\n\n')

            f.write('Entry {}\n\n'.format(n))
            f.write(TEXT)


if __name__ == '__main__':
    main()
