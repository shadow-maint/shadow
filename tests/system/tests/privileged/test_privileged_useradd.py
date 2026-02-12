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
