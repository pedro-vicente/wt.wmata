#include <Wt/WApplication.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WCompositeWidget.h>
#include <Wt/WHBoxLayout.h>
#include <Wt/WBreak.h>
#include <Wt/Json/Object.h>
#include <Wt/Json/Parser.h>
#include <Wt/Json/Array.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <iomanip> 
#include <fstream>
#include <iostream>
#include <chrono>
#include "map.hh"

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

/////////////////////////////////////////////////////////////////////////////////////////////////////
// WMapLibre
/////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Wt
{
  class WT_API WMapLibre : public WCompositeWidget
  {
    class Impl;
  public:
    WMapLibre();
    ~WMapLibre();

    std::string geojson;
    std::string red_line_geojson;

  protected:
    Impl* impl;
    virtual void render(WFlags<RenderFlag> flags) override;
  };
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Station
/////////////////////////////////////////////////////////////////////////////////////////////////////

struct Station
{
  std::string Code;
  std::string Name;
  double Lat;
  double Lon;
  std::string LineCode1;
  std::string Address;

  Station(const std::string& code, const std::string& name, double lat, double lon,
    const std::string& line_code, const std::string& address)
    : Code(code), Name(name), Lat(lat), Lon(lon), LineCode1(line_code), Address(address) {
  }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// globals
/////////////////////////////////////////////////////////////////////////////////////////////////////

std::string geojson_wards;
std::string geojson_red_line;
std::vector<Station> stations;

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
// parse_stations
// Parse obj JSON and add to global stations vector
// Parameters:
//   buf - JSON string containing obj data
//   clear - If true, clear existing stations before adding new ones
/////////////////////////////////////////////////////////////////////////////////////////////////////

void parse_stations(const std::string& buf, bool clear = false)
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
// ApplicationMap
/////////////////////////////////////////////////////////////////////////////////////////////////////

class ApplicationMap : public Wt::WApplication
{
public:
  ApplicationMap(const Wt::WEnvironment& env);
  virtual ~ApplicationMap();

private:
  Wt::WMapLibre* map;
  Wt::WContainerWidget* map_container;
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

