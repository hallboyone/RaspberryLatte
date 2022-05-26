class Bounds:
    """
    Clips a value to configured upper and lower bounds. If the allow_zero flag is
    included, then a zero value will not be clipped to a lower bound greater than zero.
    """
    lower = None
    upper = None
    def __init__(self, lower_bound = None, upper_bound = None, allow_zero = False) -> None:
        self.lower = lower_bound
        self.upper = upper_bound
        self._allow_zero = allow_zero

    def clip(self, val):
        if self._allow_zero and val == 0:
            return val
        if self.lower!=None and self.lower > val:
            val = self.lower
        elif self.upper!=None and self.upper < val:
            val = self.upper
        return val