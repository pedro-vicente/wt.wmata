#include <iostream>
#include <sstream>
#include <fstream>
#include "ssl_read.hh"

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
