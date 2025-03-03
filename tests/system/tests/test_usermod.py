"""
Test usermod
"""

from __future__ import annotations

import pytest

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
        1. User exists with new name and GID is 1000
        2. Group exists and GID is 1000
        3. Home folder exists
    :expectedresults:
        1. User is found and UID matches
        2. Group is found and GID matches
        3. Home folder is found
    :customerscenario: False
    """
    shadow.useradd("tuser1")
    shadow.usermod("-l tuser2 tuser1")

    result = shadow.tools.id("tuser2")
    assert result is not None, "User should be found"
    assert result.user.name == "tuser2", "Incorrect username"
    assert result.user.id == 1000, "Incorrect UID"

    result = shadow.tools.getent.group("tuser1")
    assert result is not None, "Group should be found"
    assert result.name == "tuser1", "Incorrect groupname"
    assert result.gid == 1000, "Incorrect GID"

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
def test_usermod__expire_user_correct_date(shadow: Shadow, expiration_date: str, expected_date: int | None):
    """
    :title: Set account expiration date correctly
    :setup:
        1. Create user
    :steps:
        1. Set correct account expiration date
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
        "-1",  # Dates can't be in negative numbers
        "2025-18-18",  # That month and day don't exist
        "1969-01-01",  # This is before 1970-01-01
        "2025-13-01",  # That month doesn't exist
        "2025-01-32",  # That day doesn't exist
        "today",
        "tomorrow",
    ],
)
def test_usermod__expire_user_incorrect_date(shadow: Shadow, expiration_date: str):
    """
    :title: Set account expiration date incorrectly
    :setup:
        1. Create user
    :steps:
        1. Set incorrect account expiration date
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
    assert result.expiration_date is None, "Expiration date shouldn't be set"
