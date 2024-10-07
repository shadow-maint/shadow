"""Base classes and objects for shadow specific multihost hosts."""

from __future__ import annotations

import csv

from pytest_mh import MultihostBackupHost, MultihostHost
from pytest_mh.utils.fs import LinuxFileSystem

from ..config import ShadowMultihostDomain

__all__ = [
    "BaseHost",
    "BaseLinuxHost",
]


class BaseHost(MultihostBackupHost[ShadowMultihostDomain]):
    """
    Base class for all shadow hosts.
    """

    def __init__(self, *args, **kwargs) -> None:
        # restore is handled in topology controllers
        super().__init__(*args, **kwargs)

    @property
    def features(self) -> dict[str, bool]:
        """
        Features supported by the host.
        """
        return {}


class BaseLinuxHost(MultihostHost[ShadowMultihostDomain]):
    """
    Base Linux host.

    Adds linux specific reentrant utilities.
    """

    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)

        self.fs: LinuxFileSystem = LinuxFileSystem(self)
        self._os_release: dict = {}
        self._distro_name: str = "unknown"
        self._distro_major: int = 0
        self._distro_minor: int = 0

    def _distro_information(self):
        """
        Pulls distro information from a host from /ets/os-release
        """
        self.logger.info(f"Detecting distro information on {self.hostname}")
        os_release = self.fs.read("/etc/os-release")
        self._os_release = dict(csv.reader([x for x in os_release.splitlines() if x], delimiter="="))
        if "NAME" in self._os_release:
            self._distro_name = self._os_release["NAME"]
        if "VERSION_ID" not in self._os_release:
            return
        if "." in self._os_release["VERSION_ID"]:
            self._distro_major = int(self._os_release["VERSION_ID"].split(".", maxsplit=1)[0])
            self._distro_minor = int(self._os_release["VERSION_ID"].split(".", maxsplit=1)[1])
        else:
            self._distro_major = int(self._os_release["VERSION_ID"])

    @property
    def distro_name(self) -> str:
        """
        Host distribution

        :return: Distribution name or "unknown"
        :rtype: str
        """
        # NAME item from os-release
        if not self._os_release:
            self._distro_information()
        return self._distro_name

    @property
    def distro_major(self) -> int:
        """
        Host distribution major version

        :return: Major version
        :rtype: int
        """
        # First part of VERSION_ID from os-release
        # Returns zero when could not detect
        if not self._os_release:
            self._distro_information()
        return self._distro_major

    @property
    def distro_minor(self) -> int:
        """
        Host distribution minor version

        :return: Minor version
        :rtype: int
        """
        # Second part of VERSION_ID from os-release
        # Returns zero when no minor version is present
        if not self._os_release:
            self._distro_information()
        return self._distro_minor
