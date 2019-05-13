#pragma once
#include <iostream>
#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <cpprest/http_headers.h>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <filesystem>
#include <optional>
#include <bit7zlibrary.hpp>
#include <bitextractor.hpp>
#include "pugixml.hpp"
#include <winhttp.h>
#include <regex>
class downloader
{
	std::shared_ptr<web::http::client::http_client> client_;
	std::shared_ptr<std::string>file_name_; // name of the downloaded zip file.
	bool start_console_log();
	void download_current_data(std::optional<std::string> url);
	void get_expiration_date();
	void extract_files() const;
	std::optional<std::string> get_current_data_url();
	std::optional<std::string> extract_zip_file_name(const std::string& input);
	void convert_to_x_plane_format() const;
public:
	std::shared_ptr<spdlog::logger> console{};
	void initialize();
	void run();
	void shutdown();
};