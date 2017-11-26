#!/bin/bash

#!/bin/bash

mount-input() {
    cat <<EOF
mount
EOF
}

mount-output() {
    cat <<EOF
disk mounted.
2 disk block reads
0 disk block writes
EOF
}

mount-mount-input() {
    cat <<EOF
mount
mount
EOF
}

mount-mount-output() {
    cat <<EOF
disk mounted.
mount failed!
2 disk block reads
0 disk block writes
EOF
}

mount-format-input() {
    cat <<EOF
mount
format
EOF
}

mount-format-output() {
    cat <<EOF
disk mounted.
format failed!
2 disk block reads
0 disk block writes
EOF
}

test-mount () {
    TEST=$1

    echo -n "Testing $TEST on data/image.5 ... "
    if diff -u <($TEST-input| ./bin/sfssh data/image.5 5 2> /dev/null) <($TEST-output) > test.log; then
    	echo "Success"
    else
    	echo "Failure"
    fi
    cat test.log
    rm -f test.log
}

test-mount mount
test-mount mount-mount
test-mount mount-format
