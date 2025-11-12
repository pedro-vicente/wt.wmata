#ifndef GET_CONGRESS_HH
#define GET_CONGRESS_HH

#include <string>

int get_station_list(const std::string& api_key, const std::string& line_code);
int get_gtfs_rail(const std::string& api_key);
int get_rail_predictions(const std::string& api_key, const std::string& station_codes);
std::string extract_value(const std::string& content, const std::string& key);

#endif