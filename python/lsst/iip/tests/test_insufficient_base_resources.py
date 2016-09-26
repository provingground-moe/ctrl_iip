import pika
import redis
import yaml
import sys
import os
from time import sleep
import thread
import pytest
import random
import logging

sys.path.insert(0, '../iip')
#sys.path.append('/home/fedora/src/git/ctrl_iip/python/lsst/iip')
from const import *
import toolsmod
from toolsmod import L1Exception
from toolsmod import L1MessageError
from toolsmod import get_timestamp
from BaseForeman import *
from Scoreboard import Scoreboard
from ForwarderScoreboard import ForwarderScoreboard
from JobScoreboard import JobScoreboard
from AckScoreboard import AckScoreboard
from Consumer import Consumer
from SimplePublisher import SimplePublisher

"""
 Creates BaseForeman object used to test the methods below
 Can either last for the entire testing session or per test one can be created

This test file requires the use of two external fixtures:
1) An AMQP message broker
2) The Redis in-memory database

The BaseForeman class s built to receive parameters froma configuration file.
If an init arg is not given when creating a BaseForeman, the default
ForemanCFG.yaml is used...but it is also possible to create a BaseForeman
instance with a specific config file given as the arg - in this case, we will
use a file called ForemanCFGTest.yaml. This file contains the user/passwd
information for access to the message broker, and also gives explicit info
about the Forwarders that this file will use as well.

"""

@pytest.fixture(scope='session')
def bf(request):
    return BaseForeman('ForemanCfgTest.yaml')

fman = bf

f = open('ForemanCfgTest.yaml')
x = yaml.safe_load(f)
base_broker_url = "amqp://" + "TesT" + ":" + "TesT" + "@" + str(x['ROOT']['BASE_BROKER_ADDR'])
def pb(request):
    return SimplePublisher(base_broker_url)


logging.basicConfig(filename='logs/BaseForeman.log', level=logging.INFO, format=LOG_FORMAT)

PUB = None

def setup_publisher(broker_url):
    PUB = SimplePublisher(broker_url)
    #publisher = SimplePublisher(broker_url)
    return True

def on_insufficient_base_message(ch, method, properties, body):
    base_broker_url = "amqp://" + "TesT" + ":" + "TesT" + "@" + "141.142.208.191:5672/%2ftester"
    pub = SimplePublisher(base_broker_url)
    msg_dict = body
    timed_ack = msg_dict["ACK_ID"]
    job_num = msg_dict['JOB_NUM']
    msg_params = {}
    msg_params['MSG_TYPE'] = 'FORWARDER_HEALTH_ACK'
    msg_params['JOB_NUM'] = job_num
    msg_params['ACK_BOOL'] = False
    msg_params['ACK_ID'] = timed_ack
    msg_params['NEEDED_FORWARDERS'] = msg_dict['NEEDED_FORWARDERS']
    msg_params['AVAILABLE_FORWARDERS'] = msg_dict['AVAILABLE_FORWARDERS']
    msg_params['COMPONENT_NAME'] = 'BASE_' + str(get_timestamp()) 
    msg_params['COMMENT'] = yaml.dump(msg_dict)
    publisher.publish_message("ack_publish", msg_params)


def run_consumer(threadname, delay, adict):
    callback = adict['cb']
    adict['csume'].run(callback)


def setup_consumer(base_broker_url, Q, format, callback):
    consumer = Consumer(base_broker_url, Q, format)
    try:
        thread.start_new_thread( run_consumer, ("thread-test-consume", 2, {'csume': consumer, 'cb': callback}))
    except:
        print "Bad trouble creating consumer thread for testing...exiting..."
        sys.exit(101)


def test_insufficient_base_resources(fman):
    os.system('rabbitmqctl -p /tester purge_queue f_consume')
    os.system('rabbitmqctl -p /tester purge_queue forwarder_publish')
    os.system('rabbitmqctl -p /tester purge_queue ack_publish')
    try:
        f = open('ForemanCfgTest.yaml')
    except IOError:
        print "Can't open ForemanCfgTest.yaml"
        print "Bailing out on test_forwarder_check_health..."
        sys.exit(99)

    cdm = yaml.safe_load(f)
    number_of_pairs = cdm['ROOT']['NUMBER_OF_PAIRS']
    base_broker_address = cdm['ROOT']['BASE_BROKER_ADDR']
    name = cdm['ROOT']['BROKER_NAME']
    passwd = cdm['ROOT']['BROKER_PASSWD']
    base_broker_url = "amqp://" + name + ":" + passwd + "@" + str(base_broker_address)
    setup_publisher(base_broker_url)
    setup_consumer(base_broker_url, 'dmcs_consume', 'XML', on_insufficient_base_message)
    forwarders = cdm['ROOT']['XFER_COMPONENTS']['FORWARDERS']

    #The test cfg file includes a list of all 21 rafts in non-consecutive order
    #The NUMBER_OF_PAIRS param is used to generate a sublist of above
    needed_forwarders = number_of_pairs - 1
    L = []
    for i in range (0, (number_of_pairs - 1)):
        L.append(cdm['ROOT']['RAFT_LIST'][i])

    acker = str(42)
    params = {}
    params['MSG_TYPE'] = 'NEW_JOB'
    params['JOB_NUM'] = str(7)
    params['ACK_ID'] = acker
    params['RAFTS'] = L

    available_forwarders = number_of_pairs - 2
    fman.insufficient_base_resources(params, available_forwarders)
    sleep(6)
    ack_responses = fman.ACK_SCBD.get_components_for_timed_ack(acker)

    assert len(ack_responses) == 1
    assert ack_responses[acker]['ACK_BOOL'] == False

    details = ack_responses[acker]['COMMENT']

    assert details['NEEDED_FORWARDERS'] == needed_forwarders
    assert details['AVAILABLE_FORWARDERS'] == available_forwarders
    assert details['JOB_NUM'] == str(7)

def test_ncsa_resources_query(fman):
    pass

def test_distribute_job_params(fman):
    pass

def test_accept_job(fman):
    pass

def test_insufficient_ncsa_resources(fman):
    pass

def test_ncsa_no_response(fman):
    pass