#!/bin/bash
# turn shadow passwords on or off on a Debian system

set -e

permfix () {
    [ -f $1 ] || return 0
    chown root:shadow $1
    chmod 2755 $1
}
export -f permfix

shadowon () {
bash<<- EOF
    set -e

    permfix /usr/X11R6/bin/xlock
    permfix /usr/X11R6/bin/xtrlock
    permfix /bin/vlock

    pwck -q
    grpck
    pwconv
    grpconv
    cd /etc
    chown root:root passwd group
    chmod 644 passwd group
    chown root:shadow shadow gshadow
    chmod 640 shadow gshadow
EOF
}

shadowoff () {
bash<<- EOF
    set -e
    pwck -q
    grpck
    pwunconv
    grpunconv
    cd /etc
    # sometimes the passwd perms get munged
    chown root:root passwd group
    chmod 644 passwd group
EOF
}

case "$1" in
    "on")
	if shadowon ; then
	    echo Shadow passwords are now on.
	else
	    echo Please correct the error and rerun \`$0 on\'
	    exit 1
	fi
	;;
    "off")
	if shadowoff ; then
	    echo Shadow passwords are now off.
	else
	    echo Please correct the error and rerun \`$0 off\'
	    exit 1
	fi
	;;
     *)
	echo Usage: $0 on \| off
	;;
esac
