import subprocess 
import os
import sys
import traceback
import signal
import pytest
sys.path.insert(1, "../iip")
sys.path.insert(1, "../")
from Consumer import Consumer 
from MessageAuthority import MessageAuthority
from const import * 
import toolsmod 
from time import sleep


class TestOCS_CommandListener: 

    # TODO: KILL CommandListener with pid
    # os.chdir("ocs/src")
    # subprocess.call("./CommandListener&", shell=True)
    # sleep(5) 

    EXPECTED_DMCS_MESSAGES = 24
    dmcs_consumer = None
    dmcs_consumer_msg_list = [] 

    def test_ocs_commandlistener(self): 
        try: 
            cdm = toolsmod.intake_yaml_file("/home/centos/src/git/ctrl_iip/python/lsst/iip/tests/yaml/L1SystemCfg_Test_ocs_cmdListener.yaml")
        except IOError as e: 
            trace = traceback.print_exc() 
            emsg = "Unable to fine CFG Yaml file %s\n" % self._config_file 
            print(emsg + trace)
            sys.exit(101) 

        broker_addr = cdm[ROOT]["BASE_BROKER_ADDR"] 

        dmcs_name = cdm[ROOT]["DMCS_BROKER_NAME"] 
        dmcs_pwd = cdm[ROOT]["DMCS_BROKER_PASSWD"]

        dmcs_broker_url = "amqp://" + dmcs_name + ":" + \
                                      dmcs_pwd + "@" + \
                                      broker_addr 

        self.dmcs_consumer = Consumer(dmcs_broker_url, "ocs_dmcs_consume", "thread-dmcs-consume", 
                                      self.on_ocs_message, "YAML", None) 
        self.dmcs_consumer.start()
        print("Test setup Complete. Commencing Messages...")

        self._msg_auth = MessageAuthority("/home/centos/src/git/ctrl_iip/python/lsst/iip/messages.yaml")

        self.send_messages() 
        sleep(10)

        self.verify_ocs_messages() 
        print("Finished with CommandListener tests.") 

    def send_messages(self): 
        # os.chdir("../commands/")
        os.chdir("ocs/commands/")
        
        commands = ["start", "stop", "enable", "disable", "enterControl", "exitControl", "standby", "abort"] 
        devices = ["archiver", "catchuparchiver", "processingcluster"] 

        for device in devices: 
            for command in commands: 
                cmd = "./sacpp_" + device + "_" + command + "_commander 0"
                p = subprocess.Popen(cmd, shell=True, preexec_fn=os.setsid)
                print("=== " + device.upper() + " " + command.upper() + " Message")
                sleep(10)  # this is not random. startup .sacpp_ thing takes about 7 seconds. 
                os.killpg(os.getpgid(p.pid), signal.SIGTERM) 

        print("Message Sender Done.") 

    def verify_ocs_messages(self): 
        print("Messages received by verify_ocs_messages:")
        len_list = len(self.dmcs_consumer_msg_list)
        if len_list != self.EXPECTED_DMCS_MESSAGES: 
            print("Messages received by verify_ocs_messages:")
            pytest.fail("DMCS simulator received incorrect number of messages.\n Expected %s but received %s" \
                    % (self.EXPECTED_DMCS_MESSAGES, len_list))

        for i in range(0, len_list): 
            msg = self.dmcs_consumer_msg_list[i]
            result = self._msg_auth.check_message_shape(msg)
            if result == False: 
                pytest.fail("The following OCS Bridge response message failed when compared with the sovereign\
                             example: %s" % msg)
        print("Responses to DMCS pass verification")

    def clear_message_lists(self): 
        self.dmcs_consumer_msg_list = [] 


    def on_ocs_message(self, ch, method, properties, body): 
        self.dmcs_consumer_msg_list.append(body)