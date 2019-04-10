import pika
#from FirehoseConsumer import FirehoseConsumer
from lsst.ctrl.iip.Consumer import Consumer
from lsst.ctrl.iip.SimplePublisher import SimplePublisher
import sys
import os
import time
import logging
import _thread

class Premium:
  def __init__(self):
    logging.basicConfig()
    # commented out - srp
    #broker_url = 'amqp://FORWARD_F93:FORWARD_F93@140.252.32.128:5672/%2Ftest_at'
    broker_url = 'amqp://FORWARD_F93:FORWARD_F93@141.142.238.10:5672/%2Ftest_at_srp'

    #self.new_thread = Consumer(broker_url, 'telemetry_queue', 'xthread', self.mycallback, 'YAML')
    #self.new_thread.start()

    #cdm = toolsmod.intake_yaml_file("L1SystemCfg.yaml")
    #self.ccd_list = cdm['ROOT']['CCD_LIST']
   
  def mycallback(self, ch, method, properties, body):
    ch.basic_ack(method.delivery_tag)
    print("  ")
    print(">>>>>>>>>>>>>>><<<<<<<<<<<<<<<<")
    print((" [z] body Received %r" % body))
    print(">>>>>>>>>>>>>>><<<<<<<<<<<<<<<<")

    #print("Message done")
    #print("Still listening...")


def main():
  # commented out - srp
  #sp1 = SimplePublisher('amqp://DMCS:DMCS@140.252.32.128:5672/%2Ftest_at', "YAML")
  sp1 = SimplePublisher('amqp://DMCS:DMCS@141.142.238.10:5672/%2Ftest_at', "YAML")

  #while 1:
    #pass
  """
  """
  msg = {}
  msg['MSG_TYPE'] = "STANDBY"
  msg['DEVICE'] = 'AT'
  #msg['CFG_KEY'] = "2C16"
  msg['ACK_ID'] = 'AT_4'
  msg['CMD_ID'] = '4434278812'
  time.sleep(3)
  print("AT STANDBY")
  sp1.publish_message("ocs_dmcs_consume", msg)

  msg = {}
  msg['MSG_TYPE'] = "DISABLE"
  msg['DEVICE'] = 'AT'
  msg['ACK_ID'] = 'AT_6'
  msg['CMD_ID'] = '4434278814'
  time.sleep(3)
  print("AT DISABLE")
  sp1.publish_message("ocs_dmcs_consume", msg)

  msg = {}
  msg['MSG_TYPE'] = "ENABLE"
  msg['DEVICE'] = 'AT'
  msg['ACK_ID'] = 'AT_11'
  msg['CMD_ID'] = '4434278816'
  time.sleep(3)
  print("AT ENABLE")
  sp1.publish_message("ocs_dmcs_consume", msg)

  sp1.close()


if __name__ == "__main__":  main()
