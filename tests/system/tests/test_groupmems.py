"""
Test groupmems
"""

from __future__ import annotations

import pytest

from framework.roles.shadow import Shadow
from framework.topology import KnownTopology


@pytest.mark.topology(KnownTopology.Shadow)
def test_groupmems__add_user_as_root(shadow: Shadow):
    """
    :title: Add user to group as root user
    :setup:
        1. Create a test user
        2. Create a test group
    :steps:
        1. Add user to group using groupmems as root
        2. Check group entry
        3. Check gshadow entry
    :expectedresults:
        1. User is successfully added to group
        2. group entry for the group exists and the attributes are correct
        3. gshadow entry for the user exists and the attributes are correct
    :customerscenario: False
    """
    shadow.useradd("tuser")
    shadow.groupadd("tgroup")

    shadow.groupmems("-g tgroup -a tuser")

    result = shadow.tools.getent.group("tgroup")
    assert result is not None, "Group should be found"
    assert result.name == "tgroup", "Incorrect groupname"
    assert "tuser" in result.members, "User should be member of group"

    if shadow.host.features["gshadow"]:
        result = shadow.tools.getent.gshadow("tgroup")
        assert result is not None, "Group should be found"
        assert result.name == "tgroup", "Incorrect groupname"
        assert result.password == "!", "Incorrect password"
