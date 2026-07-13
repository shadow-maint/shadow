"""
Test grpconv
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
def test_grpconv__basic_conversion(shadow: Shadow):
    """
    :title: Basic group to shadow conversion
    :setup:
        1. Remove existing group and gshadow files
        2. Copy group file with old format (passwords in group attribute)
    :steps:
        1. Convert group file
        2. Check group entries
        3. Check gshadow file exists with correct permissions
        4. Check gshadow entries
    :expectedresults:
        1. group file converted successfully
        2. group entriy exist with correct passwd field
        3. gshadow file exists with correct permissions
        4. gshadow entries exist with correct attributes
    :customerscenario: False
    """
    shadow.host.fs.rm("/etc/group")
    shadow.host.fs.rm("/etc/gshadow")

    data_file = Path(__file__).parent.parent / "data" / "test_grpconv" / "group_old_format"
    shadow.host.fs.upload(str(data_file.resolve()), "/etc/group")

    shadow.grpconv()

    group_entry = shadow.tools.getent.group("tgroup1")
    assert group_entry is not None, "tgroup1 should exist in group"
    assert group_entry.name == "tgroup1", "Incorrect groupname"
    assert group_entry.password == "x", "tgroup1 group should have 'x' after grpconv"
    assert group_entry.gid == 1001, "Incorrect GID"

    assert shadow.host.fs.exists("/etc/gshadow"), "/etc/gshadow should exist"
    gshadow_stat = shadow.host.fs.stat("/etc/gshadow")
    assert gshadow_stat.access == "0400", f"gshadow file should have 0400 permissions, got {gshadow_stat.access}"
    assert gshadow_stat.user == "root", f"gshadow file should be owned by root, got {gshadow_stat.user}"

    gshadow_root = shadow.tools.getent.gshadow("root")
    assert gshadow_root is not None, "root should exist in gshadow"
    assert gshadow_root.name == "root", "Incorrect groupname"
    assert gshadow_root.password == "*", "Incorrect password"

    gshadow_daemon = shadow.tools.getent.gshadow("daemon")
    assert gshadow_daemon is not None, "daemon should exist in gshadow"
    assert gshadow_daemon.name == "daemon", "Incorrect groupname"
    assert gshadow_daemon.password == "*", "Incorrect password"

    gshadow_tgroup1 = shadow.tools.getent.gshadow("tgroup1")
    assert gshadow_tgroup1 is not None, "tgroup1 should exist in gshadow"
    assert gshadow_tgroup1.name == "tgroup1", "Incorrect groupname"
    assert gshadow_tgroup1.password is not None, "Password should not be None"
    assert re.match(shadow_password_pattern(), gshadow_tgroup1.password), "Incorrect password"
