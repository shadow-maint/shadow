"""
Test passwd
"""

from __future__ import annotations

import re

import pytest

from framework.misc import days_since_epoch, shadow_password_pattern
from framework.roles.shadow import Shadow
from framework.topology import KnownTopology


@pytest.mark.topology(KnownTopology.Shadow)
def test_passwd__change_password_as_root_with_stdin(shadow: Shadow):
    """
    :title: Change user password as root using stdin
    :setup:
        1. Create test user
    :steps:
        1. Change user password as root using --stdin flag
        2. Check the user's shadow entry
    :expectedresults:
        1. User's password is changed
        2. User's shadow entry is correct
    :customerscenario: False
    """
    shadow.useradd("tuser")

    shadow.passwd("--stdin tuser", new_password="Secret123")

    shadow_entry = shadow.tools.getent.shadow("tuser")
    assert shadow_entry is not None, "User should be found"
    assert shadow_entry.name == "tuser", "Incorrect username"
    assert shadow_entry.password is not None, "Password should not be None"
    assert re.match(shadow_password_pattern(), shadow_entry.password), "Incorrect password"
    assert shadow_entry.last_changed == days_since_epoch(), "Incorrect last changed"
    assert shadow_entry.min_days == 0, "Incorrect min days"
    assert shadow_entry.max_days == 99999, "Incorrect max days"
    assert shadow_entry.warn_days == 7, "Incorrect warn days"


@pytest.mark.topology(KnownTopology.Shadow)
def test_passwd__change_password_as_root_interactive(shadow: Shadow):
    """
    :title: Change user password as root in interactive mode
    :setup:
        1. Create test user
    :steps:
        1. Change user password as root in interactive mode
        2. Check the user's shadow entry
    :expectedresults:
        1. User's password is changed
        2. User's shadow entry is correct
    :customerscenario: False
    """
    shadow.useradd("tuser")

    shadow.passwd("tuser", new_password="Secret123")

    shadow_entry = shadow.tools.getent.shadow("tuser")
    assert shadow_entry is not None, "User should be found"
    assert shadow_entry.name == "tuser", "Incorrect username"
    assert shadow_entry.password is not None, "Password should not be None"
    assert re.match(shadow_password_pattern(), shadow_entry.password), "Incorrect password"
    assert shadow_entry.last_changed == days_since_epoch(), "Incorrect last changed"
    assert shadow_entry.min_days == 0, "Incorrect min days"
    assert shadow_entry.max_days == 99999, "Incorrect max days"
    assert shadow_entry.warn_days == 7, "Incorrect warn days"
