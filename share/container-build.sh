#!/usr/bin/env bash

#
# SPDX-FileCopyrightText:  2023, Iker Pedrosa <ipedrosa@redhat.com>
# SPDX-FileCopyrightText:  2024, Iker Pedrosa <ipedrosa@redhat.com>
# SPDX-FileCopyrightText:  2026, Hadi Chokr <hadichokr@icloud.com>
#
# SPDX-License-Identifier:  BSD-3-Clause
#

set -e
cd "$(dirname "$0")/ansible"

PLAYBOOK="playbook-unprivileged.yml"

for arg in "$@"; do
    case "$arg" in
        --privileged)
            PLAYBOOK="playbook-privileged.yml"
            ;;
    esac
done

for distro in alpine debian fedora opensuse; do
    ansible-playbook "$PLAYBOOK" \
        -i inventory.ini \
        -e "distribution=${distro}"
done
