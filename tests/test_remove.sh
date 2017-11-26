#!/bin/bash

test-input() {
    echo debug
    echo mount
    echo create
    echo create
    echo create
    echo remove 0
    echo debug
    echo create
    echo remove 0
    echo remove 0
    echo remove 1
    echo remove 3
    echo debug
}

test-output() {
    cat <<EOF
SuperBlock:
    magic number is valid
    5 blocks
    1 inode blocks
    128 inodes
Inode 1:
    size: 965 bytes
    direct blocks: 2
disk mounted.
created inode 0.
created inode 2.
created inode 3.
removed inode 0.
SuperBlock:
    magic number is valid
    5 blocks
    1 inode blocks
    128 inodes
Inode 1:
    size: 965 bytes
    direct blocks: 2
Inode 2:
    size: 0 bytes
    direct blocks:
Inode 3:
    size: 0 bytes
    direct blocks:
created inode 0.
removed inode 0.
remove failed!
removed inode 1.
removed inode 3.
SuperBlock:
    magic number is valid
    5 blocks
    1 inode blocks
    128 inodes
Inode 2:
    size: 0 bytes
    direct blocks:
25 disk block reads
8 disk block writes
EOF
}

cp data/image.5 data/image.5.remove
trap "rm -f data/image.5.remove test.log" INT QUIT TERM EXIT

echo -n "Testing remove in data/image.5.remove ... "
if diff -u <(test-input | ./bin/sfssh data/image.5.remove 5 2> /dev/null) <(test-output) > test.log; then
    echo "Success"
else
    echo "False"
    cat test.log
fi
