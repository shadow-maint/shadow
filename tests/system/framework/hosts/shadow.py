"""shadow multihost host."""

from __future__ import annotations

from pathlib import PurePosixPath
from typing import Any

from pytest_mh.conn import ProcessLogLevel

from .base import BaseHost, BaseLinuxHost

__all__ = [
    "ShadowHost",
]


class ShadowHost(BaseHost, BaseLinuxHost):
    """
    shadow host object.

    This is the host where the tests are run.

    .. note::

        Full backup and restore of shadow state is supported.
    """

    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)

        self._features: dict[str, bool] | None = None

    def pytest_setup(self) -> None:
        super().pytest_setup()

    def start(self) -> None:
        """
        Not supported.

        :raises NotImplementedError: _description_
        """
        raise NotImplementedError("Starting shadow service is not implemented.")

    def stop(self) -> None:
        """
        Not supported.

        :raises NotImplementedError: _description_
        """
        raise NotImplementedError("Stopping shadow service is not implemented.")

    def backup(self) -> Any:
        """
        Backup all shadow data.

        :return: Backup data.
        :rtype: Any
        """
        self.logger.info("Creating backup of shadow host")

        result = self.conn.run(
            """
            set -ex

            function backup {
                if [ -d "$1" ] || [ -f "$1" ]; then
                    cp --force --archive "$1" "$2"
                fi
            }

            path=`mktemp -d`
            backup /etc/login.defs "$path/login.defs"
            backup /etc/default/useradd "$path/useradd"
            backup /etc/passwd "$path/passwd"
            backup /etc/shadow "$path/shadow"
            backup /etc/group "$path/group"
            backup /etc/gshadow "$path/gshadow"
            backup /etc/subuid "$path/subuid"
            backup /etc/subgid "$path/subgid"
            backup /home "$path/home"
            backup /var/log/secure "$path/secure"

            echo $path
            """,
            log_level=ProcessLogLevel.Error,
        )

        return PurePosixPath(result.stdout_lines[-1].strip())

    def restore(self, backup_data: Any | None) -> None:
        """
        Restore all shadow data.

        :return: Backup data.
        :rtype: Any
        """
        if backup_data is None:
            return

        if not isinstance(backup_data, PurePosixPath):
            raise TypeError(f"Expected PurePosixPath, got {type(backup_data)}")

        backup_path = str(backup_data)

        self.logger.info(f"Restoring shadow data from {backup_path}")
        self.conn.run(
            f"""
            set -ex

            function restore {{
                rm --force --recursive "$2"
                if [ -d "$1" ] || [ -f "$1" ]; then
                    cp --force --archive "$1" "$2"
                fi
            }}

            rm --force --recursive /var/log/secure
            restore "{backup_path}/login.defs" /etc/login.defs
            restore "{backup_path}/useradd" /etc/default/useradd
            restore "{backup_path}/passwd" /etc/passwd
            restore "{backup_path}/shadow" /etc/shadow
            restore "{backup_path}/group" /etc/group
            restore "{backup_path}/gshadow" /etc/gshadow
            restore "{backup_path}/subuid" /etc/subuid
            restore "{backup_path}/subgid" /etc/subgid
            restore "{backup_path}/home" /home
            restore "{backup_path}/secure" /var/log/secure
            """,
            log_level=ProcessLogLevel.Error,
        )
