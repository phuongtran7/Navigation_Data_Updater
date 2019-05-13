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
		console->info("Start downloading...");
		const auto complete_url = conversions::to_string_t(url.value());
		http_client download_client(complete_url);

		file_name_ = std::make_shared<std::string>(extract_zip_file_name(url.value()).value());

		const auto download = download_client.request(methods::GET)
			.then([=](const http_response & response)
				{
					return response.body();
				})

			.then([=](const istream & is)
				{
					// Get the whole file name and extension
					auto rwbuf = file_buffer<uint8_t>::open(conversions::to_string_t(file_name_->c_str())).get();
					// ReSharper disable once CppExpressionWithoutSideEffects
					is.read_to_end(rwbuf).get();
					rwbuf.close().get();
				});

				try
				{
					// ReSharper disable once CppExpressionWithoutSideEffects
					download.wait();
					console->info("Finished downloading.");
				}
				catch (const std::exception & e)
				{
					console->critical("Error exception: {}", e.what());
				}
	}
	else
	{
		console->critical("Cannot find URL to download.");
	}
}

void downloader::get_expiration_date()
{
	uri_builder builder;
	builder.set_path(U("/cifp/chart?edition=next"));
	const pplx::task<void> get_url = client_->request(methods::GET, builder.to_string())

		// Handle response headers arriving.
		.then([=](const http_response & response)
			{
				if (response.status_code() != status_codes::OK)
				{
					console->critical("Received response status code from Get Expiration querry: {}.", response.status_code());
					throw;
				}

				// Extract JSON out of the response
				return response.extract_utf8string();
			})
		// parse XML
				.then([=](const std::string & xml_data)
					{
						pugi::xml_document doc;
						const pugi::xml_parse_result result = doc.load_string(xml_data.c_str());

						if (!result)
						{
							return;
						}

						// Currently does not know how to switch to JSON response so just use XML for now
						const std::string expiration_date = doc.child("productSet").child("edition").child("editionDate").text().as_string();

						console->warn("The data will expire on: {}", expiration_date);
						console->warn("Please make sure to run this again before then.");
					});

			// Wait for all the outstanding I/O to complete and handle any exceptions
			try
			{
				// ReSharper disable once CppExpressionWithoutSideEffects
				get_url.wait();

			}
			catch (const std::exception & e)
			{
				console->critical("Error exception: {}", e.what());
			}
}

// Extract the download zip file and override the old files
void downloader::extract_files() const
{
	try
	{
		console->info("Start extracting zip file into Output folder.");
		// Extract the zip file
		const bit7z::Bit7zLibrary lib(L"7z.dll");
		const bit7z::BitExtractor extractor(lib, bit7z::BitFormat::Zip);

		// Create temporary folder to store extracted files
		fs::create_directory("Output");

		// Extract and override the current files with whole file name and extension
		extractor.extract(conversions::to_string_t(file_name_->c_str()), L"Output/");

		fs::remove(file_name_->c_str());
	}
	catch (const std::exception & e)
	{
		console->critical("Error extracting file: {}.", e.what());
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
	const auto url = get_current_data_url();
	download_current_data(url);
	extract_files();
	convert_to_x_plane_format();
	get_expiration_date();
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
		.then([=](const http_response & response)
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
				.then([=](const std::string & xml_data)
					{
						pugi::xml_document doc;
						const pugi::xml_parse_result result = doc.load_string(xml_data.c_str());

						if (!result)
						{
							return std::string("");
						}

						// Currently does not know how to switch to JSON response so just use XML for now
						std::string return_url = doc.child("productSet").child("edition").child("product").attribute("url").as_string();

						console->info("Found current CIFP data:");
						console->info(return_url);
						return return_url;

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

std::optional<std::string> downloader::extract_zip_file_name(const std::string& input)
{
	const std::regex expression(R"((CIFP_).*(\.zip))");

	std::smatch match;

	if (std::regex_search(input, match, expression))
	{
		return match[0];
	}
	return std::nullopt;
}

void downloader::convert_to_x_plane_format() const
{
	console->info("Start converting FAACIFP18 to X-Plane format. Please wait.");
	// Guess the current name of the folder inside the zip file as it
	// different from the zip name itself.
	for (auto& p : fs::directory_iterator("Output"))
	{
		std::wstring directory_name;
		directory_name.append(p.path().c_str());
		directory_name.append(U("\\FAACIFP18"));
		if (fs::exists(directory_name))
		{
			// Found the file
			fs::copy_file(directory_name, "Output\\FAACIFP18", fs::copy_options::overwrite_existing);
			// Remove all unessesary files
			fs::remove_all(p.path().c_str());
			// Copy X-Plane tool to Output folder to convert FAACIFP18 file
			try
			{
				fs::copy_file("convert424toxplane11.exe", "Output\\convert424toxplane11.exe", fs::copy_options::overwrite_existing);
			}
			catch (fs::filesystem_error&)
			{
				console->critical("convert424toxplane11.exe not found.");
				fs::remove_all("Output");
				break;
			}
			std::system("cd Output && convert424toxplane11 FAACIFP18 \"CSF\"");
			fs::rename("Output\\FAACIFP18", "Output\\earth_424.dat");
			fs::remove("Output\\convert424toxplane11.exe");
			break;
		}
	}

}
