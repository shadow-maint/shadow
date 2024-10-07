"""Pytest fixtures."""

from __future__ import annotations

from functools import partial

import pytest
from pytest_mh import MultihostItemData, Topology

from .misc import to_list_of_strings
from .roles.base import BaseRole
from .topology import KnownTopology, KnownTopologyGroup


def pytest_configure(config: pytest.Config):
    """
    Pytest hook: register multihost plugin.
    """

    # register additional markers
    config.addinivalue_line(
        "markers",
        "builtwith(feature): Run test only if shadow was built with given feature",
    )


def builtwith(item: pytest.Function, requirements: dict[str, str], **kwargs: BaseRole):
    def value_error(msg: str) -> ValueError:
        return ValueError(f"{item.nodeid}::{item.originalname}: @pytest.mark.builtwith: {msg}")

    errors: list[str] = []
    for role, features in requirements.items():
        if role not in kwargs:
            raise value_error(f"unknown fixture '{role}'")

        if not isinstance(kwargs[role], BaseRole):
            raise value_error(f"fixture '{role}' is not instance of BaseRole")

        obj = kwargs[role]
        for feature in to_list_of_strings(features):
            if feature not in obj.features:
                raise value_error(f"unknown feature '{feature}' in '{role}'")

            if not obj.features[feature]:
                errors.append(f'{role} does not support "{feature}"')

    if len(errors) == 1:
        return (False, errors[0])
    elif len(errors) > 1:
        return (False, str(errors))

    # All requirements were passed
    return True


@pytest.hookimpl(tryfirst=True)
def pytest_runtest_setup(item: pytest.Item) -> None:
    if not isinstance(item, pytest.Function):
        raise TypeError(f"Unexpected item type: {type(item)}")

    topology: list[Topology] = []
    mh_item_data: MultihostItemData | None = MultihostItemData.GetData(item)
    for mark in item.iter_markers("builtwith"):
        requirements: dict[str, str] = {}

        if len(mark.args) == 1 and not mark.kwargs:
            # @pytest.mark.builtwith("feature_x")
            #  -> check if "feature_x" is supported by shadow
            requirements["shadow"] = mark.args[0]
            topology = []
        elif not mark.args and mark.kwargs:
            # @pytest.mark.builtwith(shadow="feature_x", another_host="feature_x") ->
            # -> check if "feature_x" is supported by both shadow and another_host
            requirements = dict(mark.kwargs)
            topology = []
        elif (
            len(mark.args) == 1
            and isinstance(mark.args[0], (Topology, KnownTopology, KnownTopologyGroup))
            and mark.kwargs
        ):
            # @pytest.mark.builtwith(KnownTopology.Shadow, shadow="feature_x") ->
            # -> check if "feature_x" is supported by shadow only if the test runs on shadow topology
            requirements = dict(mark.kwargs)
            if isinstance(mark.args[0], Topology):
                topology = [mark.args[0]]
            elif isinstance(mark.args[0], KnownTopology):
                topology = [mark.args[0].value.topology]
            elif isinstance(mark.args[0], KnownTopologyGroup):
                topology = [x.value.topology for x in mark.args[0].value]
        else:
            raise ValueError(f"{item.nodeid}::{item.originalname}: invalid arguments for @pytest.mark.builtwith")

        if mh_item_data is None:
            raise ValueError(f"{item.nodeid}::{item.originalname}: multihost item data is not set")

        if mh_item_data.topology_mark is None:
            raise ValueError(f"{item.nodeid}::{item.originalname}: multihost topology mark is not set")

        if not topology or mh_item_data.topology_mark.topology in topology:
            item.add_marker(pytest.mark.require(partial(builtwith, item=item, requirements=requirements)))
