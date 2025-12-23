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
        1. Check group entry
        2. Check gshadow entry
    :expectedresults:
        1. group entry for the user exists and the attributes are correct
        2. gshadow entry for the user exists and the attributes are correct
    :customerscenario: False
    """
    shadow.groupadd("tgroup")
    shadow.groupmod("-g 1001 tgroup")

    group_entry = shadow.tools.getent.group("tgroup")
    assert group_entry is not None, "Group should be found"
    assert group_entry.name == "tgroup", "Incorrect groupname"
    assert group_entry.gid == 1001, "Incorrect GID"

    if shadow.host.features["gshadow"]:
        gshadow_entry = shadow.tools.getent.gshadow("tgroup")
        assert gshadow_entry is not None, "Group should be found"
        assert gshadow_entry.name == "tgroup", "Incorrect groupname"
        assert gshadow_entry.password == "!", "Incorrect password"


@pytest.mark.topology(KnownTopology.Shadow)
def test_groupmod__u_option_empty_string_clears_members(shadow: Shadow):
    """
    :title: Test groupmod -U option with empty user list
    :setup:
        1. Create test group
    :steps:
        1. Run groupmod with -U option and empty string parameter
        2. Verify group exists after operation
        3. Confirm group has no members
    :expectedresults:
        1. groupmod -U '' command completes successfully
        2. Group entry remains valid and accessible
        3. Group member list is empty (no users assigned to group)
    :customerscenario: False
    """
    shadow.groupadd("tgroup")
    shadow.groupmod("-U '' tgroup")

    group_entry = shadow.tools.getent.group("tgroup")
    assert group_entry is not None, "Group should be found"
    assert group_entry.name == "tgroup", "Incorrect groupname"
    assert not group_entry.members, "Group should have no members"

    if shadow.host.features["gshadow"]:
        gshadow_entry = shadow.tools.getent.gshadow("tgroup")
        assert gshadow_entry is not None, "Group should be found"
        assert gshadow_entry.name == "tgroup", "Incorrect groupname"
        assert not gshadow_entry.members, "Group should have no members"


@pytest.mark.topology(KnownTopology.Shadow)
def test_groupmod__u_option_with_user_list(shadow: Shadow):
    """
    :title: Test groupmod -U option with user list to set group membership
    :setup:
        1. Create three test users
        2. Create test group
    :steps:
        1. Set group membership to all three users using groupmod -U
        2. Verify all three users are members in group and gshadow entry
        3. Modify group membership to only two users using groupmod -U
        4. Verify updated membership in group and gshadow entries
    :expectedresults:
        1. Initial groupmod -U command sets membership correctly for all three users
        2. group and gshadow entries show correct membership
        3. Second groupmod -U command updates membership correctly
        4. Updated group and gshadow entries reflect new membership
    :customerscenario: False
    """
    shadow.useradd("tuser1")
    shadow.useradd("tuser2")
    shadow.useradd("tuser3")
    shadow.groupadd("tgroup")
    shadow.groupmod("-U tuser1,tuser2,tuser3 tgroup")

    group_entry = shadow.tools.getent.group("tgroup")
    assert group_entry is not None, "Group should be found"
    assert group_entry.name == "tgroup", "Incorrect groupname"
    assert len(group_entry.members) == 3, f"Group should have 3 members, but has {len(group_entry.members)}"
    assert "tuser1" in group_entry.members, "tuser1 should be a member of tgroup"
    assert "tuser2" in group_entry.members, "tuser2 should be a member of tgroup"
    assert "tuser3" in group_entry.members, "tuser3 should be a member of tgroup"

    if shadow.host.features["gshadow"]:
        gshadow_entry = shadow.tools.getent.gshadow("tgroup")
        assert gshadow_entry is not None, "Group should be found"
        assert gshadow_entry.name == "tgroup", "Incorrect groupname"
        assert len(gshadow_entry.members) == 3, f"Group should have 3 members, but has {len(gshadow_entry.members)}"
        assert "tuser1" in gshadow_entry.members, "tuser1 should be a member of tgroup"
        assert "tuser2" in gshadow_entry.members, "tuser2 should be a member of tgroup"
        assert "tuser3" in gshadow_entry.members, "tuser3 should be a member of tgroup"

    shadow.groupmod("-U tuser1,tuser2 tgroup")

    group_entry = shadow.tools.getent.group("tgroup")
    assert group_entry is not None, "Group should be found"
    assert group_entry.name == "tgroup", "Incorrect groupname"
    assert len(group_entry.members) == 2, f"Group should have 2 members, but has {len(group_entry.members)}"
    assert "tuser1" in group_entry.members, "tuser1 should be a member of tgroup"
    assert "tuser2" in group_entry.members, "tuser2 should be a member of tgroup"

    if shadow.host.features["gshadow"]:
        gshadow_entry = shadow.tools.getent.gshadow("tgroup")
        assert gshadow_entry is not None, "Group should be found"
        assert gshadow_entry.name == "tgroup", "Incorrect groupname"
        assert len(gshadow_entry.members) == 2, f"Group should have 2 members, but has {len(gshadow_entry.members)}"
        assert "tuser1" in gshadow_entry.members, "tuser1 should be a member of tgroup"
        assert "tuser2" in gshadow_entry.members, "tuser2 should be a member of tgroup"
