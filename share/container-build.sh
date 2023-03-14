#! /bin/bash

#
# SPDX-FileCopyrightText:  2023, Iker Pedrosa <ipedrosa@redhat.com>
#
# SPDX-License-Identifier:  BSD-3-Clause
#

for FILE in share/containers/*; do
	IFS='/'
	read -ra ADDR <<< "$FILE"
	IFS='.'
	read -ra ADDR <<< "${ADDR[2]}"
	IFS=''
	if ! docker build -f $FILE . --output build-out/${ADDR[0]}; then
		exit
	fi
done
