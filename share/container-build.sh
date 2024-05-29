#! /bin/bash

#
# SPDX-FileCopyrightText:  2023, Iker Pedrosa <ipedrosa@redhat.com>
# SPDX-FileCopyrightText:  2024, Iker Pedrosa <ipedrosa@redhat.com>
#
# SPDX-License-Identifier:  BSD-3-Clause
#

set -eE
cd ansible/
ansible-playbook playbook.yml -i inventory.ini -e 'distribution=alpine'
ansible-playbook playbook.yml -i inventory.ini -e 'distribution=debian'
ansible-playbook playbook.yml -i inventory.ini -e 'distribution=fedora'
