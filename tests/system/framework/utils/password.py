"""Password field validation utilities for shadow test framework."""

from __future__ import annotations

from enum import Enum


class Algorithm(Enum):
    SHA256 = "5"
    SHA512 = "6"
    BCRYPT = "2b"
    YESCRYPT = "y"


class PasswordUtils:
    DEFAULT_ALGORITHM = Algorithm.SHA512

    def __init__(self, default_algorithm=None):
        if default_algorithm is None:
            self.default_algorithm = self.DEFAULT_ALGORITHM
        else:
            self.default_algorithm = default_algorithm

    def assert_hash(self, value: str | None, algorithm: Algorithm | None = None) -> None:
        if algorithm is None:
            algo = self.default_algorithm
        else:
            algo = algorithm

        prefix = "$" + algo.value + "$"

        if value is None:
            raise AssertionError(f"Expected valid {algo.name} hash, but password is empty. Value: {value}")
        if not value.startswith(prefix):
            raise AssertionError(f"Expected valid {algo.name} hash, but algorithm prefix is not found. Value: {value}")
        if value.count("$") < 3:
            raise AssertionError(f"Expected valid {algo.name} hash, but password format is invalid. Value: {value}")

    def assert_locked(self, value: str | None) -> None:
        if value != "!" and value != "!!":
            raise AssertionError(f"Expected locked password ('!' or '!!') with no preserved hash. Value: {value}")

    def assert_locked_hash(self, value: str | None, algorithm: Algorithm | None = None) -> None:
        if algorithm is None:
            algo = self.default_algorithm
        else:
            algo = algorithm

        if value is None:
            raise AssertionError(f"Expected locked {algo.name} hash starting with '!'. Value: {value}")
        if not value.startswith("!"):
            raise AssertionError(f"Expected locked {algo.name} hash starting with '!'. Value: '{value}'")

        if value is not None and len(value) > 0:
            string = value[1:]
        else:
            string = ""

        prefix = "$" + algo.value + "$"

        if not string.startswith(prefix):
            raise AssertionError(
                f"Expected locked {algo.name} hash, but algorithm prefix is not found. Value: {value}"
            )
        if string.count("$") < 3:
            raise AssertionError(f"Expected locked {algo.name} hash, but password format is invalid. Value: {value}")

    def assert_no_password(self, value: str | None) -> None:
        if value != "*":
            raise AssertionError(f"Expected no-password marker ('*'). Value: {value}")

    def assert_in_shadow(self, value: str | None) -> None:
        if value != "x":
            raise AssertionError(f"Expected shadow pointer ('x'). Value: {value}")

    def assert_empty(self, value: str | None) -> None:
        if value != "":
            raise AssertionError(f"Expected empty password. Value: {value}")
