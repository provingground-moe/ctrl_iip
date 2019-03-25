# This file is part of ctrl_iip
# 
# Developed for the LSST Data Management System.
# This product includes software developed by the LSST Project
# (https://www.lsst.org).
# See the COPYRIGHT file at the top-level directory of this distribution
# for details of code ownership.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.


import threading
import logging
from time import sleep
from lsst.ctrl.iip.Consumer import Consumer
from lsst.ctrl.iip.SimplePublisher import SimplePublisher
from copy import deepcopy

LOG_FORMAT = ('%(levelname) -10s %(asctime)s %(name) -30s %(funcName) '
              '-35s %(lineno) -5d: %(message)s')
LOGGER = logging.getLogger(__name__)
#logging.basicConfig(filename='logs/ThreadManager.log', level=logging.DEBUG, format=LOG_FORMAT)



class ThreadManager(threading.Thread):
    def __init__(self, name, kwargs, shutdown_event):
        threading.Thread.__init__(self, group=None, target=None, name=name) 
        self.running_threads = []
        self.shutdown_event = shutdown_event

        #self.consumer_kwargs = deepcopy(kwargs)
        self.consumer_kwargs = kwargs

        self.lock = threading.Lock()
        consumers = list(self.consumer_kwargs.keys())
        for consumer in consumers:
            x = self.setup_consumer_thread(self.consumer_kwargs[consumer])
            self.lock.acquire()
            self.running_threads.append(x)
            self.lock.release()

    def run(self):
        self.start_background_loop()

    def setup_consumer_thread(self, consumer_params):
        url = consumer_params['amqp_url']
        q = consumer_params['queue']
        threadname = consumer_params['name']
        callback = consumer_params['callback']
        format = consumer_params['format']

        new_thread = Consumer(url, q, threadname, callback, format)
        new_thread.start()
        sleep(1)
        return new_thread


    def start_background_loop(self):
        # Time for threads to start and quiesce
        sleep(2)
        try:
            while 1:
                # self.get_next_backlog_item() 
                if self.shutdown_event.isSet():
                    #self.shutdown_consumers()
                    return
                    #break
                sleep(1)
                self.check_thread_health()
                # self.resolve_non-blocking_acks() 
        except KeyboardInterrupt:
            pass


    def check_thread_health(self):
        self.lock.acquire()
        num_threads = len(self.running_threads)
        for i in range(0, num_threads):
            if self.running_threads[i].is_alive():
                continue
            else:
                LOGGER.critical("Thread with name %s has died. Attempting to restart..." 
                                 % self.running_threads[i].name)
                dead_thread_name = self.running_threads[i].name
                del self.running_threads[i]
                ### Restart thread...
                new_consumer = self.setup_consumer_thread(self.consumer_kwargs[dead_thread_name])

                self.running_threads.append(new_consumer)
        self.lock.release()


    def shutdown_consumers(self):
        self.lock.acquire()
        num_threads = len(self.running_threads)
        LOGGER.info("shutting down consumer threads")
        for i in range (0, num_threads):
            LOGGER.info("Stopping rabbit connection in consumer %s" % self.running_threads[i].name)
            self.running_threads[i].stop()
            LOGGER.info("Shutting down consumer %s" % self.running_threads[i].name)
            self.running_threads[i].join()
            LOGGER.info("consumer %s finished" % self.running_threads[i].name)
        self.lock.release()

        LOGGER.info("consumer thread shutdown completed")
