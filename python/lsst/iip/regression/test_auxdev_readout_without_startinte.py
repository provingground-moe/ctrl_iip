import sys
sys.path.append("../")
from SimplePublisher import SimplePublisher
from Consumer import Consumer
import toolsmod
import logging
import time

class Premium:
    def __init__(self):
        logging.basicConfig()
        broker_url = "amqp://ARCHIE:ARCHIE@140.252.32.128:5672/%2Ftest_at"

        self.new_thread = Consumer(broker_url, 'telemetry_queue', 'xthread', self.mycallback, 'YAML')
        self.new_thread.start()

        self.publisher = SimplePublisher('amqp://DMCS:DMCS@140.252.32.128:5672/%2Ftest_at', "YAML")

        self.send_messages() 
        time.sleep(12)

    def mycallback(self, ch, method, properties, body):
        ch.basic_ack(method.delivery_tag)
        msg = yaml.load(body) 
        if msg["STATUS_CODE"] == 1: 
            print("PASSED")
        else: 
            print("FAILED")

    def send_messages(self): 

        IMAGE_ID = "AT_O_20181003_000014"

        msg = {}
        msg['MSG_TYPE'] = "DMCS_AT_END_READOUT"
        msg['IMAGE_ID'] = IMAGE_ID
        msg['IMAGE_INDEX'] = '2'
        msg['IMAGE_SEQUENCE_NAME'] = 'MAIN'
        msg['IMAGES_IN_SEQUENCE'] = '3'
        msg['RESPONSE_QUEUE'] = "dmcs_ack_consume"
        msg['ACK_ID'] = 'READOUT_ACK_77'
        self.publisher.publish_message("ocs_dmcs_consume", msg)
        print("READOUT Message")
        time.sleep(2)

        msg = {}
        msg["MSG_TYPE"] = "DMCS_AT_HEADER_READY"
        msg["IMAGE_ID"] = IMAGE_ID 
        msg["FILENAME"] = "http://localhost:8000/visitJune-28.header"
        self.publisher.publish_message("ocs_dmcs_consume", msg)
        print("HEADER_READY Message")
        time.sleep(2)

        print("Sender done")


def main():
    p = Premium()

if __name__ == "__main__":  main()
