from time import time, sleep

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
            self._points = {self._points[-2], self._points[-1]}
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
    def __init__(self, windup_bounds = None) -> None:
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
            self._sum = self._sum + ((self._prev_val + point)/2)*(t - self._prev_time)
            self._prev_time = t
            self._prev_val = point
            self._clip_to_bounds()

    def reset(self) -> None:
        self._sum = 0;
        self._prev_time = None
        self._prev_val = None

    def sum(self) -> float:
        return self._sum

    def _clip_to_bounds(self):
        if self._windup_bounds:
            self._sum = max(self._sum, self._windup_bounds[0])
            self._sum = min(self._sum, self._windup_bounds[1])
# DiscreteIntegral

class PIDGains:
    Kp = None
    Ki = None
    Kd = None

class PIDSensor:
    def read(self) -> float:
        pass

class PIDOutput:
    def write(self, val : float):
        pass

class PID:
    def __init__(self, gains : PIDGains) -> None:
        self._gains = gains
        self._sensor : PIDSensor = None
        self._output : PIDOutput = None
        self._derivative = DiscreteDerivative()
        self._integral = DiscreteIntegral()
        self._setpoint = 0

    def attach_sensor(self, sensor : PIDSensor):
        self._sensor = sensor
    
    def attach_output(self, output : PIDOutput):
        self._output = output

    def update_setpoint_to(self, setpoint : float):
        self._setpoint = setpoint

    def tick(self):
        val = self._sensor.read()
        err = self._setpoint - val
        self._derivative.add_point(val)
        self._integral.add_point(err)
        output = self._gains.Kp * err + self._gains.Ki * self._integral.sum() + self._gains.Kd * self._derivative.slope()
        self._output.write(self._gains.Kp * err +
                           self._gains.Ki * self._integral.sum() +
                           self._gains.Kd * self._derivative.slope())