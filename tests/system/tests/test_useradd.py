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

    result = shadow.tools.id("tuser")
    assert result is not None, "User should be found"
    assert result.user.name == "tuser", "Incorrect username"
    assert result.user.id == 1000, "Incorrect UID"

    result = shadow.tools.getent.group("tuser")
    assert result is not None, "Group should be found"
    assert result.name == "tuser", "Incorrect groupname"
    assert result.gid == 1000, "Incorrect GID"

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

    result = shadow.tools.id("tuser")
    assert result is not None, "User should be found"
    assert result.user.name == "tuser", "Incorrect username"
    assert result.user.id == 1000, "Incorrect UID"

    result = shadow.tools.getent.group("tuser")
    assert result is not None, "Group should be found"
    assert result.name == "tuser", "Incorrect groupname"
    assert result.gid == 1000, "Incorrect GID"

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
def test_useradd__expire_user_correct_date(shadow: Shadow, expiration_date: str, expected_date: int | None):
    """
    :title: Create account with correct expiration date
    :setup:
        1. Create user with a correct account expiration date
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
def test_useradd__expire_user_incorrect_date(shadow: Shadow, expiration_date: str):
    """
    :title: Create account with incorrect expiration date
    :steps:
        1. Create user with an correct account expiration date
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
