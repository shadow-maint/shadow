"""
Test userdel
"""

from __future__ import annotations

import pytest

from framework.roles.shadow import Shadow
from framework.topology import KnownTopology


@pytest.mark.topology(KnownTopology.Shadow)
def test_userdel__homedir_removed(shadow: Shadow):
    """
    :title: Delete user and homedir
    :setup:
        1. Create user
        2. Delete the user and the homedir
    :steps:
        1. User doesn't exist
        2. Group doesn't exist
        3. Home folder doesn't exist
    :expectedresults:
        1. User is not found
        2. Group is not found
        3. Home folder is not found
    :customerscenario: False
    """
    shadow.useradd("tuser")
    shadow.userdel("-r tuser")

    result = shadow.tools.id("tuser")
    assert result is None, "User should not be found"

    result = shadow.tools.getent.group("tuser")
    assert result is None, "Group should not be found"

    assert not shadow.fs.exists("/home/tuser"), "Home folder should not exist"
