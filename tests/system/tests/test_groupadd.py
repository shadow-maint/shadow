"""
Test groupadd
"""

from __future__ import annotations

import re

import pytest
from passlib.hash import sha512_crypt

from framework.misc import shadow_password_pattern
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
    assert group_entry.gid == 1001, "Incorrect GID"

    if shadow.host.features["gshadow"]:
        gshadow_entry = shadow.tools.getent.gshadow("tgroup")
        assert gshadow_entry is not None, "Group should be found"
        assert gshadow_entry.name == "tgroup", "Incorrect groupname"
        assert gshadow_entry.password == "!", "Incorrect password"


@pytest.mark.topology(KnownTopology.Shadow)
def test_groupadd__u_option_empty_string_clears_members(shadow: Shadow):
    """
    :title: Test groupadd -U option with empty user list
    :setup:
        1. None required
    :steps:
        1. Run groupadd with -U option and empty string parameter
        2. Verify group exists after creation
        3. Confirm group has no members
    :expectedresults:
        1. groupadd -U '' command completes successfully
        2. Group entry is created and accessible
        3. Group member list is empty (no users assigned to group)
    :customerscenario: False
    """
    shadow.groupadd("-U '' tgroup")

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
def test_groupadd__set_gid(shadow: Shadow):
    """
    :title: Create group with specific GID using -g flag
    :setup:
        1. Create group with specific GID
    :steps:
        1. Check group entry
    :expectedresults:
        1. Group entry is created with the specified GID
    :customerscenario: False
    """
    shadow.groupadd("-g 1500 tgroup")

    group_entry = shadow.tools.getent.group("tgroup")
    assert group_entry is not None, "Group should be found"
    assert group_entry.name == "tgroup", "Incorrect groupname"
    assert group_entry.gid == 1500, "Incorrect GID"


@pytest.mark.topology(KnownTopology.Shadow)
def test_groupadd__set_password(shadow: Shadow):
    """
    :title: Add group with password using -p flag
    :setup:
        1. Create group with password
    :steps:
        1. Check group entry
        2. Check gshadow entry
    :expectedresults:
        1. Group entry is created
        2. Group's password is correctly set
    :customerscenario: False
    """
    password = "Secret123"
    password_hash = sha512_crypt.hash(password)
    shadow.groupadd(f"-p '{password_hash}' tgroup")

    group_entry = shadow.tools.getent.group("tgroup")
    assert group_entry is not None, "Group should be found"

    if shadow.host.features["gshadow"]:
        gshadow_entry = shadow.tools.getent.gshadow("tgroup")
        assert gshadow_entry is not None, "Group should be found"
        assert gshadow_entry.name == "tgroup", "Incorrect groupname"
        assert gshadow_entry.password is not None, "Password should not be None"
        assert re.match(shadow_password_pattern(), gshadow_entry.password), "Incorrect password"


@pytest.mark.topology(KnownTopology.Shadow)
def test_groupadd__force_group_creation(shadow: Shadow):
    """
    :title: Forced creation of existing group exits successfully
    :setup:
        1. Create group entry
    :steps:
        1. Check group entry
        2. Force create existing group
        3. Check group entry
        4. Check gshadow entry
    :expectedresults:
        1. Group entry is created
        2. groupadd -f command completes successfully
        3. Group entry is unchanged
        4. gshadow entry is unchanged
    :customerscenario: False
    """
    shadow.groupadd("tgroup")
    existing_group_entry = shadow.tools.getent.group("tgroup")
    assert existing_group_entry is not None, "Group should be found"
    shadow.groupadd("-f tgroup")
    group_entry = shadow.tools.getent.group("tgroup")
    assert group_entry is not None, "Group should be found"
    assert group_entry.name == "tgroup", "Incorrect groupname"
    if shadow.host.features["gshadow"]:
        gshadow_entry = shadow.tools.getent.gshadow("tgroup")
        assert gshadow_entry is not None, "Group should be found"
        assert gshadow_entry.name == "tgroup", "Incorrect groupname"


@pytest.mark.topology(KnownTopology.Shadow)
def test_groupadd__force_group_creation_with_existing_gid(shadow: Shadow):
    """
    :title: Forced creation of group assigns unique GID if specified GID exists
    :setup:
        1. Create group with specific GID
    :steps:
        1. Check existing group and gshadow entry
        2. Force create new group with existing GID
        3. Check new group and gshadow entry
    :expectedresults:
        1. Group entry is found with specific GID along with gshadow entry
        2. groupadd command completes successfully
        3. Group entry is created with next available GID along with gshadow entry
    :customerscenario: False
    """
    shadow.groupadd("-g 1001 tgroup1")

    existing_group_entry = shadow.tools.getent.group("tgroup1")
    assert existing_group_entry is not None, "Group should be found"
    assert existing_group_entry.name == "tgroup1", "Incorrect groupname"
    assert existing_group_entry.gid == 1001, "Incorrect GID"

    if shadow.host.features["gshadow"]:
        existing_gshadow_entry = shadow.tools.getent.gshadow("tgroup1")
        assert existing_gshadow_entry is not None, "Group should be found"
        assert existing_gshadow_entry.name == "tgroup1", "Incorrect groupname"

    shadow.groupadd("-g 1001 -f tgroup2")

    group_entry = shadow.tools.getent.group("tgroup2")
    assert group_entry is not None, "Group should be found"
    assert group_entry.name == "tgroup2", "Incorrect groupname"
    assert group_entry.gid == 1002, "Group should have next available unique GID"

    if shadow.host.features["gshadow"]:
        gshadow_entry = shadow.tools.getent.gshadow("tgroup2")
        assert gshadow_entry is not None, "Group should be found"
        assert gshadow_entry.name == "tgroup2", "Incorrect groupname"
