"""Miscellaneous functions."""

from __future__ import annotations

from typing import Any


def to_list(value: Any | list[Any] | None) -> list[Any]:
    """
    Convert value into a list.

    - if value is ``None`` then return an empty list
    - if value is already a list then return it unchanged
    - if value is not a list then return ``[value]``

    :param value: Value that should be converted to a list.
    :type value: Any | list[Any] | None
    :return: List with the value as an element.
    :rtype: list[Any]
    """
    if value is None:
        return []

    if isinstance(value, list):
        return value

    return [value]


def to_list_of_strings(value: Any | list[Any] | None) -> list[str]:
    """
    Convert given list or single value to list of strings.

    The ``value`` is first converted to a list and then ``str(item)`` is run on
    each of its item.

    :param value: Value to convert.
    :type value: Any | list[Any] | None
    :return: List of strings.
    :rtype: list[str]
    """
    return [str(x) for x in to_list(value)]
