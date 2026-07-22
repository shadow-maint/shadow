"""shadow multihost host."""

from __future__ import annotations

from pathlib import PurePosixPath
from typing import Any

from pytest_mh import MultihostUtility, mh_utility_used
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

        self._verify_files: list[dict[str, str]] = [
            {"origin": "/etc/passwd", "backup": "passwd"},
            {"origin": "/etc/shadow", "backup": "shadow"},
            {"origin": "/etc/group", "backup": "group"},
            {"origin": "/etc/gshadow", "backup": "gshadow"},
        ]
        """Files to verify for mismatch."""

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

    def _restore_home_directory(self, home_backup_path: str) -> None:
        """
        Restore /home directory, handling mount point scenarios.

        If /home is a mount point, do selective cleanup by removing only
        directories that don't exist in the backup. Otherwise, do full restore.

        :param backup_path: Path to the backup directory
        """
        mount_check = self.conn.run(
            "mountpoint -q /home",
            log_level=ProcessLogLevel.Error,
            raise_on_error=False,
        )

        if mount_check.rc == 0:
            self.logger.info("/home is a mount point, doing selective cleanup")

            backup_dirs = self.conn.run(
                f"find '{home_backup_path}' -mindepth 1 -maxdepth 1 -type d -printf '%f\\n' || true",
                log_level=ProcessLogLevel.Error,
                raise_on_error=False,
            )
            backup_dir_list = set(line for line in backup_dirs.stdout_lines if line.strip())

            current_dirs = self.conn.run(
                "find /home -mindepth 1 -maxdepth 1 -type d -printf '%f\\n' || true",
                log_level=ProcessLogLevel.Error,
                raise_on_error=False,
            )
            current_dir_list = set(line for line in current_dirs.stdout_lines if line.strip())

            dirs_to_remove = current_dir_list - backup_dir_list
            for dir_name in dirs_to_remove:
                if dir_name:
                    self.logger.info(f"Removing extra directory: /home/{dir_name}")
                    self.conn.run(
                        f"rm --force --recursive '/home/{dir_name}'",
                        log_level=ProcessLogLevel.Error,
                    )

            self.conn.run(
                f"""
                set -e
                if ls '{home_backup_path}'/* >/dev/null 2>&1; then
                    cp --force --recursive '{home_backup_path}'/* /home/
                fi

                if ls '{home_backup_path}'/.[^.]* >/dev/null 2>&1; then
                    cp --force --recursive '{home_backup_path}'/.[^.]* /home/
                fi
                """,
                log_level=ProcessLogLevel.Error,
                raise_on_error=False,
            )
        else:
            self.logger.info("/home is not a mount point, doing full restore")
            self.conn.run(
                f"""
                set -ex
                function restore {{
                    rm --force --recursive "$2"
                    if [ -d "$1" ] || [ -f "$1" ]; then
                        cp --force --archive "$1" "$2"
                    fi
                }}
                restore "{home_backup_path}" /home
                """,
                log_level=ProcessLogLevel.Error,
            )

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
            restore "{backup_path}/secure" /var/log/secure
            """,
            log_level=ProcessLogLevel.Error,
        )
        self._restore_home_directory(f"{backup_path}/home")

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


class KeyValueFileConfig(MultihostUtility[ShadowHost]):
    """
    Manage a key-value configuration file with dictionary-like access.

    Usage:
        # Set a value
        config["KEY"] = "value"

        # Get a value
        value = config["KEY"]

        # Remove an option
        del config["KEY"]

    The file is backed up on setup and restored on teardown. If the file
    or its parent directory does not exist, it is created on setup and
    removed again on teardown.
    """

    def __init__(self, host: ShadowHost, path: str, separator: str = " ") -> None:
        """
        :param host: Multihost host.
        :type host: ShadowHost
        :param path: Path to the configuration file.
        :type path: str
        :param separator: Separator between key and value, ``" "`` or ``"="``.
        :type separator: str
        """
        super().__init__(host)
        self.path: str = path
        self.separator: str = separator
        self._backup_path: str | None = None
        self._file_existed: bool = False
        self._dir_created: str | None = None

    def _ensure_exists(self) -> None:
        """
        Make sure the parent directory and the file exist, creating them
        when missing. Anything created here is removed on teardown.
        """
        parent = str(PurePosixPath(self.path).parent)
        if not self.host.fs.exists(parent):
            self.host.conn.run(f"mkdir -p '{parent}'", log_level=ProcessLogLevel.Error)
            if self._dir_created is None:
                self._dir_created = parent

        if not self.host.fs.exists(self.path):
            self.host.conn.run(f"touch '{self.path}'", log_level=ProcessLogLevel.Error)

    def setup(self) -> None:
        """
        Backup the original file.
        Called automatically by the framework.
        """
        if self.host.fs.exists(self.path):
            self._file_existed = True
            result = self.host.conn.run(
                f"""
                set -ex
                backup_path=`mktemp`
                cp '{self.path}' $backup_path
                echo $backup_path
                """,
                log_level=ProcessLogLevel.Error,
            )
            self._backup_path = result.stdout_lines[-1].strip()

        self._ensure_exists()

    def teardown(self) -> None:
        """
        Restore the original file.
        Called automatically by the framework.
        """
        if self._file_existed:
            if self._backup_path and self.host.fs.exists(self._backup_path):
                self.host.conn.run(f"mv '{self._backup_path}' '{self.path}'")
        else:
            self.host.conn.run(f"rm -f '{self.path}'", raise_on_error=False)

        if self._dir_created is not None:
            self.host.conn.run(f"rmdir '{self._dir_created}'", raise_on_error=False)

    @mh_utility_used
    def __getitem__(self, key: str) -> str:
        """
        Get a value from the file.

        :param key: Option name (e.g., 'CREATE_HOME')
        :type key: str
        :return: Current value
        :rtype: str
        """
        self._ensure_exists()

        if self.separator == "=":
            cmd = f"grep -E '^{key}=' {self.path} | tail -1 | cut -d= -f2-"
        else:
            cmd = f"grep -E '^{key}\\s+' {self.path} | tail -1 | awk '{{print $2}}'"

        result = self.host.conn.run(cmd, raise_on_error=False)
        if result.rc != 0 or not result.stdout.strip():
            return ""
        return result.stdout.strip()

    @mh_utility_used
    def __setitem__(self, key: str, value: str) -> None:
        """
        Set a value in the file.

        :param key: Option name (e.g., 'CREATE_HOME')
        :type key: str
        :param value: Value to set (can contain special characters like /, &, etc.)
        :type value: str
        """
        self._ensure_exists()
        self.logger.info(f"Setting {key}={value} in {self.path} on {self.host.hostname}")

        sep = self.separator
        grep_match = "=" if sep == "=" else "\\s"
        awk_match = "=" if sep == "=" else "\\\\s"

        # Escape special characters for awk
        escaped_value = value.replace("/", "\\/")

        self.host.conn.run(
            f"""
            if grep -q '^{key}{grep_match}' {self.path}; then
                awk -v key="{key}" -v val="{escaped_value}" \\
                    '{{if ($0 ~ "^" key "{awk_match}") print key "{sep}" val; else print $0}}' \\
                    {self.path} > {self.path}.tmp && mv {self.path}.tmp {self.path}
            elif grep -q '^#\\s*{key}{grep_match}' {self.path}; then
                awk -v key="{key}" -v val="{escaped_value}" \\
                    '{{if ($0 ~ "^#\\\\s*" key "{awk_match}") print key "{sep}" val; else print $0}}' \\
                    {self.path} > {self.path}.tmp && mv {self.path}.tmp {self.path}
            else
                echo '{key}{sep}{value}' >> {self.path}
            fi
            """,
            log_level=ProcessLogLevel.Error,
        )

    @mh_utility_used
    def __delitem__(self, key: str) -> None:
        """
        Remove an option from the file (comment it out).

        :param key: Option name to remove
        :type key: str
        """
        self._ensure_exists()
        self.logger.info(f"Removing {key} from {self.path} on {self.host.hostname}")

        match = "=" if self.separator == "=" else "\\s"
        self.host.conn.run(f"sed -i 's/^{key}{match}.*/#&/' {self.path}", log_level=ProcessLogLevel.Error)
