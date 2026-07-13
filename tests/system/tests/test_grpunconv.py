"""
Test grpunconv
"""

from __future__ import annotations

import re
from pathlib import Path

import pytest

from framework.misc import shadow_password_pattern
from framework.roles.shadow import Shadow
from framework.topology import KnownTopology


@pytest.mark.topology(KnownTopology.Shadow)
@pytest.mark.builtwith(shadow="gshadow")
def test_grpunconv__basic_conversion(shadow: Shadow):
    """
    :title: Basic shadow to group conversion
    :setup:
        1. Remove existing group and gshadow files
        2. Copy group and gshadow files
    :steps:
        1. Convert shadow format back to group format
        2. Check gshadow file is removed
        3. Check group entries
    :expectedresults:
        1. shadow format converted successfully
        2. gshadow file is removed
        3. group entries exist with correct attributes
    :customerscenario: False
    """
    shadow.host.fs.rm("/etc/group")
    shadow.host.fs.rm("/etc/gshadow")

    test_dir = Path(__file__).parent.parent
    group_file = test_dir / "data" / "test_grpunconv" / "group"
    shadow.host.fs.upload(str(group_file.resolve()), "/etc/group")
    gshadow_file = test_dir / "data" / "test_grpunconv" / "gshadow"
    shadow.host.fs.upload(str(gshadow_file.resolve()), "/etc/gshadow")

    shadow.grpunconv()

    assert not shadow.host.fs.exists("/etc/gshadow"), "/etc/gshadow should not exist after grpunconv"

    group_root = shadow.tools.getent.group("root")
    assert group_root is not None, "root should exist in group"
    assert group_root.name == "root", "Incorrect groupname"
    assert group_root.password == "*", "Incorrect password"
    assert group_root.gid == 0, "Incorrect GID"

    group_daemon = shadow.tools.getent.group("daemon")
    assert group_daemon is not None, "daemon should exist in group"
    assert group_daemon.name == "daemon", "Incorrect groupname"
    assert group_daemon.password == "*", "Incorrect password"
    assert group_daemon.gid == 1, "Incorrect GID"

    group_tgroup1 = shadow.tools.getent.group("tgroup1")
    assert group_tgroup1 is not None, "tgroup1 should exist in group"
    assert group_tgroup1.name == "tgroup1", "Incorrect groupname"
    assert group_tgroup1.password is not None, "Password should not be None"
    assert re.match(shadow_password_pattern(), group_tgroup1.password), "Incorrect password"
    assert group_tgroup1.gid == 1001, "Incorrect GID"
