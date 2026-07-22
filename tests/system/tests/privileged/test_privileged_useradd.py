"""
Privileged useradd tests.
"""

from __future__ import annotations

import pytest

from framework.roles.shadow import Shadow
from framework.topology import KnownTopology

pytestmark = [
    pytest.mark.privileged,
    pytest.mark.topology(KnownTopology.ShadowPrivileged),
]


def test_useradd__btrfs_subvolume_home(shadow: Shadow) -> None:
    """
    :title: Btrfs subvolume home
    :steps:
        1. Create a user with --btrfs-subvolume-home
        2. Verify the home directory exists
        3. Verify the home directory is a Btrfs subvolume
    :expectedresults:
        1. The user home directory exists at /home/tuser
        2. /home/tuser is listed as a Btrfs subvolume
    :customerscenario: False
    """
    with shadow.host.btrfs.loopback_mount("/home"):
        shadow.useradd("tuser --btrfs-subvolume-home")

        assert shadow.fs.exists("/home/tuser"), "Home directory should exist"

        assert shadow.host.btrfs.subvolume_exists("/home", "tuser")


def test_useradd__btrfs_subvolume_home_system_user(shadow: Shadow) -> None:
    """
    :title: Btrfs subvolume home is skipped for system users by default
    :steps:
        1. Create a system user, requesting its home directory as a Btrfs subvolume
        2. Verify the user is a system user
        3. Verify the home directory exists
        4. Verify the home directory is not a Btrfs subvolume
    :expectedresults:
        1. The user is created
        2. The UID is in the system range
        3. The user home directory exists at /home/tsysuser
        4. /home/tsysuser is a regular directory, not a subvolume
    :customerscenario: False
    """
    with shadow.host.btrfs.loopback_mount("/home"):
        shadow.useradd("tsysuser -r -m --btrfs-subvolume-home")

        passwd_entry = shadow.tools.getent.passwd("tsysuser")
        assert passwd_entry is not None, "User should be found"
        assert passwd_entry.uid is not None and passwd_entry.uid < 1000, "User should be a system user"

        assert shadow.fs.exists("/home/tsysuser"), "Home directory should exist"

        assert not shadow.host.btrfs.subvolume_exists("/home", "tsysuser"), "Home should not be a subvolume"


def test_useradd__btrfs_subvolume_home_system_user_enabled(shadow: Shadow) -> None:
    """
    :title: Btrfs subvolume home is created for system users with BTRFS_SUBVOLUME_SYSTEM=yes
    :setup:
        1. Enable Btrfs subvolume creation for system users in the useradd defaults
    :steps:
        1. Create a system user, requesting its home directory as a Btrfs subvolume
        2. Verify the home directory exists
        3. Verify the home directory is a Btrfs subvolume
    :expectedresults:
        1. The user is created
        2. The user home directory exists at /home/tsysuser
        3. /home/tsysuser is listed as a Btrfs subvolume
    :customerscenario: False
    """
    with shadow.host.btrfs.loopback_mount("/home"):
        shadow.useradd_defaults["BTRFS_SUBVOLUME_SYSTEM"] = "yes"

        shadow.useradd("tsysuser -r -m --btrfs-subvolume-home")

        assert shadow.fs.exists("/home/tsysuser"), "Home directory should exist"

        assert shadow.host.btrfs.subvolume_exists("/home", "tsysuser"), "Home should be a subvolume"
