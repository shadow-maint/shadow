"""
Test getsubids
"""

from __future__ import annotations

import pytest

from framework.roles.shadow import Shadow
from framework.topology import KnownTopology


@pytest.mark.topology(KnownTopology.Shadow)
def test_getsubids__list_ranges(shadow: Shadow):
    """
    :title: List the subuid and subgid ranges of a user
    :setup:
        1. Create user
        2. Give the user one subuid range and one subgid range
    :steps:
        1. List the subuid ranges of the user
        2. List the subgid ranges of the user
    :expectedresults:
        1. One subuid range is returned, 100000 with 65536 ids, owned by the user
        2. One subgid range is returned, 200000 with 65536 ids, owned by the user
    :customerscenario: False
    """
    shadow.useradd("tuser")
    shadow.fs.write("/etc/subuid", "tuser:100000:65536\n", dedent=False)
    shadow.fs.write("/etc/subgid", "tuser:200000:65536\n", dedent=False)

    uids = shadow.tools.getsubids("tuser")
    assert uids is not None, "Subuid ranges should be found"
    assert len(uids) == 1, "Incorrect number of subuid ranges"
    assert uids[0].index == 0, "Incorrect index"
    assert uids[0].owner == "tuser", "Incorrect owner"
    assert uids[0].start == 100000, "Incorrect start"
    assert uids[0].count == 65536, "Incorrect count"

    gids = shadow.tools.getsubids("tuser", gid=True)
    assert gids is not None, "Subgid ranges should be found"
    assert len(gids) == 1, "Incorrect number of subgid ranges"
    assert gids[0].index == 0, "Incorrect index"
    assert gids[0].owner == "tuser", "Incorrect owner"
    assert gids[0].start == 200000, "Incorrect start"
    assert gids[0].count == 65536, "Incorrect count"


@pytest.mark.topology(KnownTopology.Shadow)
def test_getsubids__user_without_ranges(shadow: Shadow):
    """
    :title: User without subordinate ids
    :setup:
        1. Create user
        2. Write subuid and subgid files without a range for the user
    :steps:
        1. List the subuid ranges of the user
        2. List the subgid ranges of the user
    :expectedresults:
        1. getsubids succeeds and returns no range
        2. getsubids succeeds and returns no range
    :customerscenario: False
    """
    shadow.useradd("tuser")
    shadow.fs.write("/etc/subuid", "root:100000:65536\n", dedent=False)
    shadow.fs.write("/etc/subgid", "root:100000:65536\n", dedent=False)

    uids = shadow.tools.getsubids("tuser")
    assert uids is not None, "getsubids should succeed"
    assert not uids, "User should have no subuid ranges"

    gids = shadow.tools.getsubids("tuser", gid=True)
    assert gids is not None, "getsubids should succeed"
    assert not gids, "User should have no subgid ranges"


@pytest.mark.topology(KnownTopology.Shadow)
def test_getsubids__unknown_user(shadow: Shadow):
    """
    :title: User that does not exist
    :setup:
        1. Write an empty subuid file
    :steps:
        1. List the subuid ranges of a user that does not exist
    :expectedresults:
        1. getsubids succeeds and returns no range
    :customerscenario: False
    """
    shadow.fs.write("/etc/subuid", "")

    uids = shadow.tools.getsubids("nosuchuser")
    assert uids is not None, "getsubids should succeed"
    assert not uids, "Unknown user should have no subuid ranges"
