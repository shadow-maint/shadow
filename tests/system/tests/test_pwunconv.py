"""
Test pwunconv
"""

from __future__ import annotations

import re
from pathlib import Path

import pytest

from framework.misc import shadow_password_pattern
from framework.roles.shadow import Shadow
from framework.topology import KnownTopology


@pytest.mark.topology(KnownTopology.Shadow)
def test_pwunconv__basic_conversion(shadow: Shadow):
    """
    :title: Basic shadow to passwd conversion
    :setup:
        1. Remove existing passwd and shadow files
        2. Copy passwd and shadow files
    :steps:
        1. Convert shadow format back to passwd format
        2. Check shadow file is removed
        3. Check passwd entries
    :expectedresults:
        1. shadow format converted successfully
        2. shadow file is removed
        3. passwd entries exist with correct attributes
    :customerscenario: False
    """
    shadow.host.fs.rm("/etc/passwd")
    shadow.host.fs.rm("/etc/shadow")

    test_dir = Path(__file__).parent.parent
    passwd_file = test_dir / "data" / "test_pwunconv" / "passwd"
    shadow.host.fs.upload(str(passwd_file.resolve()), "/etc/passwd")
    shadow_file = test_dir / "data" / "test_pwunconv" / "shadow"
    shadow.host.fs.upload(str(shadow_file.resolve()), "/etc/shadow")

    shadow.pwunconv()

    assert not shadow.host.fs.exists("/etc/shadow"), "/etc/shadow should not exist after pwunconv"

    passwd_root = shadow.tools.getent.passwd("root")
    assert passwd_root is not None, "root should exist in passwd"
    assert passwd_root.name == "root", "Incorrect username"
    assert passwd_root.password is not None, "Password should not be None"
    assert re.match(shadow_password_pattern(), passwd_root.password), "Incorrect password"
    assert passwd_root.uid == 0, "Incorrect UID"
    assert passwd_root.gid == 0, "Incorrect GID"
    assert passwd_root.home == "/root", "Incorrect home"
    assert passwd_root.shell == "/bin/bash", "Incorrect shell"

    passwd_daemon = shadow.tools.getent.passwd("daemon")
    assert passwd_daemon is not None, "daemon should exist in passwd"
    assert passwd_daemon.name == "daemon", "Incorrect username"
    assert passwd_daemon.password == "*", "Incorrect password"
    assert passwd_daemon.uid == 1, "Incorrect UID"
    assert passwd_daemon.gid == 1, "Incorrect GID"
    assert passwd_daemon.home == "/usr/sbin", "Incorrect home"
    assert passwd_daemon.shell == "/bin/sh", "Incorrect shell"

    passwd_tuser1 = shadow.tools.getent.passwd("tuser1")
    assert passwd_tuser1 is not None, "tuser1 should exist in passwd"
    assert passwd_tuser1.name == "tuser1", "Incorrect username"
    assert passwd_tuser1.password is not None, "Password should not be None"
    assert re.match(shadow_password_pattern(), passwd_tuser1.password), "Incorrect password"
    assert passwd_tuser1.uid == 1001, "Incorrect UID"
    assert passwd_tuser1.gid == 1001, "Incorrect GID"
    assert passwd_tuser1.home == "/home/tuser1", "Incorrect home"
    assert passwd_tuser1.shell == "/bin/bash", "Incorrect shell"
