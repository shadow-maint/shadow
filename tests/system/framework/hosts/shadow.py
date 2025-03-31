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
        """Features dictionary."""

        self._backup_path: PurePosixPath | None = None
        """Path to backup files."""

        self._verify_files: [dict[str, str]] = [
            {"origin": "/etc/passwd", "backup": "passwd"},
            {"origin": "/etc/shadow", "backup": "shadow"},
            {"origin": "/etc/group", "backup": "group"},
            {"origin": "/etc/gshadow", "backup": "gshadow"},
        ]
        """Files to verify for mismatch."""

        self._features: dict[str, bool] | None = None
        """Features supported by the host."""

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

    @property
    def features(self) -> dict[str, bool]:
        """
        Features supported by the host.
        """
        if self._features is not None:
            return self._features

        self.logger.info(f"Detecting shadow features on {self.hostname}")
        result = self.conn.run(
            """
            set -ex

            getent gshadow > /dev/null 2>&1 && echo "gshadow" || :
            """,
            log_level=ProcessLogLevel.Error,
        )

        # Set default values
        self._features = {
            "gshadow": False,
        }

        self._features.update({k: True for k in result.stdout_lines})
        self.logger.info("Detected features:", extra={"data": {"Features": self._features}})

        return self._features

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

        self._backup_path = PurePosixPath(result.stdout_lines[-1].strip())

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

    def detect_file_mismatches(self) -> None:
        """
        Shadow binaries modify a number of files, but usually do not modify all of them. This is why we add an
        additional check at the end of the test to verify that the files that should not have been modified are still
        intact.
        """
        self.logger.info(f"Detecting mismatches in shadow files {self._backup_path}")

        for x in self._verify_files:
            result = self.conn.run(
                f"""
                set -ex

                cmp {x['origin']} {self._backup_path}/{x['backup']}
                """,
                log_level=ProcessLogLevel.Error,
                raise_on_error=False,
            )
            if result.rc != 0:
                self.logger.error(f"File mismatch in '{x['origin']}' and '{self._backup_path}/{x['backup']}'")
                result.throw()

    def discard_file(self, origin: str) -> None:
        """
        Discard modified files from the files that should be verified.
        """
        for x in self._verify_files:
            if x["origin"] == origin:
                self._verify_files.remove(x)
                break
