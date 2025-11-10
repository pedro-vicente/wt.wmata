#include <iomanip> 
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include "wmata.hh"
#include "ssl_read.hh"

/////////////////////////////////////////////////////////////////////////////////////////////////////
// globals
/////////////////////////////////////////////////////////////////////////////////////////////////////

extern std::vector<Station> stations;
extern std::vector<Prediction> predictions; 

/////////////////////////////////////////////////////////////////////////////////////////////////////
//to_hex
//convert int to hex string, apply zero padding
/////////////////////////////////////////////////////////////////////////////////////////////////////

std::string to_hex(int n)
{
  std::stringstream ss;
  ss << std::hex << std::setw(2) << std::setfill('0') << n;
  return ss.str();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//rgb_to_hex
/////////////////////////////////////////////////////////////////////////////////////////////////////

std::string rgb_to_hex(int r, int g, int b)
{
  std::string str("#");
  str += to_hex(r);
  str += to_hex(g);
  str += to_hex(b);
  return str;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// load_file
/////////////////////////////////////////////////////////////////////////////////////////////////////

std::string load_file(const std::string& filename)
{
  std::ifstream file(filename);
  if (!file.is_open())
    return "";

  std::stringstream buf;
  buf << file.rdbuf();
  return buf.str();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// escape_js_string
// escape special characters for JavaScript string literals
/////////////////////////////////////////////////////////////////////////////////////////////////////

std::string escape_js_string(const std::string& str)
{
  std::string s;
  s.reserve(str.length());

  for (size_t i = 0; i < str.length(); ++i)
  {
    char c = str[i];
    switch (c)
    {
    case '\'': s += "\\'"; break;
    case '\"': s += "\\\""; break;
    case '\\': s += "\\\\"; break;
    case '\n': s += "\\n"; break;
    case '\r': s += "\\r"; break;
    case '\t': s += "\\t"; break;
    default: s += c; break;
    }
  }

  return s;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// parse_stations
// Parse obj JSON and add to global stations vector
// Parameters:
//   buf - JSON string containing obj data
//   clear - If true, clear existing stations before adding new ones
/////////////////////////////////////////////////////////////////////////////////////////////////////

void parse_stations(const std::string& buf, bool clear)
{
  if (clear)
  {
    stations.clear();
  }

  try
  {
    Wt::Json::Object root;
    Wt::Json::parse(buf, root);

    if (root.contains("Stations"))
    {
      const Wt::Json::Array& arr = root.get("Stations");
      size_t count = 0;

      for (size_t idx = 0; idx < arr.size(); ++idx)
      {
        const Wt::Json::Object& obj = arr[idx];

        std::string Code = obj.get("Code").orIfNull("");
        std::string Name = obj.get("Name").orIfNull("");
        double Lat = obj.get("Lat").orIfNull(0.0);
        double Lon = obj.get("Lon").orIfNull(0.0);
        std::string lineCode1 = obj.get("LineCode1").orIfNull("");

        std::string Address = "";
        if (obj.contains("Address"))
        {
          const Wt::Json::Object& addrObj = obj.get("Address");
          Address = addrObj.get("Street").orIfNull("");
        }

        stations.emplace_back(Code, Name, Lat, Lon, lineCode1, Address);
        count++;
      }
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// parse_predictions
// Parse prediction JSON and update global predictions vector
/////////////////////////////////////////////////////////////////////////////////////////////////////

void parse_predictions(const std::string& buf)
{
  predictions.clear();

  try
  {
    Wt::Json::Object root;
    Wt::Json::parse(buf, root);

    if (root.contains("Trains"))
    {
      const Wt::Json::Array& arr = root.get("Trains");

      for (size_t idx = 0; idx < arr.size(); ++idx)
      {
        const Wt::Json::Object& obj = arr[idx];

        std::string Car = obj.get("Car").orIfNull("");
        std::string Destination = obj.get("Destination").orIfNull("");
        std::string DestinationCode = obj.get("DestinationCode").orIfNull("");
        std::string Group = obj.get("Group").orIfNull("");
        std::string Line = obj.get("Line").orIfNull("");
        std::string LocationCode = obj.get("LocationCode").orIfNull("");
        std::string LocationName = obj.get("LocationName").orIfNull("");
        std::string Min = obj.get("Min").orIfNull("");

        if (Line == "RD")
        {
          predictions.emplace_back(Car, Destination, DestinationCode, Group, Line, LocationCode, LocationName, Min);
        }
      }
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// fetch_red_line_predictions
// Fetch live Red Line predictions from WMATA API
// Returns: predictions JSON string, empty on error
/////////////////////////////////////////////////////////////////////////////////////////////////////

std::string fetch_predictions(const std::string& api_key)
{
  std::stringstream codes;
  bool first = true;
  for (size_t idx = 0; idx < stations.size(); ++idx)
  {
    if (stations[idx].LineCode1 == "RD")
    {
      if (!first) codes << ",";
      codes << stations[idx].Code;
      first = false;
    }
  }

  const std::string host = "api.wmata.com";
  const std::string port_num = "443";
  std::stringstream http;
  http << "GET /StationPrediction.svc/json/GetPrediction/" << codes.str() << " HTTP/1.1\r\n";
  http << "Host: " << host << "\r\n";
  http << "Accept: application/json\r\n";
  http << "Cache-Control: no-cache\r\n";
  http << "api_key: " << api_key << "\r\n";
  http << "Connection: close\r\n\r\n";

  std::cout << http.str() << std::endl;

  std::string json;
  try
  {
    ssl_read(host, port_num, http.str(), json);
  }
  catch (const std::exception& e)
  {
    return "";
  }

  return json;
}
