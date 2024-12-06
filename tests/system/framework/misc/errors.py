from __future__ import annotations


class ExpectScriptError(Exception):
    """
    Expect script error.

    Seeing this exception means that there is an unhandled path or other error
    in the expect script that was executed. The script needs to be fixed.
    """

    def __init__(self, code: int, msg: str | None = None) -> None:
        """
        :param code: Expect script error code.
        :type code: int
        :param msg: Error message, defaults to None (translate error code to message)
        :type msg: str | None, optional
        """
        self.code: int = code
        if msg is None:
            msg = self.code_to_message(code)

        super().__init__(msg)

    def code_to_message(self, code: int) -> str:
        """
        Translate expect script error codes used in this framework to message.

        :param code: Expect script error code.
        :type code: int
        :return: Error message.
        :rtype: str
        """
        match code:
            case 201:
                return "Timeout, unexpected output"
            case 202:
                return "Unexpected end of file"
            case 203:
                return "Unexpected code path"

        return "Unknown error code"
