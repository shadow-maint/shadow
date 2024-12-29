"""
Test groupmod
"""

from __future__ import annotations

import pytest

from framework.roles.shadow import Shadow
from framework.topology import KnownTopology


@pytest.mark.topology(KnownTopology.Shadow)
def test_groupmod__change_gid(shadow: Shadow):
    """
    :title: Change the GID of a group
    :setup:
        1. Create group
        2. Change GID
    :steps:
        1. Group exists and GID is 1001
    :expectedresults:
        1. Group is found and GID matches
    :customerscenario: False
    """
    shadow.groupadd("tgroup")
    shadow.groupmod("-g 1001 tgroup")

    result = shadow.tools.getent.group("tgroup")
    assert result is not None, "Group should be found"
    assert result.name == "tgroup", "Incorrect groupname"
    assert result.gid == 1001, "Incorrect GID"
