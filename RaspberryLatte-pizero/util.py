
class Bounds:
    lower = None
    upper = None
    def __init__(self, lower_bound = None, upper_bound = None) -> None:
        self.lower = lower_bound
        self.upper = upper_bound

    def clip(self, val):
        if self.lower!=None and self.lower > val:
            val = self.lower
        elif self.upper!=None and self.upper < val:
            val = self.upper
        return val