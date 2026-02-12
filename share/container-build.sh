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
PRIVILEGED=false
for arg in "$@"; do
    case "$arg" in
        --privileged)
            PRIVILEGED=true
            ;;
    esac
done
for distro in alpine debian fedora opensuse; do
    ansible-playbook playbook.yml \
        -i inventory.ini \
        -e "distribution=${distro}" \
        -e "privileged_mode=${PRIVILEGED}"
done
