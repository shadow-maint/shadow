"""Base classes and objects for shadow specific multihost roles."""

from __future__ import annotations

from typing import Any, Generic, TypeGuard, TypeVar

from pytest_mh import MultihostRole
from pytest_mh.cli import CLIBuilder
from pytest_mh.conn import Bash, Shell
from pytest_mh.conn.ssh import SSHClient
from pytest_mh.utils.coredumpd import Coredumpd
from pytest_mh.utils.firewall import Firewalld
from pytest_mh.utils.fs import LinuxFileSystem
from pytest_mh.utils.journald import JournaldUtils
from pytest_mh.utils.tc import LinuxTrafficControl

from ..hosts.base import BaseHost
from ..utils.tools import LinuxToolsUtils

HostType = TypeVar("HostType", bound=BaseHost)
RoleType = TypeVar("RoleType", bound=MultihostRole)


__all__ = [
    "HostType",
    "RoleType",
    "DeleteAttribute",
    "BaseObject",
    "BaseRole",
    "BaseLinuxRole",
]


class DeleteAttribute(object):
    """
    This class is used to distinguish between setting an attribute to an empty
    value and deleting it completely.
    """

    pass


class BaseObject(Generic[HostType, RoleType]):
    """
    Base class for object management classes (like users or groups).

    It provides shortcuts to low level functionality to easily enable execution
    of remote commands. It also defines multiple helper methods that are shared
    across roles.
    """

    def __init__(self, role: RoleType) -> None:
        self.role: RoleType = role
        """Multihost role object."""

        self.host: HostType = role.host
        """Multihost host object."""

        self.cli: CLIBuilder = self.host.cli
        """Command line builder to easy build command line for execution."""


class BaseRole(MultihostRole[HostType]):
    """
    Base role class. Roles are the main interface to the remote hosts that can
    be directly accessed in test cases as fixtures.

    All changes to the remote host that were done through the role object API
    are automatically reverted when a test is finished.
    """

    Delete: DeleteAttribute = DeleteAttribute()
    """
    Use this to indicate that you want to delete an attribute instead of setting
    it to an empty value.
    """

    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)

    def is_delete_attribute(self, value: Any) -> TypeGuard[DeleteAttribute]:
        """
        Return ``True`` if the value is :attr:`DeleteAttribute`

        :param value: Value to test.
        :type value: Any
        :return: Return ``True`` if the value is :attr:`DeleteAttribute`
        :rtype: TypeGuard[DeleteAttribute]
        """
        return isinstance(value, DeleteAttribute)

    @property
    def features(self) -> dict[str, bool]:
        """
        Features supported by the role.
        """
        return self.host.features

    def ssh(self, user: str, password: str, *, shell: Shell | None = None) -> SSHClient:
        """
        Open SSH connection to the host as given user.

        :param user: Username.
        :type user: str
        :param password: User password.
        :type password: str
        :param shell: Shell that will run the commands, defaults to ``None`` (= ``Bash``)
        :type shell: Shell | None, optional
        :return: SSH client connection.
        :rtype: SSHClient
        """
        if shell is None:
            shell = Bash()

        host = self.host.hostname
        port = 22

        if isinstance(self.host.conn, SSHClient):
            host = getattr(self.host.conn, "host", host)
            port = getattr(self.host.conn, "port", 22)

        return SSHClient(
            host=host,
            port=port,
            user=user,
            password=password,
            shell=shell,
            logger=self.logger,
        )


class BaseLinuxRole(BaseRole[HostType]):
    """
    Base linux role.
    """

    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)

        self.fs: LinuxFileSystem = LinuxFileSystem(self.host)
        """
        File system manipulation.
        """

        self.firewall: Firewalld = Firewalld(self.host).postpone_setup()
        """
        Configure firewall using firewalld.
        """

        self.tc: LinuxTrafficControl = LinuxTrafficControl(self.host).postpone_setup()
        """
        Traffic control manipulation.
        """

        self.tools: LinuxToolsUtils = LinuxToolsUtils(self.host)
        """
        Standard tools interface.
        """

        self.journald: JournaldUtils = JournaldUtils(self.host)
        """
        Journald utilities.
        """

        coredumpd_config = self.host.config.get("coredumpd", {})
        coredumpd_mode = coredumpd_config.get("mode", "ignore")
        coredumpd_filter = coredumpd_config.get("filter", None)

        self.coredumpd: Coredumpd = Coredumpd(self.host, self.fs, mode=coredumpd_mode, filter=coredumpd_filter)
        """
        Coredumpd utilities.
        """
