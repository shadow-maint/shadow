"""
Test pwconv
"""

from __future__ import annotations

import re
from pathlib import Path

import pytest

from framework.misc import days_since_epoch, shadow_password_pattern
from framework.roles.shadow import Shadow
from framework.topology import KnownTopology


@pytest.mark.topology(KnownTopology.Shadow)
def test_pwconv__basic_conversion(shadow: Shadow):
    """
    :title: Basic passwd to shadow conversion
    :setup:
        1. Remove existing passwd and shadow files
        2. Copy passwd file with old format (passwords in passwd attribute)
    :steps:
        1. Convert passwd file
        2. Check passwd entry
        3. Check shadow file exists with correct permissions
        4. Check shadow entries
    :expectedresults:
        1. passwd file converted successfully
        2. passwd entry exists with correct passwd field
        3. shadow file exists with correct permissions
        4. shadow entries exist with correct attributes
    :customerscenario: False
    """
    shadow.host.fs.rm("/etc/passwd")
    shadow.host.fs.rm("/etc/shadow")

    data_file = Path(__file__).parent.parent / "data" / "test_pwconv" / "passwd_old_format"
    shadow.host.fs.upload(str(data_file.resolve()), "/etc/passwd")

    shadow.pwconv()

    passwd_entry = shadow.tools.getent.passwd("tuser1")
    assert passwd_entry is not None, "tuser1 should still exist in passwd"
    assert passwd_entry.password == "x", "tuser1 passwd should have 'x' after pwconv"

    assert shadow.host.fs.exists("/etc/shadow"), "/etc/shadow should exist"
    shadow_stat = shadow.host.fs.stat("/etc/shadow")
    assert shadow_stat.access == "0400", f"Shadow file should have 0400 permissions, got {shadow_stat.access}"
    assert shadow_stat.user == "root", f"Shadow file should be owned by root, got {shadow_stat.user}"

    shadow_root = shadow.tools.getent.shadow("root")
    assert shadow_root is not None, "User should be found"
    assert shadow_root.name == "root", "Incorrect username"
    assert shadow_root.password is not None, "Password should not be None"
    assert re.match(shadow_password_pattern(), shadow_root.password), "Incorrect password"
    assert shadow_root.last_changed == days_since_epoch(), "Incorrect last changed"
    assert shadow_root.max_days == 99999, "Incorrect max days"
    assert shadow_root.warn_days == 7, "Incorrect warn days"

    shadow_daemon = shadow.tools.getent.shadow("daemon")
    assert shadow_daemon is not None, "User should be found"
    assert shadow_daemon.name == "daemon", "Incorrect username"
    assert shadow_daemon.password == "*", "Incorrect password"
    assert shadow_daemon.last_changed == days_since_epoch(), "Incorrect last changed"
    assert shadow_daemon.max_days == 99999, "Incorrect max days"
    assert shadow_daemon.warn_days == 7, "Incorrect warn days"

    shadow_tuser1 = shadow.tools.getent.shadow("tuser1")
    assert shadow_tuser1 is not None, "User should be found"
    assert shadow_tuser1.name == "tuser1", "Incorrect username"
    assert shadow_tuser1.password is not None, "Password should not be None"
    assert re.match(shadow_password_pattern(), shadow_tuser1.password), "Incorrect password"
    assert shadow_tuser1.last_changed == days_since_epoch(), "Incorrect last changed"
    assert shadow_tuser1.max_days == 99999, "Incorrect max days"
    assert shadow_tuser1.warn_days == 7, "Incorrect warn days"
