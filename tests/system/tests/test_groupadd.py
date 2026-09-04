"""
Test groupadd
"""

from __future__ import annotations

import re

import pytest
from passlib.hash import sha512_crypt
from pytest_mh.conn import ProcessError

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


@pytest.mark.topology(KnownTopology.Shadow)
def test_groupadd__create_group_with_existing_gid(shadow: Shadow):
    """
    :title: Create group with existing GID using -o flag
    :setup:
        1. Create group with specific GID
    :steps:
        1. Check existing group and gshadow entry
        2. Create new group with existing GID
        3. Check new group and gshadow entry
    :expectedresults:
        1. Group entry is found with specific GID along with gshadow entry
        2. groupadd command completes successfully
        3. Group entry is created with existing GID along with gshadow entry
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

    shadow.groupadd("-g 1001 -o tgroup2")

    group_entry = shadow.tools.getent.group("tgroup2")
    assert group_entry is not None, "Group should be found"
    assert group_entry.name == "tgroup2", "Incorrect groupname"
    assert group_entry.gid == 1001, "Group should have duplicate GID"

    if shadow.host.features["gshadow"]:
        gshadow_entry = shadow.tools.getent.gshadow("tgroup2")
        assert gshadow_entry is not None, "Group should be found"
        assert gshadow_entry.name == "tgroup2", "Incorrect groupname"


@pytest.mark.topology(KnownTopology.Shadow)
def test_groupadd__add_group_with_existing_gid(shadow: Shadow):
    """
    :title: Group creation fails when specified GID already exists
    :setup:
        1. Create group with specific GID
    :steps:
        1. Check existing group and gshadow entries
        2. Attempt to create group with existing GID
        3. Verify that groupadd command fails
        4. Check group and gshadow entries
    :expectedresults:
        1. Existing group and gshadow entries are found
        2. Group is not created
        3. groupadd command fails with error (GID already exists)
        4. No group or gshadow entries are found
    :customerscenario: False
    """
    shadow.groupadd("-g 1500 tgroup1")

    existing_group_entry = shadow.tools.getent.group("tgroup1")
    assert existing_group_entry is not None, "Group should be found"
    assert existing_group_entry.name == "tgroup1", "Incorrect groupname"
    assert existing_group_entry.gid == 1500, "Incorrect GID"

    if shadow.host.features["gshadow"]:
        existing_gshadow_entry = shadow.tools.getent.gshadow("tgroup1")
        assert existing_gshadow_entry is not None, "Group should be found"
        assert existing_gshadow_entry.name == "tgroup1", "Incorrect groupname"

    with pytest.raises(ProcessError) as exc_info:
        shadow.groupadd("-g 1500 tgroup2")

    assert exc_info.value.rc == 4, f"Expected return code 4 (GID already exists), got {exc_info.value.rc}"

    group_entry = shadow.tools.getent.group("tgroup2")
    assert group_entry is None, "Group should not be found"

    if shadow.host.features["gshadow"]:
        gshadow_entry = shadow.tools.getent.gshadow("tgroup2")
        assert gshadow_entry is None, "Group should not be found"


@pytest.mark.topology(KnownTopology.Shadow)
@pytest.mark.parametrize(
    "gid_value",
    [
        pytest.param("1001x", id="invalid_gid"),
        pytest.param("-1001", id="negative_gid"),
        pytest.param("4294967295", id="exceeds_maximum_uid"),
    ],
)
def test_groupadd__invalid_gid(shadow: Shadow, gid_value: str):
    """
    :title: Group creation with invalid GID fails
    :setup:
        1. None required
    :steps:
        1. Create group with invalid GID
        2. Verify that groupadd command fails
        3. Check group and gshadow entries
    :expectedresults:
        1. Group is not created
        2. groupadd command fails with error (invalid argument)
        3. No group or gshadow entries are found
    :customerscenario: False
    """
    with pytest.raises(ProcessError) as exc_info:
        shadow.groupadd(f"-g {gid_value} tgroup")

    assert exc_info.value.rc == 3, f"Expected return code 3 (invalid argument), got {exc_info.value.rc}"

    group_entry = shadow.tools.getent.group("tgroup")
    assert group_entry is None, "Group should not be found"

    if shadow.host.features["gshadow"]:
        gshadow_entry = shadow.tools.getent.gshadow("tgroup")
        assert gshadow_entry is None, "Group should not be found"


@pytest.mark.topology(KnownTopology.Shadow)
def test_groupadd__invalid_name(shadow: Shadow):
    """
    :title: Group creation with invalid name fails
    :setup:
        1. None required
    :steps:
        1. Create group with invalid name
        2. Verify that groupadd command fails
        3. Check group and gshadow entries
    :expectedresults:
        1. Group is not created
        2. groupadd command fails with error (invalid argument)
        3. No group or gshadow entries are found
    :customerscenario: False
    """
    with pytest.raises(ProcessError) as exc_info:
        shadow.groupadd("tgroup:x")

    assert exc_info.value.rc == 3, f"Expected return code 3 (invalid argument), got {exc_info.value.rc}"

    group_entry = shadow.tools.getent.group("tgroup")
    assert group_entry is None, "Group should not be found"

    if shadow.host.features["gshadow"]:
        gshadow_entry = shadow.tools.getent.gshadow("tgroup")
        assert gshadow_entry is None, "Group should not be found"


@pytest.mark.topology(KnownTopology.Shadow)
@pytest.mark.parametrize(
    "lock_file",
    [
        pytest.param("/etc/group.lock", id="group_file"),
        pytest.param("/etc/gshadow.lock", id="gshadow_file"),
    ],
)
def test_groupadd__locked_file(shadow: Shadow, lock_file: str):
    """
    :title: Group creation fails when a lock file exists
    :setup:
        1. Create lock file
    :steps:
        1. Attempt to create group
        2. Verify that groupadd command fails
        3. Check group and gshadow entries
    :expectedresults:
        1. Group is not created
        2. groupadd command fails with rc=10 (cannot lock file)
        3. No group or gshadow entries are found
    :customerscenario: False
    """
    shadow.fs.touch(lock_file)

    with pytest.raises(ProcessError) as exc_info:
        shadow.groupadd("tgroup")

    assert exc_info.value.rc == 10, f"Expected rc=10 (cannot lock file), got {exc_info.value.rc}"

    group_entry = shadow.tools.getent.group("tgroup")
    assert group_entry is None, "Group should not be found"

    if shadow.host.features["gshadow"]:
        gshadow_entry = shadow.tools.getent.gshadow("tgroup")
        assert gshadow_entry is None, "Group should not be found"


@pytest.mark.topology(KnownTopology.Shadow)
@pytest.mark.parametrize(
    "key",
    [
        pytest.param("KEY=100", id="invalid_key"),
        pytest.param("GID_MAX", id="no_equals_sign"),
    ],
)
def test_groupadd__invalid_key(shadow: Shadow, key: str):
    """
    :title: Group creation with invalid key fails
    :setup:
        1. None required
    :steps:
        1. Create group with invalid key
        2. Verify that groupadd command fails
        3. Check group and gshadow entries
    :expectedresults:
        1. Group is not created
        2. groupadd command fails with error (invalid argument)
        3. No group or gshadow entries are found
    :customerscenario: False
    """
    with pytest.raises(ProcessError) as exc_info:
        shadow.groupadd(f"-K {key} tgroup")

    assert exc_info.value.rc == 3, f"Expected return code 3 (invalid argument), got {exc_info.value.rc}"

    group_entry = shadow.tools.getent.group("tgroup")
    assert group_entry is None, "Group should not be found"

    if shadow.host.features["gshadow"]:
        gshadow_entry = shadow.tools.getent.gshadow("tgroup")
        assert gshadow_entry is None, "Group should not be found"


@pytest.mark.topology(KnownTopology.Shadow)
def test_groupadd__existing_group(shadow: Shadow):
    """
    :title: Group creation fails when group already exists
    :setup:
        1. Create group
    :steps:
        1. Check existing group and gshadow entries
        2. Attempt to create group
        3. Verify that groupadd command fails
        4. Check existing group and gshadow entries
    :expectedresults:
        1. Existing group and gshadow entries are found
        2. Group is not created
        3. groupadd command fails with error (group already exists)
        4. Existing group and gshadow entries are still found
    :customerscenario: False
    """
    shadow.groupadd("tgroup")

    existing_group_entry = shadow.tools.getent.group("tgroup")
    assert existing_group_entry is not None, "Group should be found"
    assert existing_group_entry.name == "tgroup", "Incorrect groupname"

    if shadow.host.features["gshadow"]:
        existing_gshadow_entry = shadow.tools.getent.gshadow("tgroup")
        assert existing_gshadow_entry is not None, "Group should be found"
        assert existing_gshadow_entry.name == "tgroup", "Incorrect groupname"

    with pytest.raises(ProcessError) as exc_info:
        shadow.groupadd("tgroup")

    assert exc_info.value.rc == 9, f"Expected return code 9 (group already exists), got {exc_info.value.rc}"

    existing_group_entry = shadow.tools.getent.group("tgroup")
    assert existing_group_entry is not None, "Group should be found"
    assert existing_group_entry.name == "tgroup", "Incorrect groupname"

    if shadow.host.features["gshadow"]:
        existing_gshadow_entry = shadow.tools.getent.gshadow("tgroup")
        assert existing_gshadow_entry is not None, "Group should be found"
        assert existing_gshadow_entry.name == "tgroup", "Incorrect groupname"


@pytest.mark.topology(KnownTopology.Shadow)
def test_groupadd__usage(shadow: Shadow):
    """
    :title: Groupadd command displays usage
    :setup:
        1. None required
    :steps:
        1. Run groupadd command
        2. Verify that groupadd command exits successfully
        3. Check usage information
    :expectedresults:
        1. Command runs successfully
        2. groupadd command completes successfully
        3. Usage information is displayed
    :customerscenario: False
    """
    result = shadow.groupadd("-h")
    assert result.rc == 0, f"Expected return code 0(success), got {result.rc}"
    assert "Usage: groupadd [options] GROUP" in result.stdout


@pytest.mark.topology(KnownTopology.Shadow)
@pytest.mark.parametrize(
    "args",
    [
        pytest.param("", id="no_group"),
        pytest.param("tgroup1 tgroup2", id="two_groups"),
    ],
)
def test_groupadd__invalid_arguments(shadow: Shadow, args: str):
    """
    :title: Groupadd command fails with invalid arguments
    :setup:
        1. None required
    :steps:
        1. Attempt to create groups
        2. Verify that groupadd command fails
    :expectedresults:
        1. Groups are not created
        2. groupadd command fails with error (invalid usage)
    :customerscenario: False
    """
    with pytest.raises(ProcessError) as exc_info:
        shadow.groupadd(args)

    assert exc_info.value.rc == 2, f"Expected return code 2(invalid usage), got {exc_info.value.rc}"


@pytest.mark.topology(KnownTopology.Shadow)
def test_groupadd__no_gshadow(shadow: Shadow):
    """
    :title: Group creation succeeds when /etc/gshadow does not exist
    :setup:
        1. Remove /etc/gshadow file
        2. Set FORCE_SHADOW=no in /etc/login.defs
    :steps:
        1. Create group
        2. Check group entry
        3. Check that /etc/gshadow file is not present
    :expectedresults:
        1. Group is created
        2. Group entry is found
        3. /etc/gshadow file is not found
    :customerscenario: False
    """
    shadow.fs.rm("/etc/gshadow")

    shadow.login_defs["FORCE_SHADOW"] = "no"

    shadow.groupadd("tgroup")

    group_entry = shadow.tools.getent.group("tgroup")
    assert group_entry is not None, "Group should be found"
    assert group_entry.name == "tgroup", "Incorrect groupname"

    gshadow_file = shadow.fs.exists("/etc/gshadow")
    assert not gshadow_file, "/etc/gshadow file should not be found"


@pytest.mark.topology(KnownTopology.Shadow)
def test_groupadd__non_unique_requires_gid(shadow: Shadow):
    """
    :title: Groupadd command fails when non-unique option is used without GID
    :setup:
        1. None required
    :steps:
        1. Attempt to create group
        2. Verify that groupadd command fails
        3. Check group and gshadow entries
    :expectedresults:
        1. Group is not created
        2. groupadd command fails with error (invalid usage)
        3. No group or gshadow entries are found
    :customerscenario: False
    """
    with pytest.raises(ProcessError) as exc_info:
        shadow.groupadd("-o tgroup")

    assert exc_info.value.rc == 2, f"Expected return code 2 (invalid usage), got {exc_info.value.rc}"

    group_entry = shadow.tools.getent.group("tgroup")
    assert group_entry is None, "Group should not be found"

    if shadow.host.features["gshadow"]:
        gshadow_entry = shadow.tools.getent.gshadow("tgroup")
        assert gshadow_entry is None, "Group should not be found"


@pytest.mark.topology(KnownTopology.Shadow)
def test_groupadd__invalid_option(shadow: Shadow):
    """
    :title: Group creation fails with invalid option
    :setup:
        1. None required
    :steps:
        1. Attempt to create group
        2. Verify that groupadd command fails
        3. Check group and gshadow entries
    :expectedresults:
        1. Group is not created
        2. groupadd command fails with error (invalid usage)
        3. No group or gshadow entries are found
    :customerscenario: False
    """
    with pytest.raises(ProcessError) as exc_info:
        shadow.groupadd("-invalid tgroup")

    assert exc_info.value.rc == 2, f"Expected return code 2 (invalid usage), got {exc_info.value.rc}"

    group_entry = shadow.tools.getent.group("tgroup")
    assert group_entry is None, "Group should not be found"

    if shadow.host.features["gshadow"]:
        gshadow_entry = shadow.tools.getent.gshadow("tgroup")
        assert gshadow_entry is None, "Group should not be found"


@pytest.mark.topology(KnownTopology.Shadow)
def test_groupadd__add_system_group(shadow: Shadow):
    """
    :title: System group creation assigns GID from the system GID range
    :setup:
        1. None required
    :steps:
        1. Create system group
        2. Check group and gshadow entries
        3. Check GID for system group
    :expectedresults:
        1. System group is created
        2. Group and gshadow entries are found
        3. System group is assigned with GID within system GID range
    :customerscenario: False
    """
    shadow.groupadd("--system sgroup")

    group_entry = shadow.tools.getent.group("sgroup")
    assert group_entry is not None, "System group should be found"
    assert group_entry.name == "sgroup", "Incorrect groupname"
    assert group_entry.gid is not None, "System group should have a GID"
    assert group_entry.gid >= 101, "System group GID should be >= SYS_GID_MIN (101)"
    assert group_entry.gid < 1000, "System group GID should be < GID_MIN (1000)"

    if shadow.host.features["gshadow"]:
        gshadow_entry = shadow.tools.getent.gshadow("sgroup")
        assert gshadow_entry is not None, "System group should be found"
        assert gshadow_entry.name == "sgroup", "Incorrect groupname"


@pytest.mark.topology(KnownTopology.Shadow)
def test_groupadd__no_gids_available(shadow: Shadow):
    """
    :title: Group creation fails when all GIDs in the range are exhausted
    :setup:
        1. Set GID_MIN=2000 and GID_MAX=2001 in /etc/login.defs
        2. Create two groups with GID 2000 and 2001 respectively
    :steps:
        1. Attempt to create new group
        2. Verify that groupadd command fails
        3. Check group and gshadow entries
    :expectedresults:
        1. Group is not created
        2. groupadd command fails with rc=4 (no GID available)
        3. No group or gshadow entries are found
    :customerscenario: False
    """
    shadow.login_defs["GID_MIN"] = "2000"
    shadow.login_defs["GID_MAX"] = "2001"
    shadow.groupadd("-g 2000 tgroup1")
    shadow.groupadd("-g 2001 tgroup2")

    with pytest.raises(ProcessError) as exc_info:
        shadow.groupadd("tgroup3")
    assert exc_info.value.rc == 4, f"Expected rc=4 (no GID available), got {exc_info.value.rc}"

    group_entry = shadow.tools.getent.group("tgroup3")
    assert group_entry is None, "Group should not be found"

    if shadow.host.features["gshadow"]:
        gshadow_entry = shadow.tools.getent.gshadow("tgroup3")
        assert gshadow_entry is None, "Group should not be found"


@pytest.mark.topology(KnownTopology.Shadow)
def test_groupadd__no_system_gids_available(shadow: Shadow):
    """
    :title: System group creation fails when all GIDs in the range are exhausted
    :setup:
        1. Set SYS_GID_MIN=500 and SYS_GID_MAX=501 in /etc/login.defs
        2. Create two system groups with GID 500 and 501 respectively
    :steps:
        1. Attempt to create new system group
        2. Verify that groupadd command fails
        3. Check group and gshadow entries
    :expectedresults:
        1. System group is not created
        2. groupadd command fails with rc=4 (no GID available)
        3. No group or gshadow entries are found
    :customerscenario: False
    """
    shadow.login_defs["SYS_GID_MIN"] = "500"
    shadow.login_defs["SYS_GID_MAX"] = "501"
    shadow.groupadd("-r -g 500 sgroup1")
    shadow.groupadd("-r -g 501 sgroup2")

    with pytest.raises(ProcessError) as exc_info:
        shadow.groupadd("-r sgroup3")
    assert exc_info.value.rc == 4, f"Expected rc=4 (no system GID available), got {exc_info.value.rc}"

    group_entry = shadow.tools.getent.group("sgroup3")
    assert group_entry is None, "System group should not be found"

    if shadow.host.features["gshadow"]:
        gshadow_entry = shadow.tools.getent.gshadow("sgroup3")
        assert gshadow_entry is None, "System group should not be found"
