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

/////////////////////////////////////////////////////////////////////////////////////////////////////
// globals
/////////////////////////////////////////////////////////////////////////////////////////////////////

void parse_stations(const std::string& buf, bool clear = false);
void parse_predictions(const std::string& buf);
std::string fetch_predictions(const std::string& api_key);
std::vector<TrainPosition> calculate_positions();
std::string generate_train(const std::vector<TrainPosition>& positions);

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
std::string geojson_red_line;
std::vector<Station> stations;
std::vector<Prediction> predictions;

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

  geojson_red_line = load_file("data/line_RD.geojson");
  if (!geojson_red_line.empty())
  {
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

  if (!geojson_wards.empty())
  {
    map->geojson = geojson_wards;
  }

  if (!geojson_red_line.empty())
  {
    map->red_line_geojson = geojson_red_line;
  }

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
    geojson.clear();
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

      js << "map.addSource('wards', {\n"
        << "  'type': 'geojson',\n"
        << "  'data': " << geojson << "\n"
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

      if (!this->red_line_geojson.empty())
      {
        js << "\n// Add Red Line\n";
        js << "map.addSource('red-line', {\n"
          << "  'type': 'geojson',\n"
          << "  'data': " << this->red_line_geojson << "\n"
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
        << "    el.style.width = '16px';\n"
        << "    el.style.height = '16px';\n"
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

#ifdef _WIN32
      OutputDebugStringA(js.str().c_str());
#endif

      //close map.on('load')
      js << "});\n";

      WApplication* app = WApplication::instance();
      app->doJavaScript(js.str());
    }
  }

}// namespace Wt

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
  js << "update_trains(" << trains_json << ");";
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
// calculate_train_positions
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
        should_interpolate = (minutes > 0);
      }
      catch (...)
      {
        should_interpolate = false;
      }
    }

    if (should_interpolate)
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

          const double avg_travel_time = 3.0;
          double fraction = (avg_travel_time - minutes) / avg_travel_time;

          if (fraction < 0.0) fraction = 0.0;
          if (fraction > 1.0) fraction = 1.0;
          interpolate_coordinates(prev_lng, prev_lat, lng, lat, fraction, lng, lat);
        }
      }
    }

    pos.Lng = lng;
    pos.Lat = lat;
    positions.push_back(pos);
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

  for (size_t idx = 0; idx < positions.size(); ++idx)
  {
    const TrainPosition& pos = positions[idx];

    if (idx > 0)
    {
      json << ",";
    }

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
