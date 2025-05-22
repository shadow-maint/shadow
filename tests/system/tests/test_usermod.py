"""
Test usermod
"""

from __future__ import annotations

import pytest

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
