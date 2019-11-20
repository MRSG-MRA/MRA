#ifndef TASK_MRA_CODE
#define TASK_MRA_CODE


#include "task_mra.hpp"


Task_MRA::Task_MRA(std::string task_name, double comp_size, double comm_size, void* task_data){
    name = task_name;
    computation_size = comp_size;
    communication_size = comm_size;
    data = task_data;
    sender = nullptr;
    receiver = nullptr;
    source = nullptr;
    execution = nullptr;
    execution_status = NOT_STARTED;
}


std::string Task_MRA::getName(){
    return name;
}

void Task_MRA::setName(std::string task_name){
    name = task_name;
}

void* Task_MRA::getData(){
    return data;
}
    
void Task_MRA::setData(void* task_data){
    data = task_data;
}

simgrid::s4u::ActorPtr Task_MRA::getSender(){
    return sender;
}

void Task_MRA::setSender(simgrid::s4u::ActorPtr task_sender){
    sender = task_sender;
}

simgrid::s4u::ActorPtr Task_MRA::getReceiver(){
    return receiver;
}
void Task_MRA::setReceiver(simgrid::s4u::ActorPtr task_receiver){
    receiver = task_receiver;
}

simgrid::s4u::Host* Task_MRA::getSource(){
    return source;
}
void Task_MRA::setSource(simgrid::s4u::Host* task_source){
    source = task_source;
}

double Task_MRA::getFlopsAmount(){
    return computation_size;
}

/*
*Returns the size of the data attached to a task
*OBS: Talvez necessário mudar o nome de communication_size
*/
double Task_MRA::getBytesAmount(){
    return communication_size;
}

double Task_MRA::getRemainingRatio(){
    double remaining = 100.0;

    if(execution_status == EXECUTING)
      remaining = execution->get_remaining_ratio();  
    else if(execution_status == FINISHED)
        remaining = 0.0;

    return remaining;
}

double Task_MRA::getRemainingAmount(){
    double remaining = computation_size;

    if(execution_status == EXECUTING)
      remaining = execution->get_remaining();  
    else if(execution_status == FINISHED)
        remaining = 0.0;

    return remaining;
}


void Task_MRA::execute(){
    exec_actor_pid = simgrid::s4u::this_actor::get_pid();
    execution = simgrid::s4u::this_actor::exec_init(computation_size);
    execution->start();

    execution_status = EXECUTING;
    execution->wait();
    execution_status = FINISHED;
}

void Task_MRA::destroy(){
    if(execution_status == EXECUTING) {
        //execution->cancel();
        simgrid::s4u::ActorPtr exec_actor = simgrid::s4u::Actor::by_pid(exec_actor_pid);
        exec_actor->kill();
    }
    
    /**
     * As soon as this method ends, the unique_ptr gets out of scope and its contents are freed,
     * this way it won't matter where the class was instantiated
    **/
    std::unique_ptr<Task_MRA> pointer(this);
}


#endif 