"""
Test grpck
"""

from __future__ import annotations

from pathlib import Path

import pytest

from framework.roles.shadow import Shadow
from framework.topology import KnownTopology


@pytest.mark.topology(KnownTopology.Shadow)
def test_grpck__validates_clean_files(shadow: Shadow):
    """
    :title: grpck validates clean group files in read-only mode
    :setup:
        1. Remove existing group and gshadow files
        2. Copy group and gshadow files
    :steps:
        1. Run grpck integrity check in read-only mode
        2. Verify command exit status
    :expectedresults:
        1. grpck completes successfully
        2. Command succeeds
    :customerscenario: False
    """
    shadow.host.fs.rm("/etc/group")
    shadow.host.fs.rm("/etc/gshadow")

    test_dir = Path(__file__).parent.parent
    group_file = test_dir / "data" / "test_grpck" / "group"
    shadow.host.fs.upload(str(group_file.resolve()), "/etc/group")
    gshadow_file = test_dir / "data" / "test_grpck" / "gshadow"
    shadow.host.fs.upload(str(gshadow_file.resolve()), "/etc/gshadow")

    result = shadow.grpck("-r")
    assert result.rc == 0
