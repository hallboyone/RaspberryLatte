"""
API to log values of the different espresso machine parameters. To set up,
a logging object is provided with data sources and a sample time using add_source. Then frequent
polling calls to Logger.log() will read the different data sources and produce
data points at the sample times if needed. To finish, Logger.finish(filename)
while write the log to a file and reset the logger.
"""
from datetime import datetime
import time

class Logger:
    def __init__(self, sample_time = 0.1) -> None:
        self._ts = sample_time
        self._sources = {"t" : time.time}
        self._data : list[dict[str, float]] = []
        self._t0 = None
    
    def add_source(self, name : str, source):
        """Add a source to the logger. source paramter should be a function that takes no
        parameters but returns a numeric value castable to float: source(None)->float"""
        self._sources[name] = source

    def log(self):
        """Records each of the current source values if the sample time has elapsed."""
        if self._t0 == None:
            self._t0 = time.time()
            self._next_sample_t = self._t0 + self._ts
            self._log_datapoint()
        elif self._next_sample_t < time.time():
            self._log_datapoint()
            self._next_sample_t = self._next_sample_t + self._ts

    def finish(self, filename=None):
        """Writes all collected data to a brewlog file (or whatever filename is provided)"""
        if filename==None:
            filename = self._datetime_filename_generator()
        file = open(filename, "w")

        for datapoint in self._data:
            for name in datapoint:
                file.write(f"{name}={datapoint[name]}; ")
            file.write("\n")
        file.close()
        self._data : list[dict[str, float]] = []
        self._t0 = None


    def _log_datapoint(self):
        datapoint = {}
        for name in self._sources:
            datapoint[name] = self._sources[name]()
        self._data.append(datapoint)

    def _datetime_filename_generator(self)->str:
        date = datetime.now()
        return f"{date.year}-{date.month}-{date.day}-{date.hour}:{date.minute}:{date.second}.brewlog"