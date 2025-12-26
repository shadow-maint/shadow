"""
Test useradd
"""

from __future__ import annotations

import pytest
from pytest_mh.conn import ProcessError

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
    assert shadow_entry.last_changed is None, "Incorrect last changed"
    assert shadow_entry.min_days is None, "Incorrect min days"
    assert shadow_entry.max_days is None, "Incorrect max days"
    assert shadow_entry.warn_days is None, "Incorrect warn days"

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
