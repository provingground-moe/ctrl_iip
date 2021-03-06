/*
 * This file is part of ctrl_iip
 *
 * Developed for the LSST Data Management System.
 * This product includes software developed by the LSST Project
 * (https://www.lsst.org).
 * See the COPYRIGHT file at the top-level directory of this distribution
 * for details of code ownership.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <iostream> 
#include <sstream> 
#include <pthread.h>
#include <string>
#include <algorithm>
#include <yaml-cpp/yaml.h>
#include "CommandListener.h"
#include "Toolsmod.h"

BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(lg, src::severity_logger_mt< severity_level >);
#include "IIPMacro.h"

using namespace std;
using namespace YAML;
using namespace ATArchiver; 
using namespace MTArchiver; 
using namespace CatchupArchiver; 
using namespace PromptProcessing; 

int next_timed_ack_id = 0; 

template <typename T, typename U> 
using funcptr = int (T::*)(U*); 

map<string, string> camel_cases = { 
    {"START", "start"}, 
    {"ENABLE", "enable"},
    {"DISABLE", "disable"},
    {"ENTER_CONTROL", "enterControl"}, 
    {"EXIT_CONTROL", "exitControl"}, 
    {"STANDBY", "standby"}, 
    {"ABORT", "abort"}, 
    {"RESET_FROM_FAULT", "resetFromFault"} 
}; 

template <typename SAL_device, typename SAL_struct>
void listenCommand(SAL_device mgr, string device, string command_name, SimplePublisher* publisher, string publish_q, 
string consume_q, funcptr<SAL_device, SAL_struct> acceptCommand){ 
    os_time delay_10ms = { 0, 10000000 }; 
    int cmdId = -1; 
    SAL_struct SALInstance; 

    string processor = CommandListener::get_device(device) + "_command_" + camel_cases[command_name]; 
    mgr.salProcessor(const_cast<char *>(processor.c_str())); 
    while (1) { 
	cmdId = (mgr.*acceptCommand)(&SALInstance); 
	if (cmdId > 0) { 
	    LOG_DBG << "Got command " << command_name << "for device " << device << " at " << to_string(cmdId); 
	    string ack_id = CommandListener::get_next_timed_ack_id(command_name); 
            Emitter ack_msg; 
            ack_msg << BeginMap; 
            ack_msg << Key << "MSG_TYPE" << Value << command_name; 
	    ack_msg << Key << "DEVICE" << Value << device;
            ack_msg << Key << "CMD_ID" << Value << to_string(cmdId); 
	    ack_msg << Key << "ACK_ID" << Value << ack_id; 
            ack_msg << EndMap; 
	    LOG_DBG << "XXX NORMAL: " << command_name << ": " << ack_msg.c_str(); 

            Emitter book_keeping; 
            book_keeping << BeginMap; 
	    book_keeping << Key << "MSG_TYPE" << Value << "BOOK_KEEPING";
            book_keeping << Key << "SUB_TYPE" << Value << command_name;
	    book_keeping << Key << "ACK_ID" << Value << ack_id ;
	    book_keeping << Key << "CHECKBOX" << Value << "false"; 
	    book_keeping << Key << "TIME" << Value << get_current_time();
	    book_keeping << Key << "CMD_ID" << Value << to_string(cmdId); 
	    book_keeping << Key << "DEVICE" << Value << device; 
            book_keeping << EndMap; 
	    LOG_DBG << "XXX BOOK_KEEPING: " << book_keeping.c_str(); 

	    publisher->publish_message(consume_q, book_keeping.c_str()); 
	    publisher->publish_message(publish_q, ack_msg.c_str());  
	}
	os_nanoSleep(delay_10ms); 
    } 
    mgr.salShutdown(); 
}  

template <typename SAL_device, typename SAL_struct>
void listenCommand_start(SAL_device mgr, string device, string command_name, SimplePublisher* publisher, string publish_q, 
string consume_q, funcptr<SAL_device, SAL_struct> acceptCommand){ 
    os_time delay_10ms = { 0, 10000000 }; 
    int cmdId = -1; 
    SAL_struct SALInstance; 

    string processor = CommandListener::get_device(device) + "_command_" + camel_cases[command_name]; 
    mgr.salProcessor(const_cast<char *>(processor.c_str())); 
    while (1) { 
	cmdId = (mgr.*acceptCommand)(&SALInstance); 
	if (cmdId > 0) { 
	    LOG_DBG << "Got command " << command_name << "for device " << device << " at " << to_string(cmdId); 
	    string ack_id = CommandListener::get_next_timed_ack_id(command_name); 
            Emitter ack_msg; 
            ack_msg << BeginMap; 
            ack_msg << Key << "MSG_TYPE" << Value << command_name; 
	    ack_msg << Key << "DEVICE" << Value << device;
            ack_msg << Key << "CMD_ID" << Value << to_string(cmdId); 
	    ack_msg << Key << "ACK_ID" << Value << ack_id; 
	    ack_msg << Key << "CFG_KEY" << Value << SALInstance.settingsToApply; 
            ack_msg << EndMap; 
	    LOG_DBG << "XXX NORMAL: " << command_name << ": " << ack_msg.c_str() << endl; 

            Emitter book_keeping; 
            book_keeping << BeginMap; 
	    book_keeping << Key << "MSG_TYPE" << Value << "BOOK_KEEPING";
            book_keeping << Key << "SUB_TYPE" << Value << command_name;
	    book_keeping << Key << "ACK_ID" << Value << ack_id ;
	    book_keeping << Key << "CHECKBOX" << Value << "false"; 
	    book_keeping << Key << "TIME" << Value << get_current_time();
	    book_keeping << Key << "CMD_ID" << Value << to_string(cmdId); 
	    book_keeping << Key << "DEVICE" << Value << device; 
            book_keeping << EndMap; 
	    LOG_DBG << "XXX BOOK_KEEPING: " << book_keeping.c_str() << endl; 

	    publisher->publish_message(consume_q, book_keeping.c_str()); 
	    publisher->publish_message(publish_q, ack_msg.c_str());  
	}
	os_nanoSleep(delay_10ms); 
    } 
    mgr.salShutdown(); 
}  

CommandListener::CommandListener(): IIPBase("L1SystemCfg.yaml", "CommandListener") { 
    Node ocs;
    string user, passwd, publishq, consumeq, base_addr;
    try { 
        ocs = this->config_root["OCS"];
        user = this->credentials->get_user("service_user");
        passwd = this->credentials->get_passwd("service_passwd");
        publishq = ocs["OCS_PUBLISH"].as<string>();
        consumeq = ocs["OCS_CONSUME"].as<string>();
	base_addr = this->config_root["BASE_BROKER_ADDR"].as<string>(); 
    }
    catch (YAML::TypedBadConversion<string>& e) { 
	LOG_CRT << "Cannot read ocs fields from L1SystemCfg.yaml"; 
	exit(-1); 
    }

    string url = this->get_amqp_url(user, passwd, base_addr);
    SimplePublisher *ocs_publisher = new SimplePublisher(url);

    command_args = new ocs_thread_args; 
    command_args->publisher = ocs_publisher;
    command_args->publish_queue = publishq;  
    command_args->consume_queue = consumeq; 
    command_args->ar = SAL_MTArchiver(); 
    command_args->cu = SAL_CatchupArchiver(); 
    command_args->pp = SAL_PromptProcessing(); 
    command_args->atar = SAL_ATArchiver(); 

    setup_archiver_listeners();
    setup_catchuparchiver_listeners(); 
    setup_promptprocessing_listeners();  
    setup_atArchiver_listeners();
    setup_resolve_publisher(); 
    LOG_DBG << "=== dm COMMAND controller ready"; 
} 

CommandListener::~CommandListener(){ 
}

void CommandListener::setup_archiver_listeners() { 
    pthread_t ar_start, ar_enable, ar_disable, ar_standby, ar_enterControl, ar_exitControl, ar_abort; 
    pthread_create(&ar_start, NULL, &CommandListener::run_ar_start, command_args); 
    pthread_create(&ar_enable, NULL, &CommandListener::run_ar_enable, command_args); 
    pthread_create(&ar_disable, NULL, &CommandListener::run_ar_disable, command_args); 
    pthread_create(&ar_standby, NULL, &CommandListener::run_ar_standby, command_args); 
    pthread_create(&ar_enterControl, NULL, &CommandListener::run_ar_enterControl, command_args); 
    pthread_create(&ar_exitControl, NULL, &CommandListener::run_ar_exitControl, command_args); 
    pthread_create(&ar_abort, NULL, &CommandListener::run_ar_abort, command_args); 
    LOG_DBG << "Finished setting up archiver listeners"; 
} 

void CommandListener::setup_catchuparchiver_listeners() { 
    pthread_t cu_start, cu_enable, cu_disable, cu_standby, cu_enterControl, cu_exitControl, cu_abort; 
    pthread_create(&cu_start, NULL, &CommandListener::run_cu_start, command_args); 
    pthread_create(&cu_enable, NULL, &CommandListener::run_cu_enable, command_args); 
    pthread_create(&cu_disable, NULL, &CommandListener::run_cu_disable, command_args); 
    pthread_create(&cu_standby, NULL, &CommandListener::run_cu_standby, command_args); 
    pthread_create(&cu_enterControl, NULL, &CommandListener::run_cu_enterControl, command_args); 
    pthread_create(&cu_exitControl, NULL, &CommandListener::run_cu_exitControl, command_args); 
    pthread_create(&cu_abort, NULL, &CommandListener::run_cu_abort, command_args); 
    LOG_DBG << "Finished setting up catchuparchiver listeners"; 
}

void CommandListener::setup_promptprocessing_listeners() { 
    pthread_t pp_start, pp_enable, pp_disable, pp_standby, pp_enterControl, pp_exitControl, pp_abort; 
    pthread_create(&pp_start, NULL, &CommandListener::run_pp_start, command_args); 
    pthread_create(&pp_enable, NULL, &CommandListener::run_pp_enable, command_args); 
    pthread_create(&pp_disable, NULL, &CommandListener::run_pp_disable, command_args); 
    pthread_create(&pp_standby, NULL, &CommandListener::run_pp_standby, command_args); 
    pthread_create(&pp_enterControl, NULL, &CommandListener::run_pp_enterControl, command_args); 
    pthread_create(&pp_exitControl, NULL, &CommandListener::run_pp_exitControl, command_args); 
    pthread_create(&pp_abort, NULL, &CommandListener::run_pp_abort, command_args); 
    LOG_DBG << "Finished setting up promptprocessing listeners"; 
}

void CommandListener::setup_atArchiver_listeners() { 
    pthread_t atar_start, atar_enable, atar_disable, atar_standby, 
              atar_enterControl, atar_exitControl, atar_abort, atar_resetFromFault; 
    pthread_create(&atar_start, NULL, &CommandListener::run_atar_start, command_args); 
    pthread_create(&atar_enable, NULL, &CommandListener::run_atar_enable, command_args); 
    pthread_create(&atar_disable, NULL, &CommandListener::run_atar_disable, command_args); 
    pthread_create(&atar_standby, NULL, &CommandListener::run_atar_standby, command_args); 
    pthread_create(&atar_enterControl, NULL, &CommandListener::run_atar_enterControl, command_args); 
    pthread_create(&atar_exitControl, NULL, &CommandListener::run_atar_exitControl, command_args); 
    pthread_create(&atar_abort, NULL, &CommandListener::run_atar_abort, command_args); 
    pthread_create(&atar_resetFromFault, NULL, &CommandListener::run_atar_resetFromFault, command_args); 
    LOG_DBG << "Finished setting up atArchiver listeners"; 
} 

void *CommandListener::run_ar_start(void *pargs) {
    ocs_thread_args *params = ((ocs_thread_args *)pargs); 
    SimplePublisher* rabbit_publisher = params->publisher; 
    string publish_q = params->publish_queue; 
    string consume_q = params->consume_queue; 
    SAL_MTArchiver ar = params->ar; 
    
    funcptr<SAL_MTArchiver, MTArchiver_command_startC> ar_start = &SAL_MTArchiver::acceptCommand_start;   
    listenCommand_start(ar, "AR", "START", rabbit_publisher, publish_q, consume_q, ar_start);  
    LOG_DBG << "Finished setting up archiver START command thread"; 
    return 0; 
} 

void *CommandListener::run_ar_enable(void *pargs) {
    ocs_thread_args *params = ((ocs_thread_args *)pargs); 
    SimplePublisher* rabbit_publisher = params->publisher; 
    string publish_q = params->publish_queue; 
    string consume_q = params->consume_queue; 
    SAL_MTArchiver ar = params->ar; 
    
    funcptr<SAL_MTArchiver, MTArchiver_command_enableC> ar_enable = &SAL_MTArchiver::acceptCommand_enable;   
    listenCommand(ar,"AR", "ENABLE", rabbit_publisher, publish_q, consume_q, ar_enable);  
    return 0; 
} 

void *CommandListener::run_ar_disable(void *pargs) {
    ocs_thread_args *params = ((ocs_thread_args *)pargs); 
    SimplePublisher* rabbit_publisher = params->publisher; 
    string publish_q = params->publish_queue; 
    string consume_q = params->consume_queue; 
    SAL_MTArchiver ar = params->ar; 
    
    funcptr<SAL_MTArchiver, MTArchiver_command_disableC> ar_disable = &SAL_MTArchiver::acceptCommand_disable;   
    listenCommand(ar,"AR", "DISABLE", rabbit_publisher, publish_q, consume_q, ar_disable);  
    return 0; 
} 

void *CommandListener::run_ar_standby(void *pargs) {
    ocs_thread_args *params = ((ocs_thread_args *)pargs); 
    SimplePublisher* rabbit_publisher = params->publisher; 
    string publish_q = params->publish_queue; 
    string consume_q = params->consume_queue; 
    SAL_MTArchiver ar = params->ar; 
    
    funcptr<SAL_MTArchiver, MTArchiver_command_standbyC> ar_standby = &SAL_MTArchiver::acceptCommand_standby;   
    listenCommand(ar,"AR", "STANDBY", rabbit_publisher, publish_q, consume_q, ar_standby);  
    return 0; 
} 

void *CommandListener::run_ar_enterControl(void *pargs) {
    ocs_thread_args *params = ((ocs_thread_args *)pargs); 
    SimplePublisher* rabbit_publisher = params->publisher; 
    string publish_q = params->publish_queue; 
    string consume_q = params->consume_queue; 
    SAL_MTArchiver ar = params->ar; 
    
    funcptr<SAL_MTArchiver, MTArchiver_command_enterControlC> ar_enterControl = &SAL_MTArchiver::acceptCommand_enterControl;   
    listenCommand(ar,"AR", "ENTER_CONTROL", rabbit_publisher, publish_q, consume_q, ar_enterControl);  
    return 0; 
} 

void *CommandListener::run_ar_exitControl(void *pargs) {
    ocs_thread_args *params = ((ocs_thread_args *)pargs); 
    SimplePublisher* rabbit_publisher = params->publisher; 
    string publish_q = params->publish_queue; 
    string consume_q = params->consume_queue; 
    SAL_MTArchiver ar = params->ar; 
    
    funcptr<SAL_MTArchiver, MTArchiver_command_exitControlC> ar_exitControl = &SAL_MTArchiver::acceptCommand_exitControl;   
    listenCommand(ar,"AR", "EXIT_CONTROL", rabbit_publisher, publish_q, consume_q, ar_exitControl);  
    return 0; 
} 

void *CommandListener::run_ar_abort(void *pargs) {
    ocs_thread_args *params = ((ocs_thread_args *)pargs); 
    SimplePublisher* rabbit_publisher = params->publisher; 
    string publish_q = params->publish_queue; 
    string consume_q = params->consume_queue; 
    SAL_MTArchiver ar = params->ar; 
    
    funcptr<SAL_MTArchiver, MTArchiver_command_abortC> ar_abort = &SAL_MTArchiver::acceptCommand_abort;   
    listenCommand(ar,"AR", "ABORT", rabbit_publisher, publish_q, consume_q, ar_abort);  
    return 0; 
} 

void *CommandListener::run_cu_start(void *pargs) {
    ocs_thread_args *params = ((ocs_thread_args *)pargs); 
    SimplePublisher* rabbit_publisher = params->publisher; 
    string publish_q = params->publish_queue; 
    string consume_q = params->consume_queue; 
    SAL_CatchupArchiver cu = params->cu; 
    
    funcptr<SAL_CatchupArchiver, CatchupArchiver_command_startC> cu_start = &SAL_CatchupArchiver::acceptCommand_start;   
    listenCommand_start(cu, "CU", "START", rabbit_publisher, publish_q, consume_q, cu_start);  
    return 0; 
} 

void *CommandListener::run_cu_enable(void *pargs) {
    ocs_thread_args *params = ((ocs_thread_args *)pargs); 
    SimplePublisher* rabbit_publisher = params->publisher; 
    string publish_q = params->publish_queue; 
    string consume_q = params->consume_queue; 
    SAL_CatchupArchiver cu = params->cu; 
    
    funcptr<SAL_CatchupArchiver, CatchupArchiver_command_enableC> cu_enable = &SAL_CatchupArchiver::acceptCommand_enable;   
    listenCommand(cu, "CU", "ENABLE", rabbit_publisher, publish_q, consume_q, cu_enable);  
    return 0; 
} 

void *CommandListener::run_cu_disable(void *pargs) {
    ocs_thread_args *params = ((ocs_thread_args *)pargs); 
    SimplePublisher* rabbit_publisher = params->publisher; 
    string publish_q = params->publish_queue; 
    string consume_q = params->consume_queue; 
    SAL_CatchupArchiver cu = params->cu; 
    
    funcptr<SAL_CatchupArchiver, CatchupArchiver_command_disableC> cu_disable = &SAL_CatchupArchiver::acceptCommand_disable;   
    listenCommand(cu, "CU", "DISABLE", rabbit_publisher, publish_q, consume_q, cu_disable);  
    return 0; 
} 

void *CommandListener::run_cu_standby(void *pargs) {
    ocs_thread_args *params = ((ocs_thread_args *)pargs); 
    SimplePublisher* rabbit_publisher = params->publisher; 
    string publish_q = params->publish_queue; 
    string consume_q = params->consume_queue; 
    SAL_CatchupArchiver cu = params->cu; 
    
    funcptr<SAL_CatchupArchiver, CatchupArchiver_command_standbyC> cu_standby = &SAL_CatchupArchiver::acceptCommand_standby;   
    listenCommand(cu, "CU", "STANDBY", rabbit_publisher, publish_q, consume_q, cu_standby);  
    return 0; 
} 

void *CommandListener::run_cu_enterControl(void *pargs) {
    ocs_thread_args *params = ((ocs_thread_args *)pargs); 
    SimplePublisher* rabbit_publisher = params->publisher; 
    string publish_q = params->publish_queue; 
    string consume_q = params->consume_queue; 
    SAL_CatchupArchiver cu = params->cu; 
    
    funcptr<SAL_CatchupArchiver, CatchupArchiver_command_enterControlC> cu_enterControl = &SAL_CatchupArchiver::acceptCommand_enterControl;   
    listenCommand(cu, "CU", "ENTER_CONTROL", rabbit_publisher, publish_q, consume_q, cu_enterControl);  
    return 0; 
} 

void *CommandListener::run_cu_exitControl(void *pargs) {
    ocs_thread_args *params = ((ocs_thread_args *)pargs); 
    SimplePublisher* rabbit_publisher = params->publisher; 
    string publish_q = params->publish_queue; 
    string consume_q = params->consume_queue; 
    SAL_CatchupArchiver cu = params->cu; 
    
    funcptr<SAL_CatchupArchiver, CatchupArchiver_command_exitControlC> cu_exitControl = &SAL_CatchupArchiver::acceptCommand_exitControl;   
    listenCommand(cu, "CU", "EXIT_CONTROL", rabbit_publisher, publish_q, consume_q, cu_exitControl);  
    return 0; 
} 

void *CommandListener::run_cu_abort(void *pargs) {
    ocs_thread_args *params = ((ocs_thread_args *)pargs); 
    SimplePublisher* rabbit_publisher = params->publisher; 
    string publish_q = params->publish_queue; 
    string consume_q = params->consume_queue; 
    SAL_CatchupArchiver cu = params->cu; 
    
    funcptr<SAL_CatchupArchiver, CatchupArchiver_command_abortC> cu_abort = &SAL_CatchupArchiver::acceptCommand_abort;   
    listenCommand(cu, "CU", "ABORT", rabbit_publisher, publish_q, consume_q, cu_abort);  
    return 0; 
} 

void *CommandListener::run_pp_start(void *pargs) {
    ocs_thread_args *params = ((ocs_thread_args *)pargs); 
    SimplePublisher* rabbit_publisher = params->publisher; 
    string publish_q = params->publish_queue; 
    string consume_q = params->consume_queue; 
    SAL_PromptProcessing pp = params->pp; 
    
    funcptr<SAL_PromptProcessing, PromptProcessing_command_startC> pp_start = &SAL_PromptProcessing::acceptCommand_start;   
    listenCommand_start(pp, "PP", "START", rabbit_publisher, publish_q, consume_q, pp_start);  
    return 0; 
} 

void *CommandListener::run_pp_enable(void *pargs) {
    ocs_thread_args *params = ((ocs_thread_args *)pargs); 
    SimplePublisher* rabbit_publisher = params->publisher; 
    string publish_q = params->publish_queue; 
    string consume_q = params->consume_queue; 
    SAL_PromptProcessing pp = params->pp; 
    
    funcptr<SAL_PromptProcessing, PromptProcessing_command_enableC> pp_enable = &SAL_PromptProcessing::acceptCommand_enable;   
    listenCommand(pp, "PP", "ENABLE", rabbit_publisher, publish_q, consume_q, pp_enable);  
    return 0; 
} 

void *CommandListener::run_pp_disable(void *pargs) {
    ocs_thread_args *params = ((ocs_thread_args *)pargs); 
    SimplePublisher* rabbit_publisher = params->publisher; 
    string publish_q = params->publish_queue; 
    string consume_q = params->consume_queue; 
    SAL_PromptProcessing pp = params->pp; 
    
    funcptr<SAL_PromptProcessing, PromptProcessing_command_disableC> pp_disable = &SAL_PromptProcessing::acceptCommand_disable;   
    listenCommand(pp, "PP", "DISABLE", rabbit_publisher, publish_q, consume_q, pp_disable);  
    return 0; 
} 

void *CommandListener::run_pp_standby(void *pargs) {
    ocs_thread_args *params = ((ocs_thread_args *)pargs); 
    SimplePublisher* rabbit_publisher = params->publisher; 
    string publish_q = params->publish_queue; 
    string consume_q = params->consume_queue; 
    SAL_PromptProcessing pp = params->pp; 
    
    funcptr<SAL_PromptProcessing, PromptProcessing_command_standbyC> pp_standby = &SAL_PromptProcessing::acceptCommand_standby;   
    listenCommand(pp, "PP", "STANDBY", rabbit_publisher, publish_q, consume_q, pp_standby);  
    return 0; 
} 

void *CommandListener::run_pp_enterControl(void *pargs) {
    ocs_thread_args *params = ((ocs_thread_args *)pargs); 
    SimplePublisher* rabbit_publisher = params->publisher; 
    string publish_q = params->publish_queue; 
    string consume_q = params->consume_queue; 
    SAL_PromptProcessing pp = params->pp; 
    
    funcptr<SAL_PromptProcessing, PromptProcessing_command_enterControlC> pp_enterControl = &SAL_PromptProcessing::acceptCommand_enterControl;   
    listenCommand(pp, "PP", "ENTER_CONTROL", rabbit_publisher, publish_q, consume_q, pp_enterControl);  
    return 0; 
} 

void *CommandListener::run_pp_exitControl(void *pargs) {
    ocs_thread_args *params = ((ocs_thread_args *)pargs); 
    SimplePublisher* rabbit_publisher = params->publisher; 
    string publish_q = params->publish_queue; 
    string consume_q = params->consume_queue; 
    SAL_PromptProcessing pp = params->pp; 
    
    funcptr<SAL_PromptProcessing, PromptProcessing_command_exitControlC> pp_exitControl = &SAL_PromptProcessing::acceptCommand_exitControl;   
    listenCommand(pp, "PP", "EXIT_CONTROL", rabbit_publisher, publish_q, consume_q, pp_exitControl);  
    return 0; 
} 

void *CommandListener::run_pp_abort(void *pargs) {
    ocs_thread_args *params = ((ocs_thread_args *)pargs); 
    SimplePublisher* rabbit_publisher = params->publisher; 
    string publish_q = params->publish_queue; 
    string consume_q = params->consume_queue; 
    SAL_PromptProcessing pp = params->pp; 
    
    funcptr<SAL_PromptProcessing, PromptProcessing_command_abortC> pp_abort = &SAL_PromptProcessing::acceptCommand_abort;   
    listenCommand(pp, "PP", "ABORT", rabbit_publisher, publish_q, consume_q, pp_abort);  
    return 0; 
} 

void *CommandListener::run_atar_start(void *pargs) {
    ocs_thread_args *params = ((ocs_thread_args *)pargs); 
    SimplePublisher* rabbit_publisher = params->publisher; 
    string publish_q = params->publish_queue; 
    string consume_q = params->consume_queue; 
    SAL_ATArchiver atar = params->atar; 
    
    funcptr<SAL_ATArchiver, ATArchiver_command_startC> atar_start = &SAL_ATArchiver::acceptCommand_start;   
    listenCommand_start(atar, "AT", "START", rabbit_publisher, publish_q, consume_q, atar_start);  
    return 0; 
} 

void *CommandListener::run_atar_enable(void *pargs) {
    ocs_thread_args *params = ((ocs_thread_args *)pargs); 
    SimplePublisher* rabbit_publisher = params->publisher; 
    string publish_q = params->publish_queue; 
    string consume_q = params->consume_queue; 
    SAL_ATArchiver atar = params->atar; 
    
    funcptr<SAL_ATArchiver, ATArchiver_command_enableC> atar_enable = &SAL_ATArchiver::acceptCommand_enable;   
    listenCommand(atar,"AT", "ENABLE", rabbit_publisher, publish_q, consume_q, atar_enable);  
    return 0; 
} 

void *CommandListener::run_atar_disable(void *pargs) {
    ocs_thread_args *params = ((ocs_thread_args *)pargs); 
    SimplePublisher* rabbit_publisher = params->publisher; 
    string publish_q = params->publish_queue; 
    string consume_q = params->consume_queue; 
    SAL_ATArchiver atar = params->atar; 
    
    funcptr<SAL_ATArchiver, ATArchiver_command_disableC> atar_disable = &SAL_ATArchiver::acceptCommand_disable;   
    listenCommand(atar,"AT", "DISABLE", rabbit_publisher, publish_q, consume_q, atar_disable);  
    return 0; 
} 

void *CommandListener::run_atar_standby(void *pargs) {
    ocs_thread_args *params = ((ocs_thread_args *)pargs); 
    SimplePublisher* rabbit_publisher = params->publisher; 
    string publish_q = params->publish_queue; 
    string consume_q = params->consume_queue; 
    SAL_ATArchiver atar = params->atar; 
    
    funcptr<SAL_ATArchiver, ATArchiver_command_standbyC> atar_standby = &SAL_ATArchiver::acceptCommand_standby;   
    listenCommand(atar,"AT", "STANDBY", rabbit_publisher, publish_q, consume_q, atar_standby);  
    return 0; 
} 

void *CommandListener::run_atar_enterControl(void *pargs) {
    ocs_thread_args *params = ((ocs_thread_args *)pargs); 
    SimplePublisher* rabbit_publisher = params->publisher; 
    string publish_q = params->publish_queue; 
    string consume_q = params->consume_queue; 
    SAL_ATArchiver atar = params->atar; 
    
    funcptr<SAL_ATArchiver, ATArchiver_command_enterControlC> atar_enterControl = &SAL_ATArchiver::acceptCommand_enterControl;   
    listenCommand(atar,"AT", "ENTER_CONTROL", rabbit_publisher, publish_q, consume_q, atar_enterControl);  
    return 0; 
} 

void *CommandListener::run_atar_exitControl(void *pargs) {
    ocs_thread_args *params = ((ocs_thread_args *)pargs); 
    SimplePublisher* rabbit_publisher = params->publisher; 
    string publish_q = params->publish_queue; 
    string consume_q = params->consume_queue; 
    SAL_ATArchiver atar = params->atar; 
    
    funcptr<SAL_ATArchiver, ATArchiver_command_exitControlC> atar_exitControl = &SAL_ATArchiver::acceptCommand_exitControl;   
    listenCommand(atar,"AT", "EXIT_CONTROL", rabbit_publisher, publish_q, consume_q, atar_exitControl);  
    return 0; 
} 

void *CommandListener::run_atar_abort(void *pargs) {
    ocs_thread_args *params = ((ocs_thread_args *)pargs); 
    SimplePublisher* rabbit_publisher = params->publisher; 
    string publish_q = params->publish_queue; 
    string consume_q = params->consume_queue; 
    SAL_ATArchiver atar = params->atar; 
    
    funcptr<SAL_ATArchiver, ATArchiver_command_abortC> atar_abort = &SAL_ATArchiver::acceptCommand_abort;   
    listenCommand(atar, "AT", "ABORT", rabbit_publisher, publish_q, consume_q, atar_abort);  
    return 0; 
} 

void *CommandListener::run_atar_resetFromFault(void *pargs) {
    ocs_thread_args *params = ((ocs_thread_args *)pargs); 
    SimplePublisher* rabbit_publisher = params->publisher; 
    string publish_q = params->publish_queue; 
    string consume_q = params->consume_queue; 
    SAL_ATArchiver atar = params->atar; 
    
    funcptr<SAL_ATArchiver, ATArchiver_command_resetFromFaultC> atar_resetFromFault = &SAL_ATArchiver::acceptCommand_resetFromFault;   
    listenCommand(atar, "AT", "RESET_FROM_FAULT", rabbit_publisher, publish_q, consume_q, atar_resetFromFault);  
    return 0; 
} 

void CommandListener::setup_resolve_publisher() { 
    LOG_DBG << "Setting up OCS RESOLVE publisher" << endl; 
    pthread_t resolvethread; 
    pthread_create(&resolvethread, NULL, &CommandListener::run_resolve_publisher, command_args); 
} 

void *CommandListener::run_resolve_publisher(void *pargs) { 
    usleep(10000000); 
    ocs_thread_args *params = ((ocs_thread_args *)pargs); 
    SimplePublisher *rabbit_publisher = params->publisher; 
    string consume_q = params->consume_queue; 
    while (1) { 
	rabbit_publisher->publish_message(consume_q, "{MSG_TYPE: RESOLVE_ACK}"); 
	usleep(100000000);
    }  
    return 0; 
} 

string CommandListener::get_next_timed_ack_id(string ack_type) { 
    next_timed_ack_id = next_timed_ack_id + 1; 
    string ack = to_string(next_timed_ack_id); 
    string formatted_ack = string(6 - ack.length(), '0') + ack; 
    string retval = ack_type + "_" + formatted_ack; 
    return retval; 
}

string CommandListener::get_device(string name) { 
    string device; 
    if (name == "AR") device = "MTArchiver"; 
    else if (name == "CU") device = "CatchupArchiver"; 
    else if (name == "PP") device = "PromptProcessing"; 
    else if (name == "AT") device = "ATArchiver"; 
    return device; 
} 

int main() { 
    CommandListener cmd; 
    while (1) { 
    
        sleep(5);
    } 
    return 0; 
} 
