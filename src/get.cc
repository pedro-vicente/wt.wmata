#include <iostream>
#include <sstream>
#include <fstream>
#include "ssl_read.hh"

/////////////////////////////////////////////////////////////////////////////////////////////////////
// extract_value
/////////////////////////////////////////////////////////////////////////////////////////////////////

std::string extract_value(const std::string& content, const std::string& key)
{
  size_t pos_key = content.find("\"" + key + "\"");
  size_t pos_colon = content.find(":", pos_key);
  size_t first = content.find("\"", pos_colon);
  size_t second = content.find("\"", first + 1);
  return content.substr(first + 1, second - first - 1);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// get_station_list
// GET https://api.wmata.com/Rail.svc/json/jStations?LineCode=line_code HTTP/1.1
// Cache-Control: no-cache
// api_key: MY_KEY
//
// Parameters:
//   api_key   - Your WMATA API key
//   line_code - Metro line color code 
//
// Available Line Codes:
//   RD - Red Line
//   OR - Orange Line
//   SV - Silver Line
//   BL - Blue Line
//   YL - Yellow Line
//   GR - Green Line
//
// Example Usage:
//   get_station_list(my_key, "RD");
/////////////////////////////////////////////////////////////////////////////////////////////////////

int get_station_list(const std::string& api_key, const std::string& line_code)
{
	const std::string host = "api.wmata.com";
	const std::string port_num = "443";
	std::stringstream http;
	http << "GET /Rail.svc/json/jStations?LineCode=" << line_code << " HTTP/1.1\r\n";
	http << "Host: " << host << "\r\n";
	http << "Accept: application/json\r\n";
	http << "Cache-Control: no-cache\r\n";
	http << "api_key: " << api_key << "\r\n";
	http << "Connection: close\r\n\r\n";
	std::cout << http.str() << std::endl;

	std::string json;
	ssl_read(host, port_num, http.str(), json);

	if (!json.size())
	{
		return -1;
	}

	std::cout << json.c_str() << std::endl;
	std::ofstream ofs("stations_" + line_code + ".json");
	ofs << json;
	ofs.close();

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// get_gtfs_rail
// Download WMATA Rail GTFS static data ZIP file
// GET https://api.wmata.com/gtfs/rail-gtfs-static.zip HTTP/1.1
// Cache-Control: no-cache
// api_key: MY_KEY
// Output:
//   wmata_rail_gtfs.zip - GTFS data archive containing:
//     - shapes.txt (detailed track coordinates)
//     - stops.txt (station locations)
//     - routes.txt (line information)
//     - trips.txt (trip details)
//     - stop_times.txt (schedules)
/////////////////////////////////////////////////////////////////////////////////////////////////////

int get_gtfs_rail(const std::string& api_key)
{
	const std::string host = "api.wmata.com";
	const std::string port_num = "443";
	const std::string path = "/gtfs/rail-gtfs-static.zip";

	std::stringstream http;
	http << "GET " << path << " HTTP/1.1\r\n";
	http << "Host: " << host << "\r\n";
	http << "Accept: application/zip, */*\r\n";
	http << "Cache-Control: no-cache\r\n";
	http << "api_key: " << api_key << "\r\n";
	http << "Connection: close\r\n\r\n";

	std::cout << http.str() << std::endl;

	std::string response;
	ssl_read(host, port_num, http.str(), response);

	std::string zip_data = response;

	std::string filename = "gtfs.zip";
	std::ofstream ofs(filename, std::ios::binary);
	if (!ofs.is_open())
	{
		return -1;
	}

	ofs.write(zip_data.c_str(), zip_data.size());
	ofs.close();
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// get_rail_predictions
// GET https://api.wmata.com/StationPrediction.svc/json/GetPrediction/{StationCodes} HTTP/1.1
// Cache-Control: no-cache
// api_key: MY_KEY
//
// Parameters:
//   api_key       - Your WMATA API key
//   station_codes - Comma-separated station codes (e.g., "A01,C01") or "All" for all stations
/////////////////////////////////////////////////////////////////////////////////////////////////////

int get_rail_predictions(const std::string& api_key, const std::string& station_codes)
{
	const std::string host = "api.wmata.com";
	const std::string port_num = "443";
	std::stringstream http;
	http << "GET /StationPrediction.svc/json/GetPrediction/" << station_codes << " HTTP/1.1\r\n";
	http << "Host: " << host << "\r\n";
	http << "Accept: application/json\r\n";
	http << "Cache-Control: no-cache\r\n";
	http << "api_key: " << api_key << "\r\n";
	http << "Connection: close\r\n\r\n";
	std::cout << http.str() << std::endl;

	std::string json;
	ssl_read(host, port_num, http.str(), json);

	if (!json.size())
	{
		return -1;
	}

	std::cout << json.c_str() << std::endl;
	std::ofstream ofs("predictions_" + station_codes + ".json");
	ofs << json;
	ofs.close();

	return 0;
}