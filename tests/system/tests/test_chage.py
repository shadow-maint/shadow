"""
Test chage
"""

from __future__ import annotations

import pytest

from pytest_mh.conn import ProcessError
from framework.misc import days_since_epoch
from framework.roles.shadow import Shadow
from framework.topology import KnownTopology


@pytest.mark.topology(KnownTopology.Shadow)
@pytest.mark.parametrize(
    "last_changed_date, expected_date",
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
def test_chage__set_last_changed_with_valid_date(shadow: Shadow, last_changed_date: str, expected_date: int | None):
    """
    :title: Set valid last changed date
    :setup:
        1. Create user
    :steps:
        1. Set valid account last changed date
        2. Check user exists and last changed date
    :expectedresults:
        1. The last changed date is correctly set
        2. User is found and last changed date is valid
    :customerscenario: False
    """
    shadow.useradd("tuser1")

    shadow.chage(f"-d {last_changed_date} tuser1")

    result = shadow.tools.getent.shadow("tuser1")
    assert result is not None, "User should be found"
    assert result.name == "tuser1", "Incorrect username"
    assert result.last_changed == expected_date, "Incorrect expiration date"


@pytest.mark.topology(KnownTopology.Shadow)
@pytest.mark.parametrize(
    "last_changed_date",
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
def test_chage__set_last_changed_with_invalid_date(shadow: Shadow, last_changed_date: str):
    """
    :title: Set invalid last changed date
    :setup:
        1. Create user
    :steps:
        1. Set invalid last changed date
        2. Check user exists and last changed date
    :expectedresults:
        1. The process fails and the last changed date isn't changed
        2. User is found and last changed is not changed
    :customerscenario: False
    """
    shadow.useradd("tuser1")

    with pytest.raises(ProcessError):
        shadow.chage(f"-d {last_changed_date} tuser1")

    result = shadow.tools.getent.shadow("tuser1")
    assert result is not None, "User should be found"
    assert result.name == "tuser1", "Incorrect username"
    assert result.last_changed == days_since_epoch(), "Expiration date should not be changed"


@pytest.mark.topology(KnownTopology.Shadow)
@pytest.mark.parametrize(
    "last_changed",
    [
        "-1",
        "''",
    ],
)
def test_chage__set_last_changed_with_empty_date(shadow: Shadow, last_changed: str):
    """
    :title: Set empty last changed date
    :setup:
        1. Create user
    :steps:
        1. Set account last changed date
        2. Check user exists and last changed date
        3. Empty account last changed date
        4. Check user exists and last changed date
    :expectedresults:
        1. The last changed date is correctly set
        2. User is found and last changed date is valid
        3. The last changed date is correctly emptied
        4. User is found and last changed date is empty
    :customerscenario: False
    """
    shadow.useradd("tuser1")

    shadow.chage("-d 10 tuser1")

    result = shadow.tools.getent.shadow("tuser1")
    assert result is not None, "User should be found"
    assert result.name == "tuser1", "Incorrect username"
    assert result.last_changed == 10, "Incorrect expiration date"

    shadow.chage(f"-d {last_changed} tuser1")

    result = shadow.tools.getent.shadow("tuser1")
    assert result is not None, "User should be found"
    assert result.name == "tuser1", "Incorrect username"
    assert result.last_changed is None, "Incorrect expiration date"


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
def test_chage__set_expire_date_with_valid_date(shadow: Shadow, expiration_date: str, expected_date: int | None):
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

    shadow.chage(f"-E {expiration_date} tuser1")

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
def test_chage__set_expire_date_with_invalid_date(shadow: Shadow, expiration_date: str):
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
        shadow.chage(f"-E {expiration_date} tuser1")

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
def test_chage__set_expire_date_with_empty_date(shadow: Shadow, expiration_date: str):
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

    shadow.chage("-E 10 tuser1")

    result = shadow.tools.getent.shadow("tuser1")
    assert result is not None, "User should be found"
    assert result.name == "tuser1", "Incorrect username"
    assert result.expiration_date == 10, "Incorrect expiration date"

    shadow.chage(f"-E {expiration_date} tuser1")

    result = shadow.tools.getent.shadow("tuser1")
    assert result is not None, "User should be found"
    assert result.name == "tuser1", "Incorrect username"
    assert result.expiration_date is None, "Expiration date should be empty"
