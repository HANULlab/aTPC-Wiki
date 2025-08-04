#include <iostream>
#include <cstdlib>
#include <CAENHVWrapper.h>
#include <string>
#include <sstream>

#define SYS_TYPE CAENHV_SYSTEM_TYPE_t::SY4527  
#define LINK_TYPE LINKTYPE_TCPIP               
#define USERNAME "admin"                       
#define PASSWORD "admin"                       
#define IP_ADDRESS "192.168.0.18"
#define SLOT 1

int handle;  


void handle_signal(int signal) {
    std::cerr << "Signal " << signal << " received. Cleaning up and exiting..." << std::endl;
    CAENHV_DeinitSystem(handle);
    exit(0);
}


void set_voltage(ushort slot, ushort channel, float voltage) {
    CAENHVRESULT ret;
    ret = CAENHV_SetChParam(handle, slot, "V0Set", 1, &channel, &voltage);
    if (ret != CAENHV_OK) {
        std::cerr << "Error setting voltage for Channel " << channel << ": " 
                  << CAENHV_GetError(handle) << std::endl;
    } else {
        std::cout << "Channel " << channel << " voltage set to " << voltage << "V." << std::endl;
    }
}


void turn_off_channel(ushort slot, ushort channel) {
    CAENHVRESULT ret;
    int pw_off = 0;  
    ret = CAENHV_SetChParam(handle, slot, "Pw", 1, &channel, &pw_off);
    if (ret != CAENHV_OK) {
        std::cerr << "Error turning off Channel " << channel << ": " 
                  << CAENHV_GetError(handle) << std::endl;
    } else {
        std::cout << "Channel " << channel << " powered off." << std::endl;
    }
}


void turn_on_channel(ushort slot, ushort channel) {
    CAENHVRESULT ret;
    int pw_on = 1;  
    ret = CAENHV_SetChParam(handle, slot, "Pw", 1, &channel, &pw_on);
    if (ret != CAENHV_OK) {
        std::cerr << "Error powering on Channel " << channel << ": " 
                  << CAENHV_GetError(handle) << std::endl;
    } else {
        std::cout << "Channel " << channel << " powered on." << std::endl;
    }
}


bool check_channel_status(ushort slot, ushort channel) {
    unsigned int status;
    CAENHVRESULT ret = CAENHV_GetChParam(handle, slot, "Status", 1, &channel, &status);
    if (ret != CAENHV_OK) {
        std::cerr << "Error reading status for Channel " << channel << ": " 
                  << CAENHV_GetError(handle) << std::endl;
        return false;
    }


    if (status & 0x01) {
        return true;
    }
    return false;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <channel_number> <voltage/off/on>" << std::endl;
        return 1;
    }

    ushort slot = SLOT;  
    ushort channel = std::stoi(argv[1]); 
    std::string command = argv[2];  

    void* arg = (void*)IP_ADDRESS;  
    CAENHVRESULT ret = CAENHV_InitSystem(SYS_TYPE, LINK_TYPE, arg, USERNAME, PASSWORD, &handle);
    if (ret != CAENHV_OK) {
        std::cerr << "Failed to connect to HV system: " << CAENHV_GetError(handle) << std::endl;
        return 1;
    }

    if (command == "off") {
        turn_off_channel(slot, channel);
    } else if (command == "on") {
        turn_on_channel(slot, channel);
    } else if (command == "kill" || command == "trip") {
        turn_off_channel(slot, channel); 
    } else {
        float voltage = std::stof(command);  
        set_voltage(slot, channel, voltage);
    }

    CAENHV_DeinitSystem(handle);

    return 0;
}
