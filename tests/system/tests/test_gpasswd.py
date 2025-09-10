"""
Test gpasswd
"""

from __future__ import annotations

import pytest

from framework.roles.shadow import Shadow
from framework.topology import KnownTopology


@pytest.mark.topology(KnownTopology.Shadow)
def test_gpasswd__add_user_group_member_as_root(shadow: Shadow):
    """
    :title: Add user to group membership as root user
    :setup:
        1. Create test user and group
    :steps:
        1. Add user to group membership using gpasswd as root
        2. Check group and gshadow entry
    :expectedresults:
        1. User is added to group
        2. group and gshadow entry values are correct
    :customerscenario: False
    """
    shadow.useradd("tuser")
    shadow.groupadd("tgroup")

    shadow.gpasswd("-a tuser tgroup")

    group_entry = shadow.tools.getent.group("tgroup")
    assert group_entry is not None, "Group should be found"
    assert group_entry.name == "tgroup", "Incorrect groupname"
    assert "tuser" in group_entry.members, "User should be member of group"

    if shadow.host.features["gshadow"]:
        gshadow_entry = shadow.tools.getent.gshadow("tgroup")
        assert gshadow_entry is not None, "Group should be found"
        assert gshadow_entry.name == "tgroup", "Incorrect groupname"
        assert "tuser" in gshadow_entry.members, "User should be member of group"


@pytest.mark.topology(KnownTopology.Shadow)
@pytest.mark.builtwith(shadow="gshadow")
def test_gpasswd__add_user_group_administrator_as_root(shadow: Shadow):
    """
    :title: Add user as group administrator
    :setup:
        1. Create test user and group
    :steps:
        1. Add user as group administrator using gpasswd
        2. Check gshadow entry
    :expectedresults:
        1. User is added as group administrator
        2. gshadow entry values are correct
    :customerscenario: False
    """
    shadow.useradd("tuser")
    shadow.groupadd("tgroup")

    shadow.gpasswd("-A tuser tgroup")

    gshadow_entry = shadow.tools.getent.gshadow("tgroup")
    assert gshadow_entry is not None, "Group should be found"
    assert gshadow_entry.name == "tgroup", "Incorrect groupname"
    assert "tuser" in gshadow_entry.administrators, "User should be administrator of group"
