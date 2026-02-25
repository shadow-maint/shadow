# Coding style

The Shadow project is developed in C,
with Python used for testing purposes.
Each language follows its own established conventions and style guidelines.

## C code

* For a general guidance refer to this
[style guide](https://git.kernel.org/pub/scm/docs/man-pages/man-pages.git/tree/CONTRIBUTING.d/style/c).

* For stuff not covered there, follow the
[Linux kernel coding style](https://www.kernel.org/doc/html/latest/process/coding-style.html)

* Patches that change the existing coding style are not welcome, as they make
downstream porting harder for the distributions

### Indentation

Tabs are preferred over spaces for indentation. Loading the `.editorconfig`
file in your preferred IDE may help you configure it.

## Python code

Python code in the Shadow project is primarily found
in the system test framework (`tests/system/`).
Follow these conventions for consistency:

### General conventions

* **PEP 8 compliance**: follow [PEP 8](https://pep8.org/) style guidelines.
* **Code quality enforcement**: all Python code must pass flake8, pycodestyle, isort, mypy, and black checks.
* **Import organization**: use absolute imports with `from __future__ import annotations`.
* **Type hints**: use modern type hints (e.g., `str | None` instead of `Optional[str]`).
* **Line length**: maximum 119 characters per line.
* **Configuration**: all formatting and linting settings are defined in `tests/system/pyproject.toml`.

### Test code style

**File and test naming:**
* Test files: `test_<command>.py` (e.g., `test_useradd.py`).
* Test functions: `test_<command>__<specific_behavior>` using double underscores.
* Use descriptive names that clearly indicate what is being tested.

**Test structure (AAA pattern):**
```python
@pytest.mark.topology(KnownTopology.Shadow)
def test_useradd__add_user(shadow: Shadow):
    """
    :title: Descriptive test title
    :setup:
        1. Setup steps
    :steps:
        1. Test steps
    :expectedresults:
        1. Expected outcomes
    :customerscenario: False
    """
    # Arrange
    setup_code_here()

    # Act
    result = shadow.command_to_test()

    # Assert
    assert result is not None, "Descriptive failure message"
```

**Avoiding flakiness:**
* Use deterministic test data (avoid random values).
* Clean up test artifacts properly (handled automatically by framework).
* Use appropriate timeouts for time-sensitive operations.
* Leverage the framework's automatic backup/restore functionality.

### Formatting and imports

**Required tools:**
* **flake8**: for style guide enforcement and error detection.
* **pycodestyle**: for PEP 8 style checking.
* **isort**: for import sorting with profiles that work well with Black.
* **Black**: for consistent code formatting.
* **mypy**: for static type checking.

**Import order:**
1. Standard library imports.
2. Third-party imports (`pytest`, `pytest_mh`).
3. Local framework imports (`framework.*`).

### Error handling and logging

**Error handling:**
* Prefer explicit exceptions over silent failures.
* Use `ProcessError` for command execution failures.
* Provide context in error messages.

**Logging guidance:**
* Use structured logging for test utilities in `tests/system/framework/`.
* Include relevant context (command, parameters, expected vs actual results).
* Leverage the framework's automatic artifact collection for debugging.
