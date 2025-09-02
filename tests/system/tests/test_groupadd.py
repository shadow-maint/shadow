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
        1. Check group entry
        2. Check gshadow entry
    :expectedresults:
        1. group entry for the user exists and the attributes are correct
        2. gshadow entry for the user exists and the attributes are correct
    :customerscenario: False
    """
    shadow.groupadd("tgroup")

    group_entry = shadow.tools.getent.group("tgroup")
    assert group_entry is not None, "Group should be found"
    assert group_entry.name == "tgroup", "Incorrect groupname"
    assert group_entry.gid == 1000, "Incorrect GID"

    if shadow.host.features["gshadow"]:
        gshadow_entry = shadow.tools.getent.gshadow("tgroup")
        assert gshadow_entry is not None, "Group should be found"
        assert gshadow_entry.name == "tgroup", "Incorrect groupname"
        assert gshadow_entry.password == "!", "Incorrect password"
