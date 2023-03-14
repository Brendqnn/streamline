#include "sl_queue.h"
#include <iostream>


SLqueue::SLqueue() 
	: filename(filename)
    , temp_file(temp_file)
	, dir_path(dir_path)
    , file_queue(file_queue)
{
	
}

SLqueue::~SLqueue() {

}
const char* SLqueue::get_file_info() {
    dir_path = "res/";
    for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
        if (entry.is_regular_file()) {
            temp_file = entry.path().filename().string();
            file_queue.push_back(temp_file);
        }
    }
    current_file = dir_path + file_queue.front();
    return filename = current_file.c_str();
}

void SLqueue::load_queue() {
    while (!file_queue.empty()) {
        /*if (_compressor->compress_state == true) {
            std::cout << "processing file..." << std::endl;
            file_queue.erase(file_queue.begin());
            if (!file_queue.empty()) {
                current_file = file_queue.front();
            }
        }*/
    }
}



