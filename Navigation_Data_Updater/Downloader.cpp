#include "Downloader.h"

using namespace utility;
using namespace web;
using namespace http;
using namespace client;
using namespace Concurrency::streams;
namespace fs = std::filesystem;

bool downloader::start_console_log()
{
	try
	{
		// Console sink
		auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		console_sink->set_level(spdlog::level::info);
		console_sink->set_pattern("[%^%l%$] %v");

		console = std::make_shared<spdlog::logger>("console_sink", console_sink);
	}
	catch (const spdlog::spdlog_ex & ex)
	{
		std::cout << "Console log init failed: " << ex.what() << std::endl;
		return false;
	}
	return true;
}

void downloader::download_current_data(std::optional<std::string> url)
{
	if (url.has_value())
	{
		const auto complete_url = conversions::to_string_t(url.value());
		http_client download_client(complete_url);

		const auto download = download_client.request(methods::GET)
			.then([=](const http_response& response)
				{
					return response.body();
				})

			.then([=](const istream& is)
				{
					auto rwbuf = file_buffer<uint8_t>::open(U("data.zip")).get();
					is.read_to_end(rwbuf).get();
					rwbuf.close().get();
				});

				try
				{
					download.wait();
				}
				catch (const std::exception & e)
				{
					console->critical("Error exception: {}", e.what());
				}
	}
}

// Extract the download zip file and override the old files
void downloader::extract_files() const
{
	try
	{
		// Extract the zip file
		const bit7z::Bit7zLibrary lib(L"7z.dll");
		const bit7z::BitExtractor extractor(lib, bit7z::BitFormat::Zip);

		// Create temporary folder to store extracted files
		fs::create_directory("Temp");

		// Extract and override the current files
		extractor.extract(L"data.zip", L"Temp/");

		fs::remove("Update.zip");
	}
	catch (const std::exception & e)
	{
		std::cout << "Error: " << e.what() << "\n";
	}
}

void downloader::initialize()
{
	start_console_log();
	// Due to the fact that current cpprestsdk does not support client side certificate
	// thus this is nessessary to bypass the error
	// More info: https://github.com/Microsoft/cpprestsdk/issues/642
	http_client_config config;
	config.set_nativehandle_options([](web::http::client::native_handle handle) {
		WinHttpSetOption(handle, WINHTTP_OPTION_CLIENT_CERT_CONTEXT, WINHTTP_NO_CLIENT_CERT_CONTEXT, 0);
	});
	client_ = std::make_shared<http_client>(U("https://soa.smext.faa.gov/apra"), config);
}

void downloader::run()
{
	get_current_data_url();
}

void downloader::shutdown()
{
	console->info("++++++++++++++++++++++++++++++++++++++++++++");
	console->info("+ Completed. Please press ENTER to exit. +");
	console->info("++++++++++++++++++++++++++++++++++++++++++++");

	// Release resource;
	console.reset();
	// Clean up
	spdlog::drop_all();
}

std::optional<std::string> downloader::get_current_data_url()
{
	uri_builder builder;
	builder.set_path(U("/cifp/chart?edition=current"));
	const pplx::task<std::string> get_url = client_->request(methods::GET, builder.to_string())

		// Handle response headers arriving.
		.then([=](const http_response& response)
			{
				if (response.status_code() != status_codes::OK)
				{
					console->critical("Received response status code from Get Latest Release querry: {}.", response.status_code());
					throw;
				}

				// Extract JSON out of the response
				return response.extract_utf8string();
			})
		// parse XML
				.then([=](const std::string& xml_data)
					{
						pugi::xml_document doc;
						const pugi::xml_parse_result result = doc.load_string(xml_data.c_str());

						if (!result)
						{
							return std::string("");
						}

						return std::string("");
						
					});

			// Wait for all the outstanding I/O to complete and handle any exceptions
			try
			{
				// ReSharper disable once CppExpressionWithoutSideEffects
				get_url.wait();
				auto return_val = get_url.get();

				if (return_val.empty())
				{
					return std::nullopt;
				}
				return return_val;
			}
			catch (const std::exception & e)
			{
				console->critical("Error exception: {}", e.what());
				return std::nullopt;
			}
}
