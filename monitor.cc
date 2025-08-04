#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <CAENHVWrapper.h>
#include <chrono>
#include <thread>
#include <csignal>
#include <iomanip>
#include <ctime>

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

std::string get_filename() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    
    std::tm tm = *std::localtime(&t);
    std::stringstream filename;
    filename << "data/aTPC_HV_" 
             << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".txt";
    return filename.str();
}

void get_channel_names(ushort slot, ushort numChannels, ushort channels[], std::string channel_names[]) {
    char chNameList[numChannels][MAX_CH_NAME];  
    CAENHVRESULT ret = CAENHV_GetChName(handle, slot, numChannels, channels, chNameList);

    if (ret != CAENHV_OK) {
        std::cerr << "Error reading channel names: " << CAENHV_GetError(handle) << std::endl;
        return;
    }

    for (ushort i = 0; i < numChannels; ++i) {
        channel_names[i] = std::string(chNameList[i]);
    }
}

std::string status_to_string(unsigned int status) {
    std::stringstream status_str;
    if (status & 0x01) status_str << "ON ";
    if (status & 0x02) status_str << "Ramp U ";
    if (status & 0x04) status_str << "Ramp D ";
    if (status & 0x08) status_str << "OvC ";
    if (status & 0x10) status_str << "OvV ";
    if (status & 0x20) status_str << "UdV ";
    if (status & 0x40) status_str << "Ext_Trip ";
    if (status & 0x80) status_str << "MaxV ";
    if (status & 0x100) status_str << "Ext_Disable ";
    if (status & 0x200) status_str << "Int_Trip ";
    if (status & 0x400) status_str << "Cal._Err ";
    if (status & 0x800) status_str << "Unplug ";
    if (status & 0x1000) status_str << "OvV_Protection ";
    if (status & 0x2000) status_str << "PW_Fail ";
    if (status & 0x4000) status_str << "Temp_Err ";
    
    std::string result = status_str.str();
    return result;
}

int main() {
    void* arg = (void*)IP_ADDRESS;  

    signal(SIGINT, handle_signal);
    signal(SIGTSTP, handle_signal);

    CAENHVRESULT ret = CAENHV_InitSystem(SYS_TYPE, LINK_TYPE, arg, USERNAME, PASSWORD, &handle);
    if (ret != CAENHV_OK) {
        std::cerr << "Failed to connect to HV system: " << CAENHV_GetError(handle) << std::endl;
        return 1;
    }

    std::cout << "Connected to SY4527LC at " << IP_ADDRESS << std::endl;

    ushort slot = SLOT;  
    ushort numChannels = 12;

    ushort channels[numChannels];
    for (ushort i = 0; i < numChannels; ++i) {
        channels[i] = i;  
    }


    std::string channel_names[numChannels];
    get_channel_names(slot, numChannels, channels, channel_names);

    std::ofstream outfile(get_filename(), std::ios::app); 

    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&t);
    outfile << "Date and Time: " << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "\n";


    
    for (ushort i = 0; i < numChannels; ++i) {
        outfile << channel_names[i] << "\t";
    }
    outfile << "\n"; 
    


    while (true) {
        now = std::chrono::system_clock::now();
        t = std::chrono::system_clock::to_time_t(now);
        tm = *std::localtime(&t);
        outfile << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "\n";
	outfile << "Ch.   Vset[V] Vmon[V] Iset[uA] Imon[uA] Status\n";
	float Vset[numChannels];
        float Vmon[numChannels];
	
        ret = CAENHV_GetChParam(handle, slot, "V0Set", numChannels, channels, Vset);
        if (ret != CAENHV_OK) {
            std::cerr << "Error reading V0Set for slot " << slot << ": " 
                      << CAENHV_GetError(handle) << std::endl;
            return 1;  
        }

	ret = CAENHV_GetChParam(handle, slot, "VMon", numChannels, channels, Vmon);
        if (ret != CAENHV_OK) {
            std::cerr << "Error reading VMon for slot " << slot << ": " 
                      << CAENHV_GetError(handle) << std::endl;
            return 1;  
        }

        float Ilimit[numChannels];
	float Imon[numChannels];
        ret = CAENHV_GetChParam(handle, slot, "I0Set", numChannels, channels, Ilimit);
        if (ret != CAENHV_OK) {
            std::cerr << "Error reading Ilimit for slot " << slot << ": " 
                      << CAENHV_GetError(handle) << std::endl;
            return 1;  
        }

	ret = CAENHV_GetChParam(handle, slot, "IMon", numChannels, channels, Imon);
        if (ret != CAENHV_OK) {
            std::cerr << "Error reading IMon for slot " << slot << ": " 
                      << CAENHV_GetError(handle) << std::endl;
            return 1;  
        }
	
	
	
	unsigned int status[numChannels];
	ret = CAENHV_GetChParam(handle, slot, "Status", numChannels, channels, status);
	if (ret != CAENHV_OK) {
            std::cerr << "Error reading Status for slot " << slot << ": " 
                      << CAENHV_GetError(handle) << std::endl;
            return 1;  
        }


        for (ushort i = 0; i < numChannels; ++i) {
	  outfile << i << "\t" << Vset[i] << "\t" << Vmon[i] << "\t" << Ilimit[i] << "\t" << Imon[i] << "\t" << status_to_string(status[i]) << "\n";
        }

        outfile << "\n";
        outfile.flush();

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    CAENHV_DeinitSystem(handle);

    return 0;
}
