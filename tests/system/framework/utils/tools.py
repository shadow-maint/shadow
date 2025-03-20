"""Run various standard Linux commands on remote host."""

from __future__ import annotations

from typing import Any

import jc
from pytest_mh import MultihostHost, MultihostUtility
from pytest_mh.conn import Process

__all__ = [
    "UnixObject",
    "UnixUser",
    "UnixGroup",
    "IdEntry",
    "PasswdEntry",
    "ShadowEntry",
    "GroupEntry",
    "GShadowEntry"
    "InitgroupsEntry",
    "LinuxToolsUtils",
    "KillCommand",
    "GetentUtils",
]


class UnixObject(object):
    """
    Generic Unix object.
    """

    def __init__(self, id: int | None, name: str | None) -> None:
        """
        :param id: Object ID.
        :type id: int | None
        :param name: Object name.
        :type name: str | None
        """
        self.id: int | None = id
        """
        ID.
        """

        self.name: str | None = name
        """
        Name.
        """

    def __str__(self) -> str:
        return f'({self.id},"{self.name}")'

    def __repr__(self) -> str:
        return str(self)

    def __eq__(self, o: object) -> bool:
        if isinstance(o, str):
            return o == self.name
        elif isinstance(o, int):
            return o == self.id
        elif isinstance(o, tuple):
            if len(o) != 2 or not isinstance(o[0], int) or not isinstance(o[1], str):
                raise NotImplementedError(f"Unable to compare {type(o)} with {self.__class__}")

            (id, name) = o
            return id == self.id and name == self.name
        elif isinstance(o, UnixObject):
            # Fallback to identity comparison
            return NotImplemented

        raise NotImplementedError(f"Unable to compare {type(o)} with {self.__class__}")


class UnixUser(UnixObject):
    """
    Unix user.
    """

    pass


class UnixGroup(UnixObject):
    """
    Unix group.
    """

    pass


class IdEntry(object):
    """
    Result of ``id``
    """

    def __init__(self, user: UnixUser, group: UnixGroup, groups: list[UnixGroup]) -> None:
        self.user: UnixUser = user
        """
        User information.
        """

        self.group: UnixGroup = group
        """
        Primary group.
        """

        self.groups: list[UnixGroup] = groups
        """
        Secondary groups.
        """

    def memberof(self, groups: int | str | tuple[int, str] | list[int | str | tuple[int, str]]) -> bool:
        """
        Check if the user is member of give group(s).

        Group specification can be either a single gid or group name. But it can
        be also a tuple of (gid, name) where both gid and name must match or list
        of groups where the user must be member of all given groups.

        :param groups: _description_
        :type groups: int | str | tuple
        :return: _description_
        :rtype: bool
        """
        if isinstance(groups, (int, str, tuple)):
            return groups in self.groups

        return all(x in self.groups for x in groups)

    def __str__(self) -> str:
        return f"{{user={str(self.user)},group={str(self.group)},groups={str(self.groups)}}}"

    def __repr__(self) -> str:
        return str(self)

    @classmethod
    def FromDict(cls, d: dict[str, Any]) -> IdEntry:
        user = UnixUser(d["uid"]["id"], d["uid"].get("name", None))
        group = UnixGroup(d["gid"]["id"], d["gid"].get("name", None))
        groups = []

        for secondary_group in d["groups"]:
            groups.append(UnixGroup(secondary_group["id"], secondary_group.get("name", None)))

        return cls(user, group, groups)

    @classmethod
    def FromOutput(cls, stdout: str) -> IdEntry:
        jcresult = jc.parse("id", stdout)

        if not isinstance(jcresult, dict):
            raise TypeError(f"Unexpected type: {type(jcresult)}, expecting dict")

        return cls.FromDict(jcresult)


class PasswdEntry(object):
    """
    Result of ``getent passwd``
    """

    def __init__(self, name: str, password: str, uid: int, gid: int, gecos: str, home: str, shell: str) -> None:
        self.name: str | None = name
        """
        User name.
        """

        self.password: str | None = password
        """
        User password.
        """

        self.uid: int = uid
        """
        User id.
        """

        self.gid: int = gid
        """
        Group id.
        """

        self.gecos: str | None = gecos
        """
        GECOS.
        """

        self.home: str | None = home
        """
        Home directory.
        """

        self.shell: str | None = shell
        """
        Login shell.
        """

    def __str__(self) -> str:
        return f"({self.name}:{self.password}:{self.uid}:{self.gid}:{self.gecos}:{self.home}:{self.shell})"

    def __repr__(self) -> str:
        return str(self)

    @classmethod
    def FromDict(cls, d: dict[str, Any]) -> PasswdEntry:
        return cls(
            name=d.get("username", None),
            password=d.get("password", None),
            uid=d.get("uid", None),
            gid=d.get("gid", None),
            gecos=d.get("comment", None),
            home=d.get("home", None),
            shell=d.get("shell", None),
        )

    @classmethod
    def FromOutput(cls, stdout: str) -> PasswdEntry:
        result = jc.parse("passwd", stdout)

        if not isinstance(result, list):
            raise TypeError(f"Unexpected type: {type(result)}, expecting list")

        if len(result) != 1:
            raise ValueError("More then one entry was returned")

        return cls.FromDict(result[0])


class ShadowEntry(object):
    """
    Result of ``getent shadow``
    """

    def __init__(
        self,
        name: str,
        password: str,
        last_changed: int,
        min_days: int,
        max_days: int,
        warn_days: int,
        inactivity_days: int,
        expiration_date: int,
    ) -> None:
        self.name: str | None = name
        """
        User name.
        """

        self.password: str | None = password
        """
        User password.
        """

        self.last_changed: int = last_changed
        """
        Last password change.
        """

        self.min_days: int = min_days
        """
        Minimum number of days before a password change is allowed.
        """

        self.max_days: int = max_days
        """
        Maximum number of days a password is valid.
        """

        self.warn_days: int = warn_days
        """
        Number of days to warn the user before the password expires.
        """

        self.inactivity_days: int | None = inactivity_days
        """
        Number of days after a password expires before the account is disabled.
        """

        self.expiration_date: int | None = expiration_date
        """
        The account expiration date, expressed as the number of days since 1970-01-01 00:00:00 UTC.
        """

    def __str__(self) -> str:
        return (
            f"({self.name}:{self.password}:{self.last_changed}:"
            f"{self.min_days}:{self.max_days}:{self.warn_days}:"
            f"{self.inactivity_days}:{self.expiration_date}:)"
        )

    def __repr__(self) -> str:
        return str(self)

    @classmethod
    def FromDict(cls, d: dict[str, Any]) -> ShadowEntry:
        return cls(
            name=d.get("username", None),
            password=d.get("password", None),
            last_changed=d.get("last_changed", None),
            min_days=d.get("minimum", None),
            max_days=d.get("maximum", None),
            warn_days=d.get("warn", None),
            inactivity_days=d.get("inactive", None),
            expiration_date=d.get("expire", None),
        )

    @classmethod
    def FromOutput(cls, stdout: str) -> ShadowEntry:
        result = jc.parse("shadow", stdout)

        if not isinstance(result, list):
            raise TypeError(f"Unexpected type: {type(result)}, expecting list")

        if len(result) != 1:
            raise ValueError("More then one entry was returned")

        return cls.FromDict(result[0])


class GroupEntry(object):
    """
    Result of ``getent group``
    """

    def __init__(self, name: str, password: str, gid: int, members: list[str]) -> None:
        self.name: str | None = name
        """
        Group name.
        """

        self.password: str | None = password
        """
        Group password.
        """

        self.gid: int = gid
        """
        Group id.
        """

        self.members: list[str] = members
        """
        Group members.
        """

    def __str__(self) -> str:
        return f'({self.name}:{self.password}:{self.gid}:{",".join(self.members)})'

    def __repr__(self) -> str:
        return str(self)

    @classmethod
    def FromDict(cls, d: dict[str, Any]) -> GroupEntry:
        return cls(
            name=d.get("group_name", None),
            password=d.get("password", None),
            gid=d.get("gid", None),
            members=d.get("members", []),
        )

    @classmethod
    def FromOutput(cls, stdout: str) -> GroupEntry:
        result = jc.parse("group", stdout)

        if not isinstance(result, list):
            raise TypeError(f"Unexpected type: {type(result)}, expecting list")

        if len(result) != 1:
            raise ValueError("More then one entry was returned")

        return cls.FromDict(result[0])


class GShadowEntry(object):
    """
    Result of ``getent gshadow``
    """

    def __init__(
        self,
        name: str,
        password: str,
        administrators: str,
        members: str,
    ) -> None:
        self.name: str | None = name
        """
        Group name.
        """

        self.password: str | None = password
        """
        Group password.
        """

        self.administrators: int = administrators
        """
        Group administrators.
        """

        self.members: int = members
        """
        Group members.
        """

    def __str__(self) -> str:
        return (
            f"({self.name}:{self.password}:{self.administrators}:"
            f"{self.members})"
        )

    def __repr__(self) -> str:
        return str(self)

    @classmethod
    def FromDict(cls, d: dict[str, Any]) -> GShadowEntry:
        return cls(
            name=d.get("group_name", None),
            password=d.get("password", None),
            administrators=d.get("administrators", None),
            members=d.get("members", []),
        )

    @classmethod
    def FromOutput(cls, stdout: str) -> GShadowEntry:
        result = jc.parse("gshadow", stdout)

        if not isinstance(result, list):
            raise TypeError(f"Unexpected type: {type(result)}, expecting list")

        if len(result) != 1:
            raise ValueError("More then one entry was returned")

        return cls.FromDict(result[0])


class InitgroupsEntry(object):
    """
    Result of ``getent initgroups``

    If user does not exist or does not have any supplementary groups then ``self.groups`` is empty.
    """

    def __init__(self, name: str, groups: list[int]) -> None:
        self.name: str = name
        """
        Exact username for which ``initgroups`` was called
        """

        self.groups: list[int] = groups
        """
        Group ids that ``name`` is member of.
        """

    def __str__(self) -> str:
        return f'({self.name}:{",".join([str(i) for i in self.groups])})'

    def __repr__(self) -> str:
        return str(self)

    def memberof(self, groups: list[int]) -> bool:
        """
        Check if the user is member of given groups.

        This method checks only supplementary groups not the primary group.

        :param groups: List of group ids
        :type groups: list[int]
        :return: If user is member of all given groups True, otherwise False.
        :rtype: bool
        """

        return all(x in self.groups for x in groups)

    @classmethod
    def FromDict(cls, d: dict[str, Any]) -> InitgroupsEntry:
        return cls(
            name=d["name"],
            groups=d.get("groups", []),
        )

    @classmethod
    def FromOutput(cls, stdout: str) -> InitgroupsEntry:
        result: list[str] = stdout.split()

        dictionary: dict[str, str | list[int]] = {}
        dictionary["name"] = result[0]

        if len(result) > 1:
            dictionary["groups"] = [int(x) for x in result[1:]]

        return cls.FromDict(dictionary)


class LinuxToolsUtils(MultihostUtility[MultihostHost]):
    """
    Run various standard commands on remote host.
    """

    def __init__(self, host: MultihostHost) -> None:
        """
        :param host: Remote host.
        :type host: MultihostHost
        """
        super().__init__(host)

        self.getent: GetentUtils = GetentUtils(host)
        """
        Run ``getent`` command.
        """

    def id(self, name: str | int) -> IdEntry | None:
        """
        Run ``id`` command.

        :param name: User name or id.
        :type name: str | int
        :return: id data, None if not found
        :rtype: IdEntry | None
        """
        command = self.host.conn.exec(["id", name], raise_on_error=False)
        if command.rc != 0:
            return None

        return IdEntry.FromOutput(command.stdout)

    def grep(self, pattern: str, paths: str | list[str], args: list[str] | None = None) -> bool:
        """
        Run ``grep`` command.

        :param pattern: Pattern to match.
        :type pattern: str
        :param paths: Paths to search.
        :type paths: str | list[str]
        :param args: Additional arguments to ``grep`` command, defaults to None.
        :type args: list[str] | None, optional
        :return: True if grep returned 0, False otherwise.
        :rtype: bool
        """
        if args is None:
            args = []

        paths = [paths] if isinstance(paths, str) else paths
        command = self.host.conn.exec(["grep", *args, pattern, *paths])

        return command.rc == 0


class KillCommand(object):
    def __init__(self, host: MultihostHost, process: Process, pid: int) -> None:
        self.host = host
        self.process = process
        self.pid = pid
        self.__killed: bool = False

    def kill(self) -> None:
        if self.__killed:
            return

        self.host.conn.exec(["kill", self.pid])
        self.__killed = True

    def __enter__(self) -> KillCommand:
        return self

    def __exit__(self, exception_type, exception_value, traceback) -> None:
        self.kill()
        self.process.wait()


class GetentUtils(MultihostUtility[MultihostHost]):
    """
    Interface to getent command.
    """

    def __init__(self, host: MultihostHost) -> None:
        """
        :param host: Remote host.
        :type host: MultihostHost
        """
        super().__init__(host)

    def passwd(self, name: str | int, *, service: str | None = None) -> PasswdEntry | None:
        """
        Call ``getent passwd $name``

        :param name: User name or id.
        :type name: str | int
        :param service: Service used, defaults to None
        :type service: str | None
        :return: passwd data, None if not found
        :rtype: PasswdEntry | None
        """
        return self.__exec(PasswdEntry, "passwd", name, service)

    def shadow(self, name: str | int, *, service: str | None = None) -> ShadowEntry | None:
        """
        Call ``getent shadow $name``

        :param name: User name or id.
        :type name: str | int
        :param service: Service used, defaults to None
        :type service: str | None
        :return: shadow data, None if not found
        :rtype: ShadowEntry | None
        """
        return self.__exec(ShadowEntry, "shadow", name, service)

    def group(self, name: str | int, *, service: str | None = None) -> GroupEntry | None:
        """
        Call ``getent group $name``

        :param name: Group name or id.
        :type name: str | int
        :param service: Service used, defaults to None
        :type service: str | None
        :return: group data, None if not found
        :rtype: PasswdEntry | None
        """
        return self.__exec(GroupEntry, "group", name, service)

    def gshadow(self, name: str | int, *, service: str | None = None) -> GShadowEntry | None:
        """
        Call ``getent gshadow $name``

        :param name: Group name or id.
        :type name: str | int
        :param service: Service used, defaults to None
        :type service: str | None
        :return: group data, None if not found
        :rtype: GShadowEntry | None
        """
        return self.__exec(GShadowEntry, "gshadow", name, service)

    def initgroups(self, name: str, *, service: str | None = None) -> InitgroupsEntry:
        """
        Call ``getent initgroups $name``

        If ``name`` does not exist, group list is empty. This is standard behavior of ``getent initgroups``

        :param name: User name.
        :type name: str
        :param service: Service used, defaults to None
        :type service: str | None
        :return: Initgroups data
        :rtype: InitgroupsEntry
        """
        return self.__exec(InitgroupsEntry, "initgroups", name, service)

    def __exec(self, cls, cmd: str, name: str | int, service: str | None = None) -> Any:
        args = []
        if service is not None:
            args = ["-s", service]

        command = self.host.conn.exec(["getent", *args, cmd, name], raise_on_error=False)
        if command.rc != 0:
            return None

        return cls.FromOutput(command.stdout)
