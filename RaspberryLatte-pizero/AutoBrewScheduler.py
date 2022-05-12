"""
Usage:
brew_cycle = AutoBrewScheduler([Ramp(from_pwr=60, to_pwr = 90, in_sec = 1),
        ])
"""
from time import time, sleep

class AutoBrewLeg:
    def tick(self) -> tuple((float, bool, bool)):
        """ Subclasses should write this. Returns the current pump input and if leg is completed """
        return (0, False)

class Ramp(AutoBrewLeg):
    def __init__(self, from_pwr = 0, to_pwr = 127, in_sec = 1) -> None:
        super().__init__()
        self._from = from_pwr
        self._to = to_pwr
        self._in_s = in_sec
        self._slope = (self._to-self._from)/self._in_s
        self._start_time = None
        self._end_time = None
        self._last_call_time = None
        self._min_dt = 0.05
        self._last_val_pair = None

    def tick(self) -> tuple((float, bool, bool)):
        if self._start_time == None:
            self._last_call_time = time()
            self._start_time = self._last_call_time 
            self._end_time = self._start_time + self._in_s
            self._last_val_pair = (self._from, True, self._last_call_time >= self._end_time)
        elif self._last_call_time + self._min_dt < time():
            self._last_call_time = time()
            val = min(self._to, self._from + (self._last_call_time - self._start_time)*self._slope)
            self._last_val_pair = (val, True, self._last_call_time >= self._end_time)
        else:
            self._last_val_pair = (self._last_val_pair[0], False, self._last_call_time >= self._end_time)
        return self._last_val_pair

class ConstantTimed(AutoBrewLeg):
    def __init__(self, pwr = 127, for_sec = 1) -> None:
        super().__init__()
        self._pwr = pwr
        self._for_s = for_sec
        
        self._end_time = None
        self._last_val_pair = None

    def tick(self) -> tuple((float, bool)):
        if self._end_time == None:
            self._end_time = time() + self._for_s
            return (self._pwr, True, time() >= self._end_time)
        else:
            return (self._pwr, False, time() >= self._end_time)
            
class ConstantTriggered(AutoBrewLeg):
    def __init__(self, trigger_callback, pwr = 127, timeout_s = 60) -> None:
        super().__init__()
        self._pwr = pwr
        self._trigger = trigger_callback
        
        self._timeout_s = timeout_s
        self._end_time = None

    def tick(self) -> tuple((float, bool)):
        if self._end_time == None:
            self._end_time = time() + self._timeout_s
            return (self._pwr, True, self._trigger() or time() >= self._end_time)
        else:
            return (self._pwr, False, self._trigger() or time() >= self._end_time)

class AutoBrewScheduler:
    def __init__(self, legs) -> None:
        self._legs = legs
        self._cur_leg = 0

    def tick(self) -> tuple((float,bool,bool)):
        val, updated, finished = self._legs[self._cur_leg].tick()
        if (finished):
            self._cur_leg = self._cur_leg + 1
            if self._cur_leg == len(self._legs):
                return (val, updated, True)
        return (val, updated, False)
    


if __name__=="__main__":
    print("Testing...")
    pre_infuse_routine = [Ramp(from_pwr = 60, to_pwr = 80, in_sec = 0.5),
                          ConstantTimed(pwr = 80, for_sec = 0.5),
                          ConstantTimed(pwr = 0,  for_sec = 5),
                          Ramp(from_pwr = 60, to_pwr = 127, in_sec = 1),
                          ConstantTriggered(lambda : 5<3, pwr = 127)]
    auto_brew = AutoBrewScheduler(pre_infuse_routine)
    
    val, updated, finished = auto_brew.tick()
    while(not finished):
        if updated:
            print(val)
        sleep(0.01)
        val, updated, finished = auto_brew.tick()
    print(val)