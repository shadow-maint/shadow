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

    result = shadow.tools.getent.passwd("tuser2")
    assert result is not None, "User should be found"
    assert result.name == "tuser2", "Incorrect username"
    assert result.uid == 1000, "Incorrect UID"

    result = shadow.tools.getent.shadow("tuser2")
    assert result is not None, "User should be found"
    assert result.name == "tuser2", "Incorrect username"

    result = shadow.tools.getent.group("tuser1")
    assert result is not None, "Group should be found"
    assert result.name == "tuser1", "Incorrect groupname"
    assert result.gid == 1000, "Incorrect GID"

    if shadow.host.features["gshadow"]:
        result = shadow.tools.getent.gshadow("tuser1")
        assert result is not None, "User should be found"
        assert result.name == "tuser1", "Incorrect username"

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
