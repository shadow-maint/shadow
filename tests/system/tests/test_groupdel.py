"""
Test groupdel
"""

from __future__ import annotations

import pytest
from pytest_mh.conn import ProcessError

from framework.roles.shadow import Shadow
from framework.topology import KnownTopology


@pytest.mark.topology(KnownTopology.Shadow)
def test_groupdel__delete_group(shadow: Shadow):
    """
    :title: Basic group deletion
    :setup:
        1. Create group
        2. Delete group
    :steps:
        1. Check group entry
        2. Check gshadow entry
    :expectedresults:
        1. group entry for the user doesn't exist
        2. gshadow entry for the user doesn't exist
    :customerscenario: False
    """
    shadow.groupadd("tgroup")
    shadow.groupdel("tgroup")

    group_entry = shadow.tools.getent.group("tgroup")
    assert group_entry is None, "Group should not be found"

    gshadow_entry = shadow.tools.getent.gshadow("tgroup")
    assert gshadow_entry is None, "Group should not be found"


@pytest.mark.topology(KnownTopology.Shadow)
def test_groupdel__delete_group_no_gshadow_entry(shadow: Shadow):
    """
    :title: Group deletion with no corresponding gshadow entry
    :setup:
        1. Create group
        2. Remove gshadow entry manually
    :steps:
        1. Delete group
        2. Check group entry
        3. Check gshadow entry
    :expectedresults:
        1. Group is deleted successfully
        2. No group entry is found
        3. No gshadow entry is found
    :customerscenario: False
    """
    shadow.groupadd("tgroup")

    shadow.fs.sed("/^tgroup:/d", "/etc/gshadow")

    shadow.groupdel("tgroup")

    group_entry = shadow.tools.getent.group("tgroup")
    assert group_entry is None, "Group should not be found"

    if shadow.host.features["gshadow"]:
        gshadow_entry = shadow.tools.getent.gshadow("tgroup")
        assert gshadow_entry is None, "Group should not be found"


@pytest.mark.topology(KnownTopology.Shadow)
def test_groupdel__delete_group_no_gshadow_file(shadow: Shadow):
    """
    :title: Group deletion with no gshadow file
    :setup:
        1. Create group
        2. Remove gshadow file
    :steps:
        1. Delete group
        2. Check group entry
        3. Verify that gshadow file doesn't exist
    :expectedresults:
        1. Group is deleted successfully
        2. No group entry is found
        3. No gshadow file is found
    :customerscenario: False
    """
    shadow.groupadd("tgroup")

    shadow.fs.rm("/etc/gshadow")

    shadow.groupdel("tgroup")

    group_entry = shadow.tools.getent.group("tgroup")
    assert group_entry is None, "Group should not be found"

    gshadow_file = shadow.fs.exists("/etc/gshadow")
    assert not gshadow_file, "/etc/gshadow file should not be found"


@pytest.mark.topology(KnownTopology.Shadow)
def test_groupdel__delete_group_error_busy_group(shadow: Shadow):
    """
    :title: Group deletion fails when it is primary group for user
    :setup:
        1. Create group
        2. Create user with primary group
    :steps:
        1. Attempt to delete group
        2. Verify that groupdel command fails
        3. Check group and gshadow entries
    :expectedresults:
        1. Group is not deleted
        2. groupdel command fails with error (cannot remove the primary group of user)
        3. Group and gshadow entries are found
    :customerscenario: False
    """
    shadow.groupadd("tgroup")
    shadow.useradd("-g tgroup tuser")

    with pytest.raises(ProcessError) as exc_info:
        shadow.groupdel("tgroup")

    assert (
        exc_info.value.rc == 8
    ), f"Expected return code 8 (cannot remove the primary group of user), got {exc_info.value.rc}"

    group_entry = shadow.tools.getent.group("tgroup")
    assert group_entry is not None, "Group should be found"
    assert group_entry.name == "tgroup", "Incorrect groupname"

    if shadow.host.features["gshadow"]:
        gshadow_entry = shadow.tools.getent.gshadow("tgroup")
        assert gshadow_entry is not None, "Group should be found"
        assert gshadow_entry.name == "tgroup", "Incorrect groupname"


@pytest.mark.topology(KnownTopology.Shadow)
def test_groupdel__delete_non_existing_group(shadow: Shadow):
    """
    :title: Group deletion fails when specified group does not exist
    :setup:
        1. None required
    :steps:
        1. Attempt to delete a non-existing group
        2. Verify that groupdel command fails
    :expectedresults:
        1. Group is not deleted
        2. groupdel command fails with error (group does not exist)
    :customerscenario: False
    """
    with pytest.raises(ProcessError) as exc_info:
        shadow.groupdel("tgroup")

    assert exc_info.value.rc == 6, f"Expected return code 6 (group does not exist), got {exc_info.value.rc}"


@pytest.mark.topology(KnownTopology.Shadow)
@pytest.mark.parametrize(
    "lock_file",
    [
        pytest.param("/etc/group.lock", id="group_file"),
        pytest.param("/etc/gshadow.lock", id="gshadow_file"),
    ],
)
def test_groupdel__delete_group_locked_file(shadow: Shadow, lock_file: str):
    """
    :title: Group deletion fails when a lock file exists
    :setup:
    1. Create group
    2. Create lock file
    :steps:
        1. Attempt to delete group
        2. Verify that groupdel command fails
        3. Check group and gshadow entries
    :expectedresults:
        1. Group is not deleted
        2. groupdel command fails with rc=10 (cannot lock file)
        3. Group and gshadow entries are still found
    :customerscenario: False
    """
    shadow.groupadd("tgroup")
    shadow.fs.touch(lock_file)

    with pytest.raises(ProcessError) as exc_info:
        shadow.groupdel("tgroup")

    assert exc_info.value.rc == 10, f"Expected rc=10 (cannot lock file), got {exc_info.value.rc}"

    group_entry = shadow.tools.getent.group("tgroup")
    assert group_entry is not None, "Group should be found"
    assert group_entry.name == "tgroup", "Incorrect groupname"

    if shadow.host.features["gshadow"]:
        gshadow_entry = shadow.tools.getent.gshadow("tgroup")
        assert gshadow_entry is not None, "Group should be found"
        assert gshadow_entry.name == "tgroup", "Incorrect groupname"
