#!/bin/sh

PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"

# Check user exists, and if so, send sigkill to processes that the user owns

RUNNING=`ps -eo user | grep -Fx "$SUBJECT" | wc -l`

# if the user does not exist, RUNNING will be 0

if [ "${RUNNING}x" = "0x" ]; then
  exit 0
fi

ls -1 /proc | while IFS= read -r PROC; do
  echo "$PROC" | grep -E '^[0-9]+$' >/dev/null
  if [ $? -ne 0 ]; then
    continue
  fi
  if [ -d "/proc/${PROC}" ]; then
    USR=`stat -c "%U" /proc/${PROC}`
    if [ "${USR}" = "${SUBJECT}" ]; then
      echo "Killing ${SUBJECT} owned ${PROC}"
      kill -9 "${PROC}"
    fi
  fi
done

