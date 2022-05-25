from abc import ABCMeta, abstractmethod
from time import time
from util import Bounds as IntegralBounds

class DiscreteDerivative:
    def __init__(self, filter_span : float = 0) -> None:
        self._points: list[float] = []
        self._times: list[float] = []
        self._filter_span = filter_span

    def add_point(self, point : float):
        self._times.append(time())
        self._points.append(point)

    def reset(self):
        self._points = []
        self._times = []

    def _remove_old_points(self):
        if (self._filter_span <= 0):
            self._points = [self._points[-2], self._points[-1]]
        else:
            while(len(self._points)>2 and self._times[-1] - self._times[0] > self._filter_span):
                self._points.pop(0)
                self._times.pop(0)
        
    def slope(self) -> float:
        if(len(self._points) < 2):
            return 0
        
        self._remove_old_points()
        p_avg = sum(self._points)/len(self._points)
        t_avg = sum(self._times)/len(self._times)

        num = 0
        dem = 0
        for i in range(len(self._points)):
            num = num + (self._times[i]-t_avg)*(self._points[i]-p_avg)
            dem = dem + ((self._times[i]-t_avg)**2)
        return num/dem
# DiscreteDerivative

class DiscreteIntegral:
    def __init__(self, windup_bounds : IntegralBounds = None) -> None:
        self._sum : float = 0
        self._windup_bounds = windup_bounds
        self._prev_time = None
        self._prev_val = None

    def add_point(self, point : float) -> None:
        if self._prev_time == None:
            self._prev_time = time()
            self._prev_val = point
        else:
            t = time()
            self._sum = self._windup_bounds.clip(self._sum + ((self._prev_val + point)/2)*(t - self._prev_time))
            self._prev_time = t
            if self._windup_bounds != None:
                self._prev_val = point

    def reset(self) -> None:
        self._sum = 0
        self._prev_time = None
        self._prev_val = None

    def sum(self) -> float:
        return self._sum
# DiscreteIntegral

class PIDGains:
    Kp = None
    Ki = None
    Kd = None
    def __init__(self, kp, ki, kd) -> None:
        self.Kp = kp
        self.Ki = ki
        self.Kd = kd

class PIDSensor(metaclass=ABCMeta):
    @abstractmethod
    def read(self) -> float:
        pass

class PIDOutput(metaclass=ABCMeta):
    """
    Object that implements the write function called by a PID controller to write the current controller
    value to the plant. Constraints on the output are implemented here.
    """
    @abstractmethod
    def write(self, val : float) -> float:
        """
        Implement in subclass. Should take the controller value, constrain it (optional), write to plant, and
        return the constrained result.
        """
        return float

class PID:
    """
    A PID controller object. User configurable paramters include the controller gains, the setpoint, the
    sensor, and the output. The sensor and output should be subclasses of the PIDSensor class with read 
    and write methods respectivly. 
    """
    def __init__(self, gains : PIDGains, setpoint : float = 0, 
                 sensor : PIDSensor = None, output : PIDOutput = None,
                 min_dwell_time = 1, windup_bounds : IntegralBounds = None) -> None:
        self._gains = gains
        self._sensor : PIDSensor = sensor
        self._output : PIDOutput = output
        self._derivative = DiscreteDerivative()
        self._integral = DiscreteIntegral(windup_bounds)
        self._setpoint = setpoint
        self._min_dt = min_dwell_time

        self._last_tick_time = time()

    def attach_sensor(self, sensor : PIDSensor):
        self._sensor = sensor
    
    def attach_output(self, output : PIDOutput):
        self._output = output

    def update_setpoint_to(self, setpoint : float):
        self._setpoint = setpoint

    def at_setpoint(self, err_tol = 5) -> bool:
        val = self._sensor.read()
        if (val-self._setpoint < err_tol) and (val-self._setpoint > err_tol):
            return True
        else:
            return False
         
    def tick(self):
        if self._sensor == None or self._output == None:
            print("Must attach sensor and output before running controller")
            return
        if self._last_tick_time!=None and time()-self._last_tick_time>self._min_dt:
            self._last_tick_time = time()
            val = self._sensor.read()
            err = self._setpoint - val
            print(f"Temp is {val} so error is {err}")
            self._derivative.add_point(-val)
            self._integral.add_point(err)
            self._output.write(self._gains.Kp * err +
                            self._gains.Ki * self._integral.sum() +
                            self._gains.Kd * self._derivative.slope())
            print(f"P = {self._gains.Kp * err}, I = {self._gains.Ki * self._integral.sum()}, D = {self._gains.Kd * self._derivative.slope()}")

    def reset(self):
        self._derivative.reset()
        self._integral.reset()