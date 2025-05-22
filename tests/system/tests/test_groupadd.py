"""
Test groupadd
"""

from __future__ import annotations

import pytest

from framework.roles.shadow import Shadow
from framework.topology import KnownTopology


@pytest.mark.topology(KnownTopology.Shadow)
def test_groupadd__add_group(shadow: Shadow):
    """
    :title: Basic group creation
    :setup:
        1. Create group
    :steps:
        1. Group exists and GID is 1000
    :expectedresults:
        1. Group is found and GID matches
    :customerscenario: False
    """
    shadow.groupadd("tgroup")

    result = shadow.tools.getent.group("tgroup")
    assert result is not None, "Group should be found"
    assert result.name == "tgroup", "Incorrect groupname"
    assert result.gid == 1000, "Incorrect GID"
