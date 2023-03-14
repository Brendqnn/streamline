#pragma once
#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <memory>


class SLqueue {
public:
	SLqueue();
	~SLqueue();

	const char* get_file_info();
	std::string current_file;

	void load_queue();
private:
	const char* filename;

	std::string dir_path;
	std::string temp_file;
	std::vector<std::string> file_queue;
};
