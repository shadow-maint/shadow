"""
Test useradd
"""

from __future__ import annotations

import pytest

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
