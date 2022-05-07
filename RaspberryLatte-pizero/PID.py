from audioop import avg
import numpy as np
from time import time, sleep


class DiscreteDerivative:
    """
    Creates a Discrete Derivative object that tracks the current rate of change for a discrete
    timeseries. Only the points within the filter timespan are used to compute the rate of change

    Attributes
    ----------
    _points : [float]]
        array of sample values
    _times : [float]
        times the corresponding samples were added
    _filter_span : float
        length of time back from latest point to consider when fitting slope. If 0, only the last two reading are kept
        

    Methods
    -------
    __init__(filter_span : float):
        Sets up the object with empty lists and given filter_span
    add_point(point : float):
        appends the new point to _points and records the call time
    slope()->float:
        returns the slope of the line fitted to the points within the filter's timespan
    """
    def __init__(self, filter_span : float = 0) -> None:
        self._points: list[float] = []
        self._times: list[float] = []
        self._filter_span = filter_span

    def add_point(self, point : float):
        self._times.append(time())
        self._points.append(point)

    def _clean_points(self):
        if (self._filter_span <= 0):
            self._points = {self._points[-2], self._points[-1]}
        else:
            while(len(self._points)>2 and self._times[-1] - self._times[0] > self._filter_span):
                self._points.pop(0)
                self._times.pop(0)
        
    def slope(self) -> float:
        if(len(self._points < 2)):
            raise Exception("Must have 2 or more points to compute the slope")
        
        self._clean_points()
        p_avg = average(self._points)
        t_avg = average(self._times)

        return p[0]



dt = DiscreteDerivative(7)
dt.add_point(0)
sleep(1)
dt.add_point(1.1)
sleep(1)
dt.add_point(1.9)
sleep(1)
dt.add_point(2.9)
sleep(1)
dt.add_point(4.1)
sleep(1)
dt.add_point(5)
print(dt.slope())