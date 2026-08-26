"""
Test pwck
"""

from __future__ import annotations

from pathlib import Path

import pytest

from framework.roles.shadow import Shadow
from framework.topology import KnownTopology


@pytest.mark.topology(KnownTopology.Shadow)
def test_pwck__validates_clean_files(shadow: Shadow):
    """
    :title: pwck validates clean password files in read-only mode
    :setup:
        1. Remove existing passwd, shadow and group files
        2. Copy passwd, shadow and group files
    :steps:
        1. Run pwck integrity check in read-only mode
        2. Verify command exit status
    :expectedresults:
        1. pwck completes successfully
        2. Command succeeds
    :cleanup:
        1. Restore original /etc/group
    :customerscenario: False
    """
    shadow.host.fs.rm("/etc/passwd")
    shadow.host.fs.rm("/etc/shadow")
    shadow.host.fs.rm("/etc/group")

    test_dir = Path(__file__).parent.parent
    passwd_file = test_dir / "data" / "test_pwck" / "passwd"
    shadow.host.fs.upload(str(passwd_file.resolve()), "/etc/passwd")
    shadow_file = test_dir / "data" / "test_pwck" / "shadow"
    shadow.host.fs.upload(str(shadow_file.resolve()), "/etc/shadow")
    group_file = test_dir / "data" / "test_pwck" / "group"
    shadow.host.fs.upload(str(group_file.resolve()), "/etc/group")

    result = shadow.pwck("-r")
    assert result.rc == 0

    # The modified /etc/group will cause the test to fail during teardown, thus restore it
    shadow.host.fs.restore("/etc/group")
