"""
Test useradd
"""

from __future__ import annotations

import pytest

from framework.misc import days_since_epoch
from framework.roles.shadow import Shadow
from framework.topology import KnownTopology


@pytest.mark.topology(KnownTopology.Shadow)
def test_useradd__add_user(shadow: Shadow):
    """
    :title: Basic user creation
    :setup:
        1. Create user
    :steps:
        1. Check passwd entry
        2. Check shadow entry
        3. Check group entry
        4. Check gshadow entry
        5. Check home folder
    :expectedresults:
        1. passwd entry for the user exists and the attributes are correct
        2. shadow entry for the user exists and the attributes are correct
        3. group entry for the user exists and the attributes are correct
        4. gshadow entry for the user exists and the attributes are correct
        5. Home folder exists
    :customerscenario: False
    """
    shadow.useradd("tuser")

    result = shadow.tools.getent.passwd("tuser")
    assert result is not None, "User should be found"
    assert result.name == "tuser", "Incorrect username"
    assert result.password == "x", "Incorrect password"
    assert result.uid == 1000, "Incorrect UID"
    assert result.gid == 1000, "Incorrect GID"
    assert result.home == "/home/tuser", "Incorrect home"
    if "Debian" in shadow.host.distro_name:
        assert result.shell == "/bin/sh", "Incorrect shell"
    else:
        assert result.shell == "/bin/bash", "Incorrect shell"

    result = shadow.tools.getent.shadow("tuser")
    assert result is not None, "User should be found"
    assert result.name == "tuser", "Incorrect username"
    assert result.password == "!", "Incorrect password"
    assert result.last_changed == days_since_epoch(), "Incorrect last changed"
    assert result.min_days == 0, "Incorrect min days"
    assert result.max_days == 99999, "Incorrect max days"
    assert result.warn_days == 7, "Incorrect warn days"

    result = shadow.tools.getent.group("tuser")
    assert result is not None, "Group should be found"
    assert result.name == "tuser", "Incorrect groupname"
    assert result.gid == 1000, "Incorrect GID"

    if shadow.host.features["gshadow"]:
        result = shadow.tools.getent.gshadow("tuser")
        assert result is not None, "User should be found"
        assert result.name == "tuser", "Incorrect username"
        assert result.password == "!", "Incorrect password"

    assert shadow.fs.exists("/home/tuser"), "Home folder should be found"


@pytest.mark.topology(KnownTopology.Shadow)
def test_useradd__recreate_deleted_user(shadow: Shadow):
    """
    :title: Recreate deleted user
    :setup:
        1. Create user
        2. Delete the user
        3. Create again the deleted user
    :steps:
        1. User exists and UID is 1000
        2. Group exists and GID is 1000
        3. Home folder exists
    :expectedresults:
        1. User is found and UID matches
        2. Group is found and GID matches
        3. Home folder is found
    :customerscenario: False
    """
    shadow.useradd("tuser")
    shadow.userdel("tuser")
    shadow.useradd("tuser")

    result = shadow.tools.id("tuser")
    assert result is not None, "User should be found"
    assert result.user.name == "tuser", "Incorrect username"
    assert result.user.id == 1000, "Incorrect UID"

    result = shadow.tools.getent.group("tuser")
    assert result is not None, "Group should be found"
    assert result.name == "tuser", "Incorrect groupname"
    assert result.gid == 1000, "Incorrect GID"

    assert shadow.fs.exists("/home/tuser"), "Home folder should be found"


# TODO: only check in environments with SELinux?
@pytest.mark.ticket(gh=940)
@pytest.mark.topology(KnownTopology.Shadow)
def test_useradd__chroot_option(shadow: Shadow):
    """
    :title: User creation with chroot option
    :setup:
        1. Create basic environment
        2. Check initial SELinux context
        3. Create user
    :steps:
        1. Check SELinux context after user creation
    :expectedresults:
        1. Initial and ending SELinux context should be the same
    :customerscenario: True
    """
    chroot_dir = "/tmp/newroot"
    shadow.fs.mkdir_p(f"{chroot_dir}/etc")
    shadow.fs.touch(f"{chroot_dir}/etc/passwd")
    shadow.fs.touch(f"{chroot_dir}/etc/group")
    # TODO: check more files? shadow, gshadow, subuid, subgid?

    init_passwd = shadow.selinux.get_file_context(f"{chroot_dir}/etc/passwd")
    init_group = shadow.selinux.get_file_context(f"{chroot_dir}/etc/group")

    shadow.useradd(f"tuser -R {chroot_dir}")

    end_passwd = shadow.selinux.get_file_context(f"{chroot_dir}/etc/passwd")
    end_group = shadow.selinux.get_file_context(f"{chroot_dir}/etc/group")

    assert init_passwd == end_passwd, "passwd SELinux context should be the same"
    assert init_group == end_group, "group SELinux context should be the same"


# TODO: only check in environments with SELinux?
@pytest.mark.ticket(gh=940)
@pytest.mark.topology(KnownTopology.Shadow)
def test_useradd__prefix_option(shadow: Shadow):
    """
    :title: User creation with prefix option
    :setup:
        1. Create basic environment
        2. Check initial SELinux context
        3. Create user
    :steps:
        1. Check SELinux context after user creation
    :expectedresults:
        1. Initial and ending SELinux context should be the same
    :customerscenario: True
    """
    prefix_dir = "/tmp/newroot"
    shadow.fs.mkdir_p(f"{prefix_dir}/etc")
    shadow.fs.touch(f"{prefix_dir}/etc/passwd")
    shadow.fs.touch(f"{prefix_dir}/etc/group")
    # TODO: check more files? shadow, gshadow, subuid, subgid?

    init_passwd = shadow.selinux.get_file_context(f"{prefix_dir}/etc/passwd")
    init_group = shadow.selinux.get_file_context(f"{prefix_dir}/etc/group")

    shadow.useradd(f"tuser -P {prefix_dir}")

    end_passwd = shadow.selinux.get_file_context(f"{prefix_dir}/etc/passwd")
    end_group = shadow.selinux.get_file_context(f"{prefix_dir}/etc/group")

    assert init_passwd == end_passwd, "passwd SELinux context should be the same"
    assert init_group == end_group, "group SELinux context should be the same"
