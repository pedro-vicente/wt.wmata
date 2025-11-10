#include <Wt/WTimer.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include "map.hh"
#include "wmata.hh"

/////////////////////////////////////////////////////////////////////////////////////////////////////
// globals
/////////////////////////////////////////////////////////////////////////////////////////////////////

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

  update_predictions();

  timer = root()->addChild(std::make_unique<Wt::WTimer>());
  timer->setInterval(std::chrono::seconds(120));
  timer->timeout().connect(this, &ApplicationMap::update_predictions);
  timer->start();

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
// updatePredictions
/////////////////////////////////////////////////////////////////////////////////////////////////////

void ApplicationMap::update_predictions()
{
  /////////////////////////////////////////////////////////////////////////////////////////////////////
  // load API key and other configuration values from file
  /////////////////////////////////////////////////////////////////////////////////////////////////////

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

}

