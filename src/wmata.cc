#include <Wt/WApplication.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WCompositeWidget.h>
#include <Wt/WHBoxLayout.h>
#include <Wt/WBreak.h>
#include <Wt/Json/Object.h>
#include <Wt/Json/Parser.h>
#include <Wt/Json/Array.h>
#include <Wt/WTimer.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <cmath>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <iostream>
#include <chrono>
#include <fstream>
#include "map.hh"
#include "wmata.hh"
#include "ssl_read.hh"
#include "get.hh"

/////////////////////////////////////////////////////////////////////////////////////////////////////
// globals
/////////////////////////////////////////////////////////////////////////////////////////////////////

void parse_stations(const std::string& buf, bool clear = false);
void parse_predictions(const std::string& buf);
std::string fetch_predictions(const std::string& api_key);
std::vector<TrainPosition> calculate_positions();
std::string generate_train(const std::vector<TrainPosition>& positions);
void parse_line_geometry(const std::string& geojson, std::vector<std::pair<double, double>>& path_points);
double calculate_distance(double lon1, double lat1, double lon2, double lat2);
void interpolate_along_path(const std::vector<std::pair<double, double>>& path,
  double start_lon, double start_lat,
  double end_lon, double end_lat,
  double fraction,
  double& out_lon, double& out_lat);

std::vector<std::string> ward_color =
{ rgb_to_hex(128, 128, 0), //olive
  rgb_to_hex(255, 255, 0), //yellow 
  rgb_to_hex(0, 128, 0), //green
  rgb_to_hex(0, 255, 0), //lime
  rgb_to_hex(0, 128, 128), //teal
  rgb_to_hex(0, 255, 255), //aqua
  rgb_to_hex(0, 0, 255), //blue
  rgb_to_hex(128, 0, 128) //purple
};

std::string geojson_wards;
std::string geojson_red;
std::vector<Station> stations;
std::vector<Prediction> predictions;
std::vector<std::pair<double, double>> red_path;

std::map<std::string, std::string> line_colors =
{
  {"RD", "#E51636"}, // Red Line
  {"OR", "#F68712"}, // Orange Line
  {"SV", "#9D9F9C"}, // Silver Line
  {"BL", "#1574C4"}, // Blue Line
  {"YL", "#FFD100"}, // Yellow Line
  {"GR", "#0FAB4B"}  // Green Line
};

const std::vector<std::string> line_codes = { "RD", "OR", "SV", "BL", "YL", "GR" };

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Red Line station order (Shady Grove to Glenmont)
/////////////////////////////////////////////////////////////////////////////////////////////////////

const std::vector<std::string> red_line_order =
{
  "A15", "A14", "A13", "A12", "A11", "A10", "A09", "A08",
  "A07", "A06", "A05", "A04", "A03", "A02", "A01", "B01",
  "B02", "B03", "B35", "B04", "B05", "B06", "B07", "B08",
  "E06", "B09", "B10"
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// average travel times between consecutive Red Line stations (in seconds)
/////////////////////////////////////////////////////////////////////////////////////////////////////

std::map<std::string, double> red_times =
{
  {"A15-A14", 180}, {"A14-A13", 120}, {"A13-A12", 120}, {"A12-A11", 120},
  {"A11-A10", 120}, {"A10-A09", 120}, {"A09-A08", 120}, {"A08-A07", 120},
  {"A07-A06", 120}, {"A06-A05", 120}, {"A05-A04", 120}, {"A04-A03", 120},
  {"A03-A02", 120}, {"A02-A01", 120}, {"A01-B01", 120}, {"B01-B02", 120},
  {"B02-B03", 120}, {"B03-B35", 60},  {"B35-B04", 120}, {"B04-B05", 120},
  {"B05-B06", 120}, {"B06-B07", 120}, {"B07-B08", 120}, {"B08-E06", 120},
  {"E06-B09", 120}, {"B09-B10", 180}
};


/////////////////////////////////////////////////////////////////////////////////////////////////////
// create_application
/////////////////////////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<Wt::WApplication> create_application(const Wt::WEnvironment& env)
{
  return std::make_unique<ApplicationMap>(env);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// main
/////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
  geojson_wards = load_file("data/ward-2012.geojson");
  if (!geojson_wards.empty())
  {
  }

  geojson_red = load_file("data/line_RD.geojson");
  if (!geojson_red.empty())
  {
    parse_line_geometry(geojson_red, red_path);
    std::cout << red_path.size() << " path points" << std::endl;
  }

  bool first = true;
  for (size_t idx = 0; idx < line_codes.size(); ++idx)
  {
    const std::string& line_code = line_codes[idx];
    std::string filename = "data/stations_" + line_code + ".json";
    std::string stations_json = load_file(filename);
    if (!stations_json.empty())
    {
      parse_stations(stations_json, first);
      first = false;
    }
    else
    {
    }
  }

  std::cout << stations.size() << " stations loaded." << std::endl;
  int result = Wt::WRun(argc, argv, &create_application);
  return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// ApplicationMap
/////////////////////////////////////////////////////////////////////////////////////////////////////

ApplicationMap::ApplicationMap(const Wt::WEnvironment& env)
  : WApplication(env), map(nullptr)
{
  std::unique_ptr<Wt::WHBoxLayout> layout = std::make_unique<Wt::WHBoxLayout>();
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  /////////////////////////////////////////////////////////////////////////////////////////////////////
  // map
  // stretch factor 1 (fills remaining space).
  /////////////////////////////////////////////////////////////////////////////////////////////////////

  std::unique_ptr<Wt::WContainerWidget> container_map = std::make_unique<Wt::WContainerWidget>();
  map_container = container_map.get();
  map = container_map->addWidget(std::make_unique<Wt::WMapLibre>());
  map->resize(Wt::WLength::Auto, Wt::WLength::Auto);

  layout->addWidget(std::move(container_map), 1);
  root()->setLayout(std::move(layout));

  /////////////////////////////////////////////////////////////////////////////////////////////////////
  // setup timer to update predictions every 2 minutes
  /////////////////////////////////////////////////////////////////////////////////////////////////////

  timer = root()->addChild(std::make_unique<Wt::WTimer>());
  timer->setInterval(std::chrono::seconds(120));
  timer->timeout().connect(this, &ApplicationMap::update_predictions);
  timer->start();

  /////////////////////////////////////////////////////////////////////////////////////////////////////
  // initial update after 3 seconds
  /////////////////////////////////////////////////////////////////////////////////////////////////////

  Wt::WTimer* t = root()->addChild(std::make_unique<Wt::WTimer>());
  t->setInterval(std::chrono::seconds(3));
  t->timeout().connect(this, &ApplicationMap::update_predictions);
  t->setSingleShot(true);
  t->start();

}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// ~ApplicationMap
/////////////////////////////////////////////////////////////////////////////////////////////////////

ApplicationMap::~ApplicationMap()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// parse_stations
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

      for (size_t idx = 0; idx < arr.size(); ++idx)
      {
        const Wt::Json::Object& obj = arr[idx];

        std::string Code = obj.get("Code").orIfNull("");
        std::string Name = obj.get("Name").orIfNull("");
        double Lat = obj.get("Lat").orIfNull(0.0);
        double Lon = obj.get("Lon").orIfNull(0.0);
        std::string LineCode1 = obj.get("LineCode1").orIfNull("");

        std::string Address = "";
        if (obj.contains("Address"))
        {
          const Wt::Json::Object& addrObj = obj.get("Address");
          Address = addrObj.get("Street").orIfNull("");
        }

        stations.emplace_back(Code, Name, Lat, Lon, LineCode1, Address);
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
// fetch_predictions
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

/////////////////////////////////////////////////////////////////////////////////////////////////////
// update_predictions
/////////////////////////////////////////////////////////////////////////////////////////////////////

void ApplicationMap::update_predictions()
{
  std::ifstream file("config.json");
  if (!file.is_open())
  {
    return;
  }

  std::stringstream ss;
  ss << file.rdbuf();
  std::string buf = ss.str();
  file.close();
  std::string api_key = extract_value(buf, "API_KEY");
  std::string json = fetch_predictions(api_key);
  try
  {
    parse_predictions(json);
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return;
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////////
 // calculate train positions
 /////////////////////////////////////////////////////////////////////////////////////////////////////

  std::vector<TrainPosition> positions = calculate_positions();

  /////////////////////////////////////////////////////////////////////////////////////////////////////
  // generate JSON with calculated positions
  /////////////////////////////////////////////////////////////////////////////////////////////////////

  std::string trains_json = generate_train(positions);

  /////////////////////////////////////////////////////////////////////////////////////////////////////
  // render on map
  /////////////////////////////////////////////////////////////////////////////////////////////////////

  std::stringstream js;
  js << "if (typeof window.update_trains === 'function') {\n"
    << "  window.update_trains(" << trains_json << ");\n"
    << "}";
  doJavaScript(js.str());
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// interpolate_coordinates
/////////////////////////////////////////////////////////////////////////////////////////////////////

void interpolate_coordinates(double lng1, double lat1, double lng2, double lat2, double fraction, double& out_lng, double& out_lat)
{
  out_lng = lng1 + (lng2 - lng1) * fraction;
  out_lat = lat1 + (lat2 - lat1) * fraction;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// parse_line_geometry
// extract path points from GeoJSON LineString
/////////////////////////////////////////////////////////////////////////////////////////////////////

void parse_line_geometry(const std::string& geojson, std::vector<std::pair<double, double>>& path_points)
{
  path_points.clear();

  try
  {
    Wt::Json::Object root;
    Wt::Json::parse(geojson, root);

    if (root.get("type").orIfNull("") == "FeatureCollection")
    {
      Wt::Json::Array features = root.get("features");
      if (features.size() > 0)
      {
        Wt::Json::Object feature = features[0];
        Wt::Json::Object geometry = feature.get("geometry");
        std::string geom_type = geometry.get("type").orIfNull("");

        if (geom_type == "LineString" || geom_type == "MultiLineString")
        {
          Wt::Json::Array coordinates = geometry.get("coordinates");

          if (geom_type == "MultiLineString")
          {
            for (size_t i = 0; i < coordinates.size(); ++i)
            {
              Wt::Json::Array line_coords = coordinates[i];
              for (size_t j = 0; j < line_coords.size(); ++j)
              {
                Wt::Json::Array coord = line_coords[j];
                double lon = coord[0].orIfNull(0.0);
                double lat = coord[1].orIfNull(0.0);
                path_points.emplace_back(lon, lat);
              }
            }
          }
          else
          {
            for (size_t i = 0; i < coordinates.size(); ++i)
            {
              Wt::Json::Array coord = coordinates[i];
              double lon = coord[0].orIfNull(0.0);
              double lat = coord[1].orIfNull(0.0);
              path_points.emplace_back(lon, lat);
            }
          }
        }
      }
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << "Error parsing line geometry: " << e.what() << std::endl;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// calculate_distance
// Haversine formula for distance between two coordinates
/////////////////////////////////////////////////////////////////////////////////////////////////////

double calculate_distance(double lon1, double lat1, double lon2, double lat2)
{
  if (std::isnan(lon1) || std::isnan(lat1) || std::isnan(lon2) || std::isnan(lat2))
  {
    return 1e10;
  }

  const double R = 6371000;
  double phi1 = lat1 * M_PI / 180.0;
  double phi2 = lat2 * M_PI / 180.0;
  double delta_phi = (lat2 - lat1) * M_PI / 180.0;
  double delta_lambda = (lon2 - lon1) * M_PI / 180.0;

  double a = std::sin(delta_phi / 2.0) * std::sin(delta_phi / 2.0) +
    std::cos(phi1) * std::cos(phi2) *
    std::sin(delta_lambda / 2.0) * std::sin(delta_lambda / 2.0);
  double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));

  return R * c;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// interpolate_along_path
// interpolate position along the actual track path
/////////////////////////////////////////////////////////////////////////////////////////////////////

void interpolate_along_path(const std::vector<std::pair<double, double>>& path,
  double start_lon, double start_lat,
  double end_lon, double end_lat,
  double fraction,
  double& out_lon, double& out_lat)
{
  if (path.empty())
  {
    //linear interpolation
    out_lon = start_lon + (end_lon - start_lon) * fraction;
    out_lat = start_lat + (end_lat - start_lat) * fraction;
    return;
  }

  int start_idx = -1;
  int end_idx = -1;
  double min_start_dist = 1e10;
  double min_end_dist = 1e10;

  for (size_t idx = 0; idx < path.size(); ++idx)
  {
    double dist_to_start = calculate_distance(path[idx].first, path[idx].second, start_lon, start_lat);
    double dist_to_end = calculate_distance(path[idx].first, path[idx].second, end_lon, end_lat);

    if (dist_to_start < min_start_dist)
    {
      min_start_dist = dist_to_start;
      start_idx = static_cast<int>(idx);
    }
    if (dist_to_end < min_end_dist)
    {
      min_end_dist = dist_to_end;
      end_idx = static_cast<int>(idx);
    }
  }

  if (start_idx == -1 || end_idx == -1 || start_idx == end_idx)
  {
    //linear interpolation
    out_lon = start_lon + (end_lon - start_lon) * fraction;
    out_lat = start_lat + (end_lat - start_lat) * fraction;
    return;
  }

  if (start_idx > end_idx)
  {
    std::swap(start_idx, end_idx);
    // reverse direction
    fraction = 1.0 - fraction;
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////////
  // calculate total distance along path
  /////////////////////////////////////////////////////////////////////////////////////////////////////

  double distance = 0.0;
  std::vector<double> cumulative_distances;
  cumulative_distances.push_back(0.0);

  for (int idx = start_idx; idx < end_idx; ++idx)
  {
    double segment_dist = calculate_distance(
      path[idx].first, path[idx].second,
      path[idx + 1].first, path[idx + 1].second
    );
    distance += segment_dist;
    cumulative_distances.push_back(distance);
  }

  double target_distance = distance * fraction;
  int segment_idx = start_idx;

  for (size_t idx = 0; idx < cumulative_distances.size() - 1; ++idx)
  {
    if (target_distance >= cumulative_distances[idx] && target_distance <= cumulative_distances[idx + 1])
    {
      segment_idx = start_idx + static_cast<int>(idx);
      double distance_diff = cumulative_distances[idx + 1] - cumulative_distances[idx];
      double segment_fraction = 0.0;
      if (distance_diff > 1e-10)
      {
        segment_fraction = (target_distance - cumulative_distances[idx]) / distance_diff;
      }

      out_lon = path[segment_idx].first +
        (path[segment_idx + 1].first - path[segment_idx].first) * segment_fraction;
      out_lat = path[segment_idx].second +
        (path[segment_idx + 1].second - path[segment_idx].second) * segment_fraction;
      return;
    }
  }

  out_lon = end_lon;
  out_lat = end_lat;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// calculate_train_positions - Uses path-based interpolation
/////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<TrainPosition> calculate_positions()
{
  std::vector<TrainPosition> positions;
  std::map<std::string, std::pair<double, double>> station_coords;
  for (size_t idx = 0; idx < stations.size(); ++idx)
  {
    const Station& station = stations[idx];
    station_coords[station.Code] = std::make_pair(station.Lon, station.Lat);
  }

  std::map<std::string, int> station_index;
  for (size_t idx = 0; idx < red_line_order.size(); ++idx)
  {
    station_index[red_line_order[idx]] = static_cast<int>(idx);
  }

  for (size_t idx = 0; idx < predictions.size(); ++idx)
  {
    const Prediction& pred = predictions[idx];
    if (pred.Line != "RD")
    {
      continue;
    }

    if (station_coords.find(pred.LocationCode) == station_coords.end())
    {
      continue;
    }

    TrainPosition pos;
    pos.Destination = pred.Destination;
    pos.LocationName = pred.LocationName;
    pos.Min = pred.Min;
    pos.Car = pred.Car;
    pos.LineColor = line_colors["RD"];

    double lng = station_coords[pred.LocationCode].first;
    double lat = station_coords[pred.LocationCode].second;

    bool should_interpolate = false;
    int minutes = 0;

    if (!pred.Min.empty() && pred.Min != "ARR" && pred.Min != "BRD" && pred.Min != "")
    {
      try
      {
        minutes = std::stoi(pred.Min);
        should_interpolate = (minutes > 0 && minutes <= 10);
      }
      catch (...)
      {
        should_interpolate = false;
      }
    }

    if (should_interpolate && !red_path.empty())
    {
      bool is_towards_glenmont = (pred.Group == "1");

      int location_idx = station_index[pred.LocationCode];
      int prev_station_idx = is_towards_glenmont ? location_idx - 1 : location_idx + 1;

      if (prev_station_idx >= 0 && prev_station_idx < static_cast<int>(red_line_order.size()))
      {
        std::string prev_station_code = red_line_order[prev_station_idx];

        if (station_coords.find(prev_station_code) != station_coords.end())
        {
          double prev_lng = station_coords[prev_station_code].first;
          double prev_lat = station_coords[prev_station_code].second;

          std::string segment_key = is_towards_glenmont ?
            prev_station_code + "-" + pred.LocationCode :
            pred.LocationCode + "-" + prev_station_code;

          double travel_time_seconds = 120.0;
          if (red_times.find(segment_key) != red_times.end())
          {
            travel_time_seconds = red_times[segment_key];
          }

          if (travel_time_seconds < 1.0)
          {
            travel_time_seconds = 120.0;
          }

          double travel_time_minutes = travel_time_seconds / 60.0;
          double fraction = (travel_time_minutes - minutes) / travel_time_minutes;

          if (fraction < 0.0) fraction = 0.0;
          if (fraction > 1.0) fraction = 1.0;

          if (!std::isnan(prev_lng) && !std::isnan(prev_lat) &&
            !std::isnan(lng) && !std::isnan(lat))
          {
            interpolate_along_path(red_path, prev_lng, prev_lat, lng, lat, fraction, lng, lat);
          }
        }
      }
    }

    pos.Lng = lng;
    pos.Lat = lat;

    if (!std::isnan(pos.Lng) && !std::isnan(pos.Lat) && !std::isinf(pos.Lng) && !std::isinf(pos.Lat))
    {
      positions.push_back(pos);
    }
  }

  return positions;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// generate_train
/////////////////////////////////////////////////////////////////////////////////////////////////////

std::string generate_train(const std::vector<TrainPosition>& positions)
{
  std::stringstream json;
  json << "{\"trains\":[";

  bool first = true;
  for (size_t idx = 0; idx < positions.size(); ++idx)
  {
    const TrainPosition& pos = positions[idx];

    if (std::isnan(pos.Lng) || std::isnan(pos.Lat) ||
      std::isinf(pos.Lng) || std::isinf(pos.Lat))
    {
      continue;
    }

    if (!first)
    {
      json << ",";
    }
    first = false;

    json << "{"
      << "\"lng\":" << pos.Lng << ","
      << "\"lat\":" << pos.Lat << ","
      << "\"destination\":\"" << pos.Destination << "\","
      << "\"location_name\":\"" << pos.LocationName << "\","
      << "\"min\":\"" << pos.Min << "\","
      << "\"car\":\"" << pos.Car << "\","
      << "\"line_color\":\"" << pos.LineColor << "\""
      << "}";
  }

  json << "]}";
  return json.str();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// WMapLibre
/////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Wt
{
  class WMapLibre::Impl : public WWebWidget
  {
  public:
    Impl();
    virtual DomElementType domElementType() const override;
  };

  WMapLibre::Impl::Impl()
  {
    setInline(false);
  }

  DomElementType WMapLibre::Impl::domElementType() const
  {
    return DomElementType::DIV;
  }

  WMapLibre::WMapLibre()
  {
    setImplementation(std::unique_ptr<Impl>(impl = new Impl()));
    WApplication* app = WApplication::instance();
    this->addCssRule("body", "margin: 0; padding: 0;");
    this->addCssRule("#" + id(), "position: absolute; top: 0; bottom: 0; width: 100%;");
    app->useStyleSheet("https://unpkg.com/maplibre-gl@4.7.1/dist/maplibre-gl.css");
    const std::string library = "https://unpkg.com/maplibre-gl@4.7.1/dist/maplibre-gl.js";
    app->require(library, "maplibre");
  }

  WMapLibre::~WMapLibre()
  {
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////////
  // render
  /////////////////////////////////////////////////////////////////////////////////////////////////////

  void WMapLibre::render(WFlags<RenderFlag> flags)
  {
    WCompositeWidget::render(flags);

    if (flags.test(RenderFlag::Full))
    {
      std::stringstream js;

      /////////////////////////////////////////////////////////////////////////////////////////////////////
      // create map
      /////////////////////////////////////////////////////////////////////////////////////////////////////

      js << "const map = new maplibregl.Map({\n"
        << "  container: " << jsRef() << ",\n"
        << "  style: 'https://basemaps.cartocdn.com/gl/positron-gl-style/style.json',\n"
        << "  center: [-77.0369, 38.9072],\n"
        << "  zoom: 11\n"
        << "});\n"

        << "map.addControl(new maplibregl.NavigationControl());\n";

#ifdef _WIN32
      OutputDebugStringA(js.str().c_str());
#endif

      js << "map.on('load', function() {\n";

      js << "var ward_color = [";
      for (size_t idx = 0; idx < ward_color.size(); ++idx)
      {
        js << "'" << ward_color[idx] << "'";
        if (idx < ward_color.size() - 1) js << ",";
      }
      js << "];\n";

      /////////////////////////////////////////////////////////////////////////////////////////////////////
      // add ward layer
      /////////////////////////////////////////////////////////////////////////////////////////////////////

      if (!geojson_wards.empty())
      {

        js << "map.addSource('wards', {\n"
          << "  'type': 'geojson',\n"
          << "  'data': " << geojson_wards << "\n"
          << "});\n"

          << "map.addLayer({\n"
          << "  'id': 'wards-fill',\n"
          << "  'type': 'fill',\n"
          << "  'source': 'wards',\n"
          << "  'paint': {\n"
          << "    'fill-color': ['get', ['to-string', ['get', 'WARD']], ['literal', {\n";

        for (size_t idx = 0; idx < ward_color.size(); ++idx)
        {
          js << "      '" << (idx + 1) << "': '" << ward_color[idx] << "'";
          if (idx < ward_color.size() - 1) js << ",\n";
        }

        js << "\n    }]],\n"
          << "    'fill-opacity': 0.2\n"
          << "  }\n"
          << "});\n";
      }

      /////////////////////////////////////////////////////////////////////////////////////////////////////
      // add metro stations as circles
      /////////////////////////////////////////////////////////////////////////////////////////////////////

      if (!stations.empty())
      {
        js << "\nvar station_features = [\n";

        for (size_t idx = 0; idx < stations.size(); ++idx)
        {
          const Station& station = stations[idx];

          std::string color = "#E51636";
          std::map<std::string, std::string>::iterator it = line_colors.find(station.LineCode1);
          if (it != line_colors.end())
          {
            color = it->second;
          }

          js << "  {\n"
            << "    'type': 'Feature',\n"
            << "    'geometry': {\n"
            << "      'type': 'Point',\n"
            << "      'coordinates': [" << station.Lon << ", " << station.Lat << "]\n"
            << "    },\n"
            << "    'properties': {\n"
            << "      'Name': '" << escape_js_string(station.Name) << "',\n"
            << "      'Code': '" << escape_js_string(station.Code) << "',\n"
            << "      'line': '" << escape_js_string(station.LineCode1) << "',\n"
            << "      'color': '" << color << "',\n"
            << "      'Address': '" << escape_js_string(station.Address) << "'\n"
            << "    }\n"
            << "  }";

          if (idx < stations.size() - 1)
          {
            js << ",";
          }
          js << "\n";
        }

        js << "];\n\n"

          << "map.addSource('stations', {\n"
          << "  'type': 'geojson',\n"
          << "  'data': {\n"
          << "    'type': 'FeatureCollection',\n"
          << "    'features': station_features\n"
          << "  }\n"
          << "});\n\n"

          << "map.addLayer({\n"
          << "  'id': 'station-circles',\n"
          << "  'type': 'circle',\n"
          << "  'source': 'stations',\n"
          << "  'paint': {\n"
          << "    'circle-radius': 8,\n"
          << "    'circle-color': ['get', 'color'],\n"
          << "    'circle-stroke-width': 2,\n"
          << "    'circle-stroke-color': '#ffffff'\n"
          << "  }\n"
          << "});\n\n"

          << "var popup = new maplibregl.Popup({\n"
          << "  closeButton: false,\n"
          << "  closeOnClick: false\n"
          << "});\n\n"

          << "map.on('mouseenter', 'station-circles', function(e) {\n"
          << "  map.getCanvas().style.cursor = 'pointer';\n"
          << "  var coordinates = e.features[0].geometry.coordinates.slice();\n"
          << "  var station_name = e.features[0].properties.Name;\n"
          << "  var station_line = e.features[0].properties.line;\n"
          << "  var station_address = e.features[0].properties.Address;\n"
          << "  var html = '<strong>' + station_name + '</strong><br>' + station_line + ' Line<br>' + station_address;\n"
          << "  while (Math.abs(e.lngLat.lng - coordinates[0]) > 180) {\n"
          << "    coordinates[0] += e.lngLat.lng > coordinates[0] ? 360 : -360;\n"
          << "  }\n"
          << "  popup.setLngLat(coordinates).setHTML(html).addTo(map);\n"
          << "});\n\n"

          << "map.on('mouseleave', 'station-circles', function() {\n"
          << "  map.getCanvas().style.cursor = '';\n"
          << "  popup.remove();\n"
          << "});\n";
      }

      /////////////////////////////////////////////////////////////////////////////////////////////////////
      // add red line
      /////////////////////////////////////////////////////////////////////////////////////////////////////

      if (!geojson_red.empty())
      {
        js << "\n// Add Red Line\n";
        js << "map.addSource('red-line', {\n"
          << "  'type': 'geojson',\n"
          << "  'data': " << geojson_red << "\n"
          << "});\n\n"
          << "map.addLayer({\n"
          << "  'id': 'red-line-layer',\n"
          << "  'type': 'line',\n"
          << "  'source': 'red-line',\n"
          << "  'layout': {\n"
          << "    'line-join': 'round',\n"
          << "    'line-cap': 'round'\n"
          << "  },\n"
          << "  'paint': {\n"
          << "    'line-color': '#E51636',\n"
          << "    'line-width': 4,\n"
          << "    'line-opacity': 0.8\n"
          << "  }\n"
          << "}, 'station-circles');\n\n";
      }

      /////////////////////////////////////////////////////////////////////////////////////////////////////
      // train markers
      /////////////////////////////////////////////////////////////////////////////////////////////////////

      js << "var train_markers = [];\n"
        << "var train_popup = new maplibregl.Popup({\n"
        << "  closeButton: false,\n"
        << "  closeOnClick: false\n"
        << "});\n\n"

        << "window.update_trains = function(data) {\n"
        << "  train_markers.forEach(m => m.remove());\n"
        << "  train_markers = [];\n"
        << "  \n"
        << "  if (!data || !data.trains) {\n"
        << "    return;\n"
        << "  }\n"
        << "  \n"
        << "  data.trains.forEach(train => {\n"
        << "    var el = document.createElement('div');\n"
        << "    el.style.width = '8px';\n"
        << "    el.style.height = '8px';\n"
        << "    el.style.borderRadius = '50%';\n"
        << "    el.style.backgroundColor = train.line_color;\n"
        << "    el.style.border = '3px solid white';\n"
        << "    el.style.boxShadow = '0 2px 4px rgba(0,0,0,0.3)';\n"
        << "    el.style.cursor = 'pointer';\n"
        << "    \n"
        << "    var marker = new maplibregl.Marker({\n"
        << "      element: el,\n"
        << "      anchor: 'center'\n"
        << "    })\n"
        << "    .setLngLat([train.lng, train.lat])\n"
        << "    .addTo(map);\n"
        << "    \n"
        << "    train_markers.push(marker);\n"
        << "    \n"
        << "    el.addEventListener('mouseenter', function() {\n"
        << "      var html = '<strong>Red Line to ' + train.destination + '</strong><br>'\n"
        << "        + 'Next stop: ' + train.location_name + '<br>'\n"
        << "        + 'Arriving in: ' + train.min + '<br>'\n"
        << "        + 'Cars: ' + train.car;\n"
        << "      train_popup.setLngLat([train.lng, train.lat]).setHTML(html).addTo(map);\n"
        << "    });\n"
        << "    \n"
        << "    el.addEventListener('mouseleave', function() {\n"
        << "      train_popup.remove();\n"
        << "    });\n"
        << "  });\n"
        << "};\n";

      //close map.on('load')
      js << "});\n";


#ifdef _WIN32
      OutputDebugStringA(js.str().c_str());
#endif


      /////////////////////////////////////////////////////////////////////////////////////////////////////
     // update_trains function - MUST be in global scope
     /////////////////////////////////////////////////////////////////////////////////////////////////////

      js << "window.update_trains = function(data) {\n"
        << "  var features = [];\n"
        << "  for (var i = 0; i < data.trains.length; i++) {\n"
        << "    var train = data.trains[i];\n"
        << "    features.push({\n"
        << "      'type': 'Feature',\n"
        << "      'geometry': {\n"
        << "        'type': 'Point',\n"
        << "        'coordinates': [train.lng, train.lat]\n"
        << "      },\n"
        << "      'properties': {\n"
        << "        'destination': train.destination,\n"
        << "        'location_name': train.location_name,\n"
        << "        'min': train.min,\n"
        << "        'car': train.car,\n"
        << "        'line_color': train.line_color\n"
        << "      }\n"
        << "    });\n"
        << "  }\n"
        << "  if (map && map.getSource && map.getSource('trains')) {\n"
        << "    map.getSource('trains').setData({\n"
        << "      'type': 'FeatureCollection',\n"
        << "      'features': features\n"
        << "    });\n"
        << "  }\n"
        << "};\n";

#ifdef _WIN32
      OutputDebugStringA(js.str().c_str());
#endif


      WApplication* app = WApplication::instance();
      app->doJavaScript(js.str());
    }
  }

}// namespace Wt