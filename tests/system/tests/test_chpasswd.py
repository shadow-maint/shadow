"""
Test chpasswd
"""

from __future__ import annotations

import re

import pytest

from framework.misc import days_since_epoch, shadow_password_pattern
from framework.roles.shadow import Shadow
from framework.topology import KnownTopology


@pytest.mark.topology(KnownTopology.Shadow)
def test_chpasswd__change_passwords_interactive(shadow: Shadow):
    """
    :title: Change multiple user passwords interactive
    :setup:
        1. Create test users
    :steps:
        1. Change user passwords using chpasswd interactively
        2. Check the users' shadow entries
    :expectedresults:
        1. Users' passwords are changed
        2. Users' shadow entries are correct
    :customerscenario: False
    """
    shadow.useradd("tuser1")
    shadow.useradd("tuser2")

    passwords_data = "tuser1:Secret123\ntuser2:Secret456"
    shadow.chpasswd(passwords_data=passwords_data)

    shadow_entry1 = shadow.tools.getent.shadow("tuser1")
    assert shadow_entry1 is not None, "tuser1 should be found"
    assert shadow_entry1.name == "tuser1", "Incorrect username"
    assert shadow_entry1.password is not None, "Password should not be None"
    assert re.match(shadow_password_pattern(), shadow_entry1.password), "Incorrect password"
    assert shadow_entry1.last_changed == days_since_epoch(), "Incorrect last changed"
    assert shadow_entry1.min_days == 0, "Incorrect min days"
    assert shadow_entry1.max_days == 99999, "Incorrect max days"
    assert shadow_entry1.warn_days == 7, "Incorrect warn days"

    shadow_entry2 = shadow.tools.getent.shadow("tuser2")
    assert shadow_entry2 is not None, "tuser2 should be found"
    assert shadow_entry2.name == "tuser2", "Incorrect username"
    assert shadow_entry2.password is not None, "Password should not be None"
    assert re.match(shadow_password_pattern(), shadow_entry2.password), "Incorrect password"
    assert shadow_entry2.last_changed == days_since_epoch(), "Incorrect last changed"
    assert shadow_entry2.min_days == 0, "Incorrect min days"
    assert shadow_entry2.max_days == 99999, "Incorrect max days"
    assert shadow_entry2.warn_days == 7, "Incorrect warn days"
