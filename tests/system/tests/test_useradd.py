"""
Test useradd
"""

from __future__ import annotations

import pytest
from pytest_mh.conn import ProcessError

from framework.misc import days_since_epoch
from framework.roles.shadow import Shadow
from framework.topology import KnownTopology


@pytest.mark.topology(KnownTopology.Shadow)
def test_useradd__add_user(shadow: Shadow):
    """
    :title: Basic user creation
    :setup:
        1. Create user
    :steps:
        1. Check passwd entry
        2. Check shadow entry
        3. Check group entry
        4. Check gshadow entry
        5. Check home folder
    :expectedresults:
        1. passwd entry for the user exists and the attributes are correct
        2. shadow entry for the user exists and the attributes are correct
        3. group entry for the user exists and the attributes are correct
        4. gshadow entry for the user exists and the attributes are correct
        5. Home folder exists
    :customerscenario: False
    """
    shadow.useradd("tuser")

    passwd_entry = shadow.tools.getent.passwd("tuser")
    assert passwd_entry is not None, "User should be found"
    assert passwd_entry.name == "tuser", "Incorrect username"
    assert passwd_entry.password == "x", "Incorrect password"
    assert passwd_entry.uid == 1000, "Incorrect UID"
    assert passwd_entry.gid == 1000, "Incorrect GID"
    assert passwd_entry.home == "/home/tuser", "Incorrect home"
    if "Debian" in shadow.host.distro_name:
        assert passwd_entry.shell == "/bin/sh", "Incorrect shell"
    else:
        assert passwd_entry.shell == "/bin/bash", "Incorrect shell"

    shadow_entry = shadow.tools.getent.shadow("tuser")
    assert shadow_entry is not None, "User should be found"
    assert shadow_entry.name == "tuser", "Incorrect username"
    assert shadow_entry.password == "!", "Incorrect password"
    assert shadow_entry.last_changed == days_since_epoch(), "Incorrect last changed"
    assert shadow_entry.min_days == 0, "Incorrect min days"
    assert shadow_entry.max_days == 99999, "Incorrect max days"
    assert shadow_entry.warn_days == 7, "Incorrect warn days"

    group_entry = shadow.tools.getent.group("tuser")
    assert group_entry is not None, "Group should be found"
    assert group_entry.name == "tuser", "Incorrect groupname"
    assert group_entry.gid == 1000, "Incorrect GID"

    if shadow.host.features["gshadow"]:
        gshadow_entry = shadow.tools.getent.gshadow("tuser")
        assert gshadow_entry is not None, "User should be found"
        assert gshadow_entry.name == "tuser", "Incorrect username"
        assert gshadow_entry.password == "!", "Incorrect password"

    assert shadow.fs.exists("/home/tuser"), "Home folder should be found"


@pytest.mark.topology(KnownTopology.Shadow)
def test_useradd__recreate_deleted_user(shadow: Shadow):
    """
    :title: Recreate deleted user
    :setup:
        1. Create user
        2. Delete the user
        3. Create again the deleted user
    :steps:
        1. User exists and UID is 1000
        2. Group exists and GID is 1000
        3. Home folder exists
    :expectedresults:
        1. User is found and UID matches
        2. Group is found and GID matches
        3. Home folder is found
    :customerscenario: False
    """
    shadow.useradd("tuser")
    shadow.userdel("tuser")
    shadow.useradd("tuser")

    id_entry = shadow.tools.id("tuser")
    assert id_entry is not None, "User should be found"
    assert id_entry.user.name == "tuser", "Incorrect username"
    assert id_entry.user.id == 1000, "Incorrect UID"

    group_entry = shadow.tools.getent.group("tuser")
    assert group_entry is not None, "Group should be found"
    assert group_entry.name == "tuser", "Incorrect groupname"
    assert group_entry.gid == 1000, "Incorrect GID"

    assert shadow.fs.exists("/home/tuser"), "Home folder should be found"


@pytest.mark.topology(KnownTopology.Shadow)
@pytest.mark.parametrize(
    "expiration_date, expected_date",
    [
        ("0", 0),  # 1970-01-01
        ("1", 1),  # 1970-01-02
        ("20089", 20089),  # 2025-01-01
        ("1000000", 1000000),  # This will happen in a very long time
        ("1970-01-01", 0),
        ("1970-01-02", 1),
        ("2025-01-01", 20089),
    ],
)
def test_useradd__set_expire_date_with_valid_date(shadow: Shadow, expiration_date: str, expected_date: int | None):
    """
    :title: Create account with valid expiration date
    :setup:
        1. Create user account with valid expiration date
    :steps:
        1. Check user exists and expiration date
    :expectedresults:
        1. User is found and expiration date is valid
    :customerscenario: False
    """
    shadow.useradd(f"tuser1 -e {expiration_date}")

    result = shadow.tools.getent.shadow("tuser1")
    assert result is not None, "User should be found"
    assert result.name == "tuser1", "Incorrect username"
    assert result.expiration_date == expected_date, "Incorrect expiration date"


@pytest.mark.topology(KnownTopology.Shadow)
@pytest.mark.parametrize(
    "expiration_date",
    [
        "-2",  # Dates can't be in negative numbers
        "-1000",  # Dates can't be in negative numbers
        "2025-18-18",  # That month and day don't exist
        "1969-01-01",  # This is before 1970-01-01
        "2025-13-01",  # That month doesn't exist
        "2025-01-32",  # That day doesn't exist
        "today",
        "tomorrow",
    ],
)
def test_useradd__set_expire_date_with_invalid_date(shadow: Shadow, expiration_date: str):
    """
    :title: Create account with invalid expiration date
    :steps:
        1. Create user account with invalid account expiration date
        2. Check user exists
    :expectedresults:
        1. The process fails and the expiration date isn't changed
        2. User isn't found
    :customerscenario: False
    """
    with pytest.raises(ProcessError):
        shadow.useradd(f"tuser1 -e {expiration_date}")

    result = shadow.tools.getent.shadow("tuser1")
    assert result is None, "User shouldn't be found"


@pytest.mark.topology(KnownTopology.Shadow)
@pytest.mark.parametrize(
    "expiration_date",
    [
        "-1",
        "''",
    ],
)
def test_useradd__set_expire_date_with_empty_date(shadow: Shadow, expiration_date: str):
    """
    :title: Create account with empty expiration date
    :setup:
        1. Create user account with empty expiration date
    :steps:
        1. Check user exists and expiration date
    :expectedresults:
        1. User is found and expiration date is empty
    :customerscenario: False
    """
    shadow.useradd(f"tuser1 -e {expiration_date}")

    result = shadow.tools.getent.shadow("tuser1")
    assert result is not None, "User should be found"
    assert result.name == "tuser1", "Incorrect username"
    assert result.expiration_date is None, "Expiration date should be empty"


@pytest.mark.topology(KnownTopology.Shadow)
def test_useradd__additional_options(shadow: Shadow):
    """
    :title: Create user with comment, expiredate, inactive, shell and custom home
    :setup:
        1. Create user with multiple additional options
    :steps:
        1. Check passwd entry
        2. Check shadow entry
        3. Check home directory
    :expectedresults:
        1. Passwd attributes are correct
        2. Shadow expiration and inactive values are correct
        3. Home directory exists
    :customerscenario: False
    """

    shadow.useradd(
        "testuser "
        "--comment 'comment testuser' "
        "--expiredate 2006-02-04 "
        "--shell /bin/bash "
        "--inactive 12 "
        "--home-dir /nonexistenthomedir"
    )

    passwd_entry = shadow.tools.getent.passwd("testuser")
    assert passwd_entry is not None, "User should exist"
    assert passwd_entry.name == "testuser"
    assert passwd_entry.home == "/nonexistenthomedir"
    assert passwd_entry.shell == "/bin/bash"
    assert passwd_entry.gecos == "comment testuser"

    shadow_entry = shadow.tools.getent.shadow("testuser")
    assert shadow_entry is not None, "Shadow entry should exist"
    assert shadow_entry.name == "testuser"
    assert shadow_entry.expiration_date == 13183, f"Expected expiration_date 13183, got {shadow_entry.expiration_date}"
    assert shadow_entry.inactivity_days == 12, "Inactive days should be 12"
    assert shadow.fs.exists("/nonexistenthomedir"), "Home directory should exist"


@pytest.mark.topology(KnownTopology.Shadow)
def test_useradd__specified_uid(shadow: Shadow):
    """
    :title: Add a new user with a specific UID
    :setup:
        1. Create user with custom UID 4242
    :steps:
        1. Check passwd entry
        2. Check group entry
    :expectedresults:
        1. Passwd entry has UID 4242
        2. Group entry matching 4242
    :customerscenario: False
    """
    shadow.useradd("test1 -u 4242")

    passwd_entry = shadow.tools.getent.passwd("test1")
    assert passwd_entry is not None, "User test1 should be found in passwd"
    assert passwd_entry.name == "test1", "Incorrect username"
    assert passwd_entry.uid == 4242, f"Incorrect UID (expected 4242, got {passwd_entry.uid})"
    assert passwd_entry.gid == 4242, f"Incorrect GID (expected 4242, got {passwd_entry.gid})"

    group_entry = shadow.tools.getent.group("test1")
    assert group_entry is not None, "Group test1 should be found"
    assert group_entry.name == "test1", "Incorrect group name"
    assert group_entry.gid == 4242, f"Incorrect GID (expected 4242, got {group_entry.gid})"


@pytest.mark.topology(KnownTopology.Shadow)
def test_useradd__two_users_same_ids(shadow: Shadow):
    """
    :title: Add two users with same UID and GID
    :setup:
        1. Create first user with UID 4242
        2. Create second user with same UID and GID using -o flag
    :steps:
        1. Check passwd entries for both users
        2. Check group entries for both groups
    :expectedresults:
        1. Both users exist in passwd with UID 4242
        2. First group should have entry with GID 4242, while second group shouldn't exist
    :customerscenario: False
    """
    shadow.useradd("test1 -u 4242")

    shadow.useradd("test2 -u 4242 -g 4242 -o")

    passwd_entry1 = shadow.tools.getent.passwd("test1")
    assert passwd_entry1 is not None, "User test1 should be found in passwd"
    assert passwd_entry1.name == "test1", "Incorrect username for test1"
    assert passwd_entry1.uid == 4242, "test1 should have UID 4242"
    assert passwd_entry1.gid == 4242, "test1 should have GID 4242"

    passwd_entry2 = shadow.tools.getent.passwd("test2")
    assert passwd_entry2 is not None, "User test2 should be found in passwd"
    assert passwd_entry2.name == "test2", "Incorrect username for test2"
    assert passwd_entry2.uid == 4242, "test2 should have UID 4242"
    assert passwd_entry2.gid == 4242, "test2 should have GID 4242"

    group_entry1 = shadow.tools.getent.group("test1")
    assert group_entry1 is not None, "Group test1 should be found"
    assert group_entry1.name == "test1", "Incorrect group name for test1"
    assert group_entry1.gid == 4242, "test1 group should have GID 4242"

    group_entry2 = shadow.tools.getent.group("test2")
    assert group_entry2 is None, "Group test2 should not be found"


@pytest.mark.topology(KnownTopology.Shadow)
def test_useradd__add_user_with_existing_uid(shadow: Shadow):
    """
    :title: Verify useradd fails when adding user with existing UID without -o flag
    :setup:
        1. Create first user with UID 4242
    :steps:
        1. Check passwd entry for first user
        2. Try to create second user with same UID 4242 without using -o flag
    :expectedresults:
        1. First user exists in passwd with UID 4242
        2. Second user creation fails with error
    :customerscenario: False
    """
    shadow.useradd("test1 -u 4242")

    passwd_entry = shadow.tools.getent.passwd("test1")
    assert passwd_entry is not None, "passwd_entry is None"
    assert passwd_entry.uid == 4242, "First user should still have UID 4242"

    with pytest.raises(ProcessError) as exc_info:
        shadow.useradd("test2 -u 4242")

    assert exc_info.value.rc == 4, f"Expected return code 4 (UID already in use), got {exc_info.value.rc}"


@pytest.mark.topology(KnownTopology.Shadow)
@pytest.mark.parametrize(
    "group_identifier",
    [
        pytest.param(4242, id="invalid_numeric_gid"),
        pytest.param("nekral", id="invalid_named_group"),
    ],
)
def test_useradd__invalid_primary_group(shadow: Shadow, group_identifier: int | str):
    """
    :title: Add a new user with a specified non-existing primary group
    :steps:
        1. Attempt to create user with -g {group_identifier} option
        2. Verify command fails with appropriate error code and message
        3. Check passwd and group entries
        4. Check home directory
    :expectedresults:
        1. Useradd command fails
        2. Return code is 6 (specified group doesn't exist) and error message indicates group doesn't exist
        3. No entries are added to passwd and group
        4. No home directory is created
    :customerscenario: False
    """
    with pytest.raises(ProcessError) as exc_info:
        shadow.useradd(f"test1 -g {group_identifier}")

    actual_rc = getattr(exc_info.value, "rc", getattr(exc_info.value, "returncode", None))
    assert actual_rc == 6, f"Expected return code 6 (specified group doesn't exist), got {actual_rc}"

    error_output = exc_info.value.stderr.lower() if exc_info.value.stderr else ""
    assert (
        "group" in error_output and "does not exist" in error_output
    ), f"Error message should indicate group doesn't exist. Got: {error_output}"

    assert shadow.tools.getent.passwd("test1") is None, "User test1 should not be found in passwd"
    assert shadow.tools.getent.group("test1") is None, "Group test1 should not be found"
    assert not shadow.fs.exists("/home/test1"), "Home directory should not be created"


@pytest.mark.topology(KnownTopology.Shadow)
@pytest.mark.parametrize(
    "group_name, group_gid",
    [
        pytest.param("testgroup", None, id="valid_named_group"),
        pytest.param("testgroup5000", 5000, id="valid_numeric_gid"),
    ],
)
def test_useradd__valid_group_as_primary(shadow: Shadow, group_name: str, group_gid: int | None):
    """
    :title: Add a new user with a valid existing group as primary
    :steps:
        1. Create a group
        2. Check group entry
        3. Create user with that group as primary
        4. Check passwd entry
    :expectedresults:
        1. Group created successfully
        2. Group attributes are correct
        3. User created with the specified group as primary
        4. User's GID matches the group's GID
    :customerscenario: False
    """
    if group_gid:
        shadow.groupadd(f"{group_name} -g {group_gid}")
    else:
        shadow.groupadd(group_name)

    group_entry = shadow.tools.getent.group(group_name)
    assert group_entry is not None, f"Group {group_name} should exist"
    assert group_entry.name == group_name, f"Incorrect group name, expected '{group_name}', got '{group_entry.name}'"

    if group_gid:
        assert group_entry.gid == group_gid, f"Incorrect GID, expected {group_gid}, got {group_entry.gid}"

    if group_gid:
        shadow.useradd(f"testuser -g {group_gid}")
    else:
        shadow.useradd(f"testuser -g {group_name}")

    passwd_entry = shadow.tools.getent.passwd("testuser")
    assert passwd_entry is not None, "User testuser should exist"
    assert (
        passwd_entry.gid == group_entry.gid
    ), f"User's GID ({passwd_entry.gid}) should match group's GID ({group_entry.gid})"


@pytest.mark.topology(KnownTopology.Shadow)
@pytest.mark.parametrize(
    "uid_value, expected_error",
    [
        pytest.param(-1, "useradd: invalid user ID '-1'", id="negative_uid"),
        pytest.param(4294967295, "useradd: invalid user ID '4294967295'", id="exceeds_maximum_uid"),
        pytest.param(4294967296, "useradd: invalid user ID '4294967296'", id="out_of_range_uid"),
    ],
)
def test_useradd__invalid_uid(shadow: Shadow, uid_value: int, expected_error: str):
    """
    :title: Add a new user with an invalid UID
    :steps:
        1. Attempt to create user with invalid UID
        2. Verify command fails with appropriate error code and message
        3. Check passwd, group entries
        4. Check home directory
    :expectedresults:
        1. useradd command fails
        2. Return code is 3 (invalid argument)
        3. No entries are added to passwd, group
        4. No home directory is created
    :customerscenario: False
    """
    with pytest.raises(ProcessError) as exc_info:
        shadow.useradd(f"test1 -u {uid_value}")

    actual_rc = getattr(exc_info.value, "rc", getattr(exc_info.value, "returncode", None))
    assert actual_rc == 3, f"Expected return code 3 (invalid argument), got {actual_rc}"

    error_output = exc_info.value.stderr.strip() if exc_info.value.stderr else ""
    assert error_output == expected_error, f"Expected error message '{expected_error}', got '{error_output}'"

    assert shadow.tools.getent.passwd("test1") is None, "User test1 should not be found in passwd"
    assert shadow.tools.getent.group("test1") is None, "Group test1 should not be found"
    assert not shadow.fs.exists("/home/test1"), "Home directory should not be created"


@pytest.mark.topology(KnownTopology.Shadow)
@pytest.mark.parametrize(
    "uid_value",
    [
        pytest.param(2147483647, id="maximum_signed_32bit_uid"),
        pytest.param(4294967294, id="maximum_unsigned_32bit_uid"),
    ],
)
def test_useradd__successful_large_uid(shadow: Shadow, uid_value: int):
    """
    :title: Verify user creation succeeds with large valid UIDs
    :steps:
        1. Create user with UID {uid_value}
        2. Check passwd entry
        3. Check group entry
    :expectedresults:
        1. User is created successfully
        2. Passwd entry exists with correct UID {uid_value}
        3. Group entry exists
    :customerscenario: False
    """
    shadow.useradd(f"test1 -u {uid_value}")

    passwd_entry = shadow.tools.getent.passwd("test1")
    assert passwd_entry is not None, "User test1 should be found in passwd"
    assert passwd_entry.name == "test1", "Incorrect username"
    assert passwd_entry.uid == uid_value, f"Incorrect UID, expected {uid_value}, got {passwd_entry.uid}"

    group_entry = shadow.tools.getent.group("test1")
    assert group_entry is not None, "Group test1 should be found"
    assert group_entry.name == "test1", "Incorrect group name"


@pytest.mark.topology(KnownTopology.Shadow)
def test_useradd_custom_primary_and_supplementary_groups(shadow: Shadow):
    """
    :title: Add a new user with custom primary group and supplementary groups
    :steps:
        1. Create required groups
        2. Create user with primary group and supplementary groups
        3. Check passwd entry
        4. Verify primary group
        5. Verify user is a member of all supplementary groups
    :expectedresults:
        1. Groups are created successfully
        2. User is created successfully
        3. Passwd entry exists with correct attributes
        4. User has primary group
        5. User is added to supplementary groups as a member
    :customerscenario: False
    """
    username = "testuser1"
    groups_to_create = ["testgroup", "testgroup1", "testgroup2", "testgroup3"]
    for group in groups_to_create:
        shadow.groupadd(group)

    shadow.useradd(f"{username} -g testgroup -G testgroup1,testgroup2,testgroup3")

    passwd_entry = shadow.tools.getent.passwd(username)
    assert passwd_entry is not None, f"User {username} should be found in passwd"
    assert passwd_entry.name == username, "Incorrect username"
    assert passwd_entry.home == f"/home/{username}", "Incorrect home directory"

    testgroup_entry = shadow.tools.getent.group("testgroup")
    assert testgroup_entry is not None, "Group testgroup should exist"
    assert (
        passwd_entry.gid == testgroup_entry.gid
    ), f"User's primary GID ({passwd_entry.gid}) should match testgroup GID ({testgroup_entry.gid})"

    supplementary_groups = ["testgroup1", "testgroup2", "testgroup3"]
    for group_name in supplementary_groups:
        group_entry = shadow.tools.getent.group(group_name)
        assert group_entry is not None, f"Group {group_name} should exist"
        assert username in group_entry.members, f"User {username} should be a member of {group_name} group"
