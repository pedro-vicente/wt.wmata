#include <iostream>
#include <sstream>
#include <fstream>
#include "get.hh"

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

  int result = get_rail_predictions(api_key, "All");

  const std::string lines[] = { "RD", "OR", "SV", "BL", "YL", "GR" };
  const std::string colors[] = { "Red", "Orange", "Silver", "Blue", "Yellow", "Green" };
  for (size_t idx = 0; idx < 6; ++idx)
  {
    result = get_station_list(api_key, lines[idx]);
    if (result != 0)
    {
    }
  }

  result = get_gtfs_rail(api_key);
  return 0;
}

