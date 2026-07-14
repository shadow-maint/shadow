"""
Test usermod
"""

from __future__ import annotations

import pytest
from passlib.hash import sha512_crypt
from pytest_mh.conn import ProcessError

from framework.roles.shadow import Shadow
from framework.topology import KnownTopology


@pytest.mark.topology(KnownTopology.Shadow)
def test_usermod__rename_user(shadow: Shadow):
    """
    :title: Rename user
    :setup:
        1. Create user
        2. Rename user
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
    shadow.useradd("tuser1")
    shadow.usermod("-l tuser2 tuser1")

    passwd_entry = shadow.tools.getent.passwd("tuser2")
    assert passwd_entry is not None, "User should be found"
    assert passwd_entry.name == "tuser2", "Incorrect username"
    assert passwd_entry.uid == 1001, "Incorrect UID"

    shadow_entry = shadow.tools.getent.shadow("tuser2")
    assert shadow_entry is not None, "User should be found"
    assert shadow_entry.name == "tuser2", "Incorrect username"

    group_entry = shadow.tools.getent.group("tuser1")
    assert group_entry is not None, "Group should be found"
    assert group_entry.name == "tuser1", "Incorrect groupname"
    assert group_entry.gid == 1001, "Incorrect GID"

    if shadow.host.features["gshadow"]:
        gshadow_entry = shadow.tools.getent.gshadow("tuser1")
        assert gshadow_entry is not None, "User should be found"
        assert gshadow_entry.name == "tuser1", "Incorrect username"

    assert shadow.fs.exists("/home/tuser1"), "Home folder should be found"


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
def test_usermod__set_expire_date_with_valid_date(shadow: Shadow, expiration_date: str, expected_date: int | None):
    """
    :title: Set valid account expiration date
    :setup:
        1. Create user
    :steps:
        1. Set valid account expiration date
        2. Check user exists and expiration date
    :expectedresults:
        1. The expiration date is correctly set
        2. User is found and expiration date is valid
    :customerscenario: False
    """
    shadow.useradd("tuser1")

    shadow.usermod(f"-e {expiration_date} tuser1")

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
def test_usermod__set_expire_date_with_invalid_date(shadow: Shadow, expiration_date: str):
    """
    :title: Set invalid account expiration date
    :setup:
        1. Create user
    :steps:
        1. Set invalid account expiration date
        2. Check user exists and expiration date
    :expectedresults:
        1. The process fails and the expiration date isn't changed
        2. User is found and expiration date is empty
    :customerscenario: False
    """
    shadow.useradd("tuser1")

    with pytest.raises(ProcessError):
        shadow.usermod(f"-e {expiration_date} tuser1")

    result = shadow.tools.getent.shadow("tuser1")
    assert result is not None, "User should be found"
    assert result.name == "tuser1", "Incorrect username"
    assert result.expiration_date is None, "Expiration date should be empty"


@pytest.mark.topology(KnownTopology.Shadow)
@pytest.mark.parametrize(
    "expiration_date",
    [
        "-1",
        "''",
    ],
)
def test_usermod__set_expire_date_with_empty_date(shadow: Shadow, expiration_date: str):
    """
    :title: Set empty account expiration date
    :setup:
        1. Create user
    :steps:
        1. Set account expiration date
        2. Check user exists and expiration date
        3. Empty account expiration date
        4. Check user exists and expiration date
    :expectedresults:
        1. The expiration date is correctly set
        2. User is found and expiration date is valid
        3. The expiration date is correctly emptied
        4. User is found and expiration date is empty
    :customerscenario: False
    """
    shadow.useradd("tuser1")

    shadow.usermod("-e 10 tuser1")

    result = shadow.tools.getent.shadow("tuser1")
    assert result is not None, "User should be found"
    assert result.name == "tuser1", "Incorrect username"
    assert result.expiration_date == 10, "Incorrect expiration date"

    shadow.usermod(f"-e {expiration_date} tuser1")

    result = shadow.tools.getent.shadow("tuser1")
    assert result is not None, "User should be found"
    assert result.name == "tuser1", "Incorrect username"
    assert result.expiration_date is None, "Expiration date should be empty"


@pytest.mark.topology(KnownTopology.Shadow)
def test_usermod__rename_user_in_group(shadow: Shadow):
    """
    :title: Rename user who is member of a group
    :setup:
        1. Create group
        2. Create user with additional group membership
        3. Rename user
    :steps:
        1. Check passwd entry
        2. Check group entry for user's primary group
        3. Check group entry for secondary group membership
    :expectedresults:
        1. Passwd entry for renamed user exists
        2. User's primary group still exists with old name
        3. User is member of group with new name
    :customerscenario: False
    """
    shadow.groupadd("tgroup")
    shadow.useradd("-G tgroup tuser1")
    shadow.usermod("-l tuser2 tuser1")

    passwd_entry = shadow.tools.getent.passwd("tuser2")
    assert passwd_entry is not None, "User should be found"
    assert passwd_entry.name == "tuser2", "Incorrect username"

    group_entry = shadow.tools.getent.group("tuser1")
    assert group_entry is not None, "Primary group should still exist"
    assert group_entry.name == "tuser1", "Primary group should keep old name"

    tgroup_group = shadow.tools.getent.group("tgroup")
    assert tgroup_group is not None, "tgroup group should exist"
    assert "tuser2" in tgroup_group.members, "User should be in tgroup group with new name"


@pytest.mark.topology(KnownTopology.Shadow)
def test_usermod__change_password(shadow: Shadow):
    """
    :title: Change user password
    :setup:
        1. Create user
        2. Change password using usermod -p
    :steps:
        1. Check shadow entry has new password
    :expectedresults:
        1. Password is updated in shadow file
    :customerscenario: False
    """
    shadow.useradd("tuser1")

    password = "Secret123"
    password_hash = sha512_crypt.hash(password)
    shadow.usermod(f"-p '{password_hash}' tuser1")

    shadow_entry = shadow.tools.getent.shadow("tuser1")
    assert shadow_entry is not None, "User should be found"
    assert shadow_entry.password is not None, "Password should not be None"
    assert shadow_entry.password.startswith("$6$"), "Password should be SHA-512 crypt hash"
    assert shadow_entry.password == password_hash, "Password hash should match the generated hash"


@pytest.mark.topology(KnownTopology.Shadow)
def test_usermod__unlock_password(shadow: Shadow):
    """
    :title: Unlock user password
    :setup:
        1. Create user
        2. Set password
    :steps:
        1. Lock password with usermod -L
        2. Check shadow entry has locked password
        3. Unlock password with usermod -U
        4. Check shadow entry has unlocked password
    :expectedresults:
        1. Password is locked
        2. Password has ! prefix when locked
        3. Password is unlocked
        4. Password no longer has ! prefix
    :customerscenario: False
    """
    shadow.useradd("tuser1")

    password = "Secret123"
    password_hash = sha512_crypt.hash(password)

    shadow.usermod(f"-p '{password_hash}' tuser1")
    shadow.usermod("-L tuser1")

    shadow_entry = shadow.tools.getent.shadow("tuser1")
    assert shadow_entry is not None, "User should be found"
    assert shadow_entry.password is not None, "Password should not be None"
    assert shadow_entry.password.startswith("!"), "Password should be locked with ! prefix"
    assert shadow_entry.password == f"!{password_hash}", "Password should have ! prefix when locked"

    shadow.usermod("-U tuser1")

    shadow_entry = shadow.tools.getent.shadow("tuser1")
    assert shadow_entry is not None, "User should be found"
    assert shadow_entry.password is not None, "Password should not be None"
    assert shadow_entry.password == password_hash, "Password should be unlocked"


@pytest.mark.topology(KnownTopology.Shadow)
def test_usermod__modify_multiple_attributes(shadow: Shadow):
    """
    :title: Change multiple user attributes at once
    :setup:
        1. Create test group
        2. Create user
        3. Change GID, comment, expiration, inactive, shell, and home
    :steps:
        1. Check passwd entry
        2. Check shadow entry
    :expectedresults:
        1. Passwd entry for the user exists and the attributes are correct
        2. Shadow entry for the user exists and the attributes are correct
    :customerscenario: False
    """
    shadow.groupadd("tgroup -g 9999")
    shadow.useradd("tuser1")
    shadow.usermod("-g 9999 --comment 'Test Comment' -e 2000-09-01 -f 17 -s /bin/bash -d /tmp tuser1")

    passwd_entry = shadow.tools.getent.passwd("tuser1")
    assert passwd_entry is not None, "User should be found"
    assert passwd_entry.gid == 9999, "GID should be 9999"
    assert passwd_entry.gecos == "Test Comment", "Comment should be updated"
    assert passwd_entry.shell == "/bin/bash", "Shell should be /bin/bash"
    assert passwd_entry.home == "/tmp", "Home should be /tmp"

    shadow_entry = shadow.tools.getent.shadow("tuser1")
    assert shadow_entry is not None, "User should be found"
    assert shadow_entry.expiration_date == 11201, "Expiration should be 2000-09-01 (11201 days since epoch)"
    assert shadow_entry.inactivity_days == 17, "Inactive days should be 17"


@pytest.mark.topology(KnownTopology.Shadow)
def test_usermod__remove_user_from_existing_supplementary_group(shadow: Shadow):
    """
    :title: Remove user from existing supplementary group
    :setup:
        1. Create test user and groups
        2. Add user to supplementary groups
    :steps:
        1. Remove user from one supplementary group
        2. Check group and gshadow entries to verify user is removed from the first group
        3. Check group and gshadow entries to verify user remains in the second group
    :expectedresults:
        1. User is successfully removed from specified group
        2. group and gshadow entries don't contain user in first group
        3. group and gshadow entries contains user in second group
    :customerscenario: False
    """
    shadow.useradd("tuser1")
    shadow.groupadd("tgroup1")
    shadow.groupadd("tgroup2")
    shadow.usermod("-a -G tgroup1,tgroup2 tuser1")

    shadow.usermod("-rG tgroup1 tuser1")

    tgroup1_passwd = shadow.tools.getent.group("tgroup1")
    assert tgroup1_passwd is not None, "Group should be found"
    assert "tuser1" not in tgroup1_passwd.members, "tuser1 should be removed from tgroup1"

    if shadow.host.features["gshadow"]:
        tgroup1_gshadow = shadow.tools.getent.gshadow("tgroup1")
        assert tgroup1_gshadow is not None, "Group should be found"
        assert "tuser1" not in tgroup1_gshadow.members, "tuser1 should be removed from tgroup1"

    tgroup2_passwd = shadow.tools.getent.group("tgroup2")
    assert tgroup2_passwd is not None, "Group should be found"
    assert "tuser1" in tgroup2_passwd.members, "tuser1 should remain in tgroup2"

    if shadow.host.features["gshadow"]:
        tgroup2_gshadow = shadow.tools.getent.gshadow("tgroup2")
        assert tgroup2_gshadow is not None, "Group should be found"
        assert "tuser1" in tgroup2_gshadow.members, "tuser1 should remain in tgroup2"
