"""
Test chgpasswd
"""

from __future__ import annotations

import re

import pytest

from framework.misc import shadow_password_pattern
from framework.roles.shadow import Shadow
from framework.topology import KnownTopology


@pytest.mark.topology(KnownTopology.Shadow)
@pytest.mark.builtwith(shadow="gshadow")
def test_chgpasswd__change_passwords_interactive(shadow: Shadow):
    """
    :title: Change multiple group passwords interactive
    :setup:
        1. Create test groups
    :steps:
        1. Change group passwords using chgpasswd interactively
        2. Check the groups' gshadow entries
    :expectedresults:
        1. Groups' passwords are changed
        2. Groups' gshadow entries are correct
    :customerscenario: False
    """
    shadow.groupadd("tgroup1")
    shadow.groupadd("tgroup2")

    passwords_data = "tgroup1:Secret123\ntgroup2:Secret456"
    shadow.chgpasswd(passwords_data=passwords_data)

    gshadow_entry1 = shadow.tools.getent.gshadow("tgroup1")
    assert gshadow_entry1 is not None, "tgroup1 should be found"
    assert gshadow_entry1.name == "tgroup1", "Incorrect groupname"
    assert gshadow_entry1.password is not None, "Password should not be None"
    assert re.match(shadow_password_pattern(), gshadow_entry1.password), "Incorrect password"

    gshadow_entry2 = shadow.tools.getent.gshadow("tgroup2")
    assert gshadow_entry2 is not None, "tgroup2 should be found"
    assert gshadow_entry2.name == "tgroup2", "Incorrect groupname"
    assert gshadow_entry2.password is not None, "Password should not be None"
    assert re.match(shadow_password_pattern(), gshadow_entry2.password), "Incorrect password"


@pytest.mark.topology(KnownTopology.Shadow)
@pytest.mark.builtwith(shadow="gshadow")
def test_chgpasswd__change_passwords_from_file(shadow: Shadow):
    """
    :title: Change multiple group passwords from file
    :setup:
        1. Create test groups and password file
    :steps:
        1. Change group passwords using chgpasswd from file
        2. Check the groups' gshadow entries
    :expectedresults:
        1. Groups' passwords are changed
        2. Groups' gshadow entries are correct
    :customerscenario: False
    """
    shadow.groupadd("tgroup1")
    shadow.groupadd("tgroup2")

    temp_file = "/tmp/test_chgpasswd_data.txt"
    passwords_data = "tgroup1:Secret123\ntgroup2:Secret456\n"
    shadow.fs.write(temp_file, passwords_data, dedent=False)
    shadow.chgpasswd(file=temp_file)

    gshadow_entry1 = shadow.tools.getent.gshadow("tgroup1")
    assert gshadow_entry1 is not None, "tgroup1 should be found"
    assert gshadow_entry1.name == "tgroup1", "Incorrect groupname"
    assert gshadow_entry1.password is not None, "Password should not be None"
    assert re.match(shadow_password_pattern(), gshadow_entry1.password), "Incorrect password"

    gshadow_entry2 = shadow.tools.getent.gshadow("tgroup2")
    assert gshadow_entry2 is not None, "tgroup2 should be found"
    assert gshadow_entry2.name == "tgroup2", "Incorrect groupname"
    assert gshadow_entry2.password is not None, "Password should not be None"
    assert re.match(shadow_password_pattern(), gshadow_entry2.password), "Incorrect password"
