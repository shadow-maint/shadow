"""shadow multihost role."""

from __future__ import annotations

import shlex
from typing import Dict

from pytest_mh.conn import ProcessLogLevel, ProcessResult

from ..hosts.shadow import ShadowHost
from .base import BaseLinuxRole

__all__ = [
    "Shadow",
]


class Shadow(BaseLinuxRole[ShadowHost]):
    """
    shadow role.

    Provides unified Python API for managing and testing shadow.
    """

    def __init__(self, *args, **kwargs) -> None:
        """
        Set up the environment.
        """
        super().__init__(*args, **kwargs)

    def teardown(self) -> None:
        """
        Detect file mismatches before cleaning up the environment.
        """
        self.host.detect_file_mismatches()
        """
        Clean up the environment.
        """
        super().teardown()

    def _parse_args(self, *args) -> Dict[str, str]:
        args_list = shlex.split(*args[0])
        name = args_list[-1]

        return {"name": name}

    def useradd(self, *args) -> ProcessResult:
        """
        Create user.
        """
        args_dict = self._parse_args(args)
        self.logger.info(f'Creating user "{args_dict["name"]}" on {self.host.hostname}')
        cmd = self.host.conn.run("useradd " + args[0], log_level=ProcessLogLevel.Error)

        self.host.discard_file("/etc/passwd")
        self.host.discard_file("/etc/shadow")
        self.host.discard_file("/etc/group")
        self.host.discard_file("/etc/gshadow")

        return cmd

    def usermod(self, *args) -> ProcessResult:
        """
        Modify user.
        """
        args_dict = self._parse_args(args)
        self.logger.info(f'Modifying user "{args_dict["name"]}" on {self.host.hostname}')
        cmd = self.host.conn.run("usermod " + args[0], log_level=ProcessLogLevel.Error)

        self.host.discard_file("/etc/passwd")
        self.host.discard_file("/etc/shadow")
        self.host.discard_file("/etc/group")
        self.host.discard_file("/etc/gshadow")

        return cmd

    def userdel(self, *args) -> ProcessResult:
        """
        Delete user.
        """
        args_dict = self._parse_args(args)
        self.logger.info(f'Deleting user "{args_dict["name"]}" on {self.host.hostname}')
        cmd = self.host.conn.run("userdel " + args[0], log_level=ProcessLogLevel.Error)

        self.host.discard_file("/etc/passwd")
        self.host.discard_file("/etc/shadow")
        self.host.discard_file("/etc/group")
        self.host.discard_file("/etc/gshadow")

        return cmd

    def groupadd(self, *args) -> ProcessResult:
        """
        Create group.
        """
        args_dict = self._parse_args(args)
        self.logger.info(f'Creating group "{args_dict["name"]}" on {self.host.hostname}')
        cmd = self.host.conn.run("groupadd " + args[0], log_level=ProcessLogLevel.Error)

        self.host.discard_file("/etc/group")
        self.host.discard_file("/etc/gshadow")

        return cmd

    def groupmod(self, *args) -> ProcessResult:
        """
        Modify group.
        """
        args_dict = self._parse_args(args)
        self.logger.info(f'Modifying group "{args_dict["name"]}" on {self.host.hostname}')
        cmd = self.host.conn.run("groupmod " + args[0], log_level=ProcessLogLevel.Error)

        self.host.discard_file("/etc/group")
        self.host.discard_file("/etc/gshadow")

        return cmd

    def groupdel(self, *args) -> ProcessResult:
        """
        Delete group.
        """
        args_dict = self._parse_args(args)
        self.logger.info(f'Deleting group "{args_dict["name"]}" on {self.host.hostname}')
        cmd = self.host.conn.run("groupdel " + args[0], log_level=ProcessLogLevel.Error)

        self.host.discard_file("/etc/group")
        self.host.discard_file("/etc/gshadow")

        return cmd

    def chage(self, *args) -> ProcessResult:
        """
        Change user password expiry information.
        """
        args_dict = self._parse_args(args)
        self.logger.info(f'Changing user password expiry information on user "{args_dict["name"]}" on {self.host.hostname}')
        cmd = self.host.conn.run("chage " + args[0], log_level=ProcessLogLevel.Error)

        self.host.discard_file("/etc/passwd")
        self.host.discard_file("/etc/shadow")

        return cmd
