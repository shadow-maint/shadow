"""shadow multihost role."""

from __future__ import annotations

import shlex
from typing import Dict, Tuple

from pytest_mh.conn import ProcessLogLevel, ProcessResult

from ..hosts.shadow import ShadowHost
from ..misc.errors import ExpectScriptError
from .base import BaseLinuxRole

__all__ = [
    "Shadow",
]

DEFAULT_INTERACTIVE_TIMEOUT: int = 60
"""Default timeout for interactive sessions."""


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
        self.logger.info(
            f'Changing user password expiry information on user "{args_dict["name"]}" on {self.host.hostname}'
        )
        cmd = self.host.conn.run("chage " + args[0], log_level=ProcessLogLevel.Error)

        self.host.discard_file("/etc/passwd")
        self.host.discard_file("/etc/shadow")

        return cmd

    def newusers(self, *args, users_data: str | None = None) -> ProcessResult:
        """
        Update or create new users in batch.

        Updates or creates multiple users by reading account information in passwd
        format. If `users_data` is provided, it's passed via stdin and takes
        precedence; otherwise, the command reads from a file specified in `args`.
        """
        if users_data:
            cmd_args = " ".join(args)
            self.logger.info(f"Creating users from stdin on {self.host.hostname}")
            cmd = self.host.conn.run(f"echo '{users_data}' | newusers {cmd_args}", log_level=ProcessLogLevel.Error)
        else:
            args_dict = self._parse_args(args)
            self.logger.info(f'Creating users from "{args_dict["name"]}" on {self.host.hostname}')
            cmd = self.host.conn.run("newusers " + args[0], log_level=ProcessLogLevel.Error)

        self.host.discard_file("/etc/passwd")
        self.host.discard_file("/etc/shadow")
        self.host.discard_file("/etc/group")
        self.host.discard_file("/etc/gshadow")

        return cmd

    def groupmems(self, *args, run_as: str = "root", password: str | None = None) -> ProcessResult:
        """
        Administer members of a user's primary group.

        The groupmems command allows management of group membership lists.
        If `run_as` is provided, then the `password` must be provided and the command groupmems run under this user.
        If `run_as` isn't provided, then the command is run under `root`.
        """
        args_dict = self._parse_args(args)

        if run_as == "root":
            self.logger.info(f'Administer {args_dict["name"]} group membership as root on {self.host.hostname}')
            cmd = self.host.conn.run("groupmems " + args[0], log_level=ProcessLogLevel.Error)
        else:
            self.logger.info(f'Administer {args_dict["name"]} group membership on {self.host.hostname}')
            cmd = self.host.conn.run(
                f"echo '{password}' | su - {run_as} -c 'groupmems {args[0]}'", log_level=ProcessLogLevel.Error
            )

        self.host.discard_file("/etc/group")
        self.host.discard_file("/etc/gshadow")

        return cmd

    def newgrp(self, *args, run_as: str = "root") -> Tuple[ProcessResult, int]:
        """
        Log in to a new group.

        The newgrp command is used to change the current group ID during a login session.
        Returns the process result and the group ID after the change.
        """
        args_dict = self._parse_args(args)

        self.logger.info(f'Changing {run_as} to group "{args_dict["name"]}" on {self.host.hostname}')

        # Use expect to handle the interactive newgrp session
        result = self.host.conn.expect(
            rf"""
            set timeout {DEFAULT_INTERACTIVE_TIMEOUT}
            set prompt "\[#\$>\] $"

            if {{ "{run_as}" eq "root" }} {{
                spawn newgrp {args[0]}
            }} else {{
                spawn su - {run_as}
                expect {{
                    -re $prompt {{send "newgrp {args[0]}\n"}}
                    timeout {{puts "expect result: Timeout waiting for su prompt"; exit 201}}
                    eof {{puts "expect result: Unexpected end of file"; exit 202}}
                }}
            }}

            expect {{
                -re $prompt {{send "id -g\n"}}
                timeout {{puts "expect result: Timeout waiting for newgrp prompt"; exit 201}}
                eof {{puts "expect result: Unexpected end of file"; exit 202}}
            }}

            expect {{
                -re "(\[0-9\]+)" {{
                    set gid $expect_out(1,string)
                    send "exit\n"
                    puts "newgrp_gid:$gid"
                }}
                timeout {{puts "expect result: Timeout waiting for id output"; exit 201}}
                eof {{puts "expect result: Unexpected end of file"; exit 202}}
            }}

            if {{ "{run_as}" ne "root" }} {{
                expect {{
                    -re $prompt {{
                        send "exit\n"
                    }}
                    timeout {{puts "expect result: Timeout waiting for original su prompt"; exit 201}}
                }}
            }}

            expect {{
                eof {{exit 0}}
                timeout {{exit 201}}
            }}
            """,
            verbose=False,
        )

        if result.rc > 200:
            raise ExpectScriptError(result.rc)

        gid_line = None
        for line in result.stdout_lines:
            if "newgrp_gid:" in line:
                gid_line = line
                break

        if gid_line is None:
            raise ValueError("Current GID is required for newgrp")

        current_gid = int(gid_line.split(":")[1])

        return result, current_gid
