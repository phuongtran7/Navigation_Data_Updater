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
class downloader
{
	std::shared_ptr<web::http::client::http_client> client_;
	bool start_console_log();
	void download_current_data(std::optional<std::string> url);
	void extract_files() const;
	std::optional<std::string> get_current_data_url();
public:
	std::shared_ptr<spdlog::logger> console{};
	void initialize();
	void run();
	void shutdown();
};