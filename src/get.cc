#include <iostream>
#include <sstream>
#include <fstream>
#include "ssl_read.hh"

/////////////////////////////////////////////////////////////////////////////////////////////////////
// get_station_list
// GET https://api.wmata.com/Rail.svc/json/jStations?LineCode=RD HTTP/1.1
// Cache-Control: no-cache
// api_key: MY_KEY
/////////////////////////////////////////////////////////////////////////////////////////////////////

int get_station_list(const std::string& api_key)
{
  const std::string host = "api.wmata.com";
  const std::string port_num = "443";
  std::stringstream http;
  http << "GET /Rail.svc/json/jStations?LineCode=RD HTTP/1.1\r\n";
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
  std::ofstream ofs("response.json");
  ofs << json;
  ofs.close();

  return 0;
}
