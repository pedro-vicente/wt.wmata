#include <iostream>
#include <ctime>
#include <sstream>
#include <fstream>
#include "asio.hpp"
#include "asio/ssl.hpp"
#include <openssl/ssl.h>
#include "ssl_read.hh"
#include "get.hh"

std::string extract_value(const std::string& content, const std::string& key);

/////////////////////////////////////////////////////////////////////////////////////////////////////
//main
/////////////////////////////////////////////////////////////////////////////////////////////////////

int main()
{
  /////////////////////////////////////////////////////////////////////////////////////////////////////
  // load API key and other configuration values from file
  /////////////////////////////////////////////////////////////////////////////////////////////////////

  std::ifstream file("config.json");
  if (!file.is_open())
  {
    return -1;
  }

  std::stringstream ss;
  ss << file.rdbuf();
  std::string buf = ss.str();
  file.close();

  std::string api_key = extract_value(buf, "API_KEY");
  const std::string lines[] = { "RD", "OR", "SV", "BL", "YL", "GR" };
  const std::string colors[] = { "Red", "Orange", "Silver", "Blue", "Yellow", "Green" };

  for (size_t idx = 0; idx < 6; ++idx)
  {
    int result = get_station_list(api_key, lines[idx]);
    if (result != 0)
    {
    }
  }
  return 0;
}

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
