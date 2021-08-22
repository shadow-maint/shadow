#!/bin/sh

. common/config.sh
set +e
set -x

create_container()
{
    lxcdefault=$(mktemp)
    gittop=$(git rev-parse --show-toplevel)
    # if I were feeling more ambitious i'd check the range...
    subuid=grep "^$(whoami):" /etc/subuid | cut -d : -f 2`
    subgid=grep "^$(whoami):" /etc/subuid | cut -d : -f 2`
    cat > $lxcdefault << EOF
lxc.include = /etc/lxc/default.conf
lxc.idmap = u 0 $(id -u) 1
lxc.idmap = g 0 $(id -g) 1
lxc.idmap = u 1 ${subuid} 65535
lxc.idmap = g 1 ${subgid} 65535
lxc.mount.entry = ${gittop} ${CONTAINER_GIT_TOP_REL} none bind,create=dir 0 0
EOF
    echo "Creating container with the following defaults:"
    cat $lxcdefault
    lxc-create -t download -f $lxcdefault -n shadow-test -- -d ubuntu -r bionic -a amd64
    rm -f $lxcdefault
    if [ $? -ne ] ; then
        echo "Failed creating test container"
        exit 1
    fi
    lxc-attach -n shadow-test -- apt-get update
    lxc-attach -n shadow-test -- apt-get -y install git ubuntu-dev-tools
}

s=$(lxc-info -qsHn shadow-test)
if [ $? -ne 0 ]; then
    echo "Creating"
    create_container
    s=$(lxc-info -qsHn shadow-test)
    if [ $? -ne 0 ]; then
        echo "Error"
        exit 1
    fi
fi
if [ $s = "STOPPED" ]; then
    lxc-start -n shadow-test
    lxc-wait -n shadow-test -s RUNNING
fi

echo "shadow-test is running"
