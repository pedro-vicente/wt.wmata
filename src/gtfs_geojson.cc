#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

/////////////////////////////////////////////////////////////////////////////////////////////////////
// ShapePoint
/////////////////////////////////////////////////////////////////////////////////////////////////////

struct ShapePoint
{
  std::string shape_id;
  double lat;
  double lon;
  int sequence;
  double dist_traveled;

  ShapePoint() : lat(0.0), lon(0.0), sequence(0), dist_traveled(0.0) {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Route
/////////////////////////////////////////////////////////////////////////////////////////////////////

struct Route
{
  std::string route_id;
  std::string short_name;
  std::string long_name;
  std::string color;
  std::string text_color;

  Route() {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// remove_quotes
/////////////////////////////////////////////////////////////////////////////////////////////////////

std::string remove_quotes(const std::string& name)
{
  std::string str = name;
  str.erase(std::remove(str.begin(), str.end(), '"'), str.end());
  return str;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// parse_shapes_txt
// Parse GTFS shapes.txt file
//
// Parameters:
//   filename - Path to shapes.txt
//
// Returns:
//   Map of shape_id -> vector of ShapePoints (sorted by sequence)
/////////////////////////////////////////////////////////////////////////////////////////////////////

std::map<std::string, std::vector<ShapePoint>> parse_shapes_txt(const std::string& filename)
{
  std::map<std::string, std::vector<ShapePoint>> shapes;

  std::ifstream file(filename);
  if (!file.is_open())
  {
    return shapes;
  }

  std::string line;
  std::getline(file, line);

  while (std::getline(file, line))
  {
    std::istringstream iss(line);
    std::string token;
    std::vector<std::string> tokens;

    while (std::getline(iss, token, ','))
    {
      tokens.push_back(token);
    }

    if (tokens.size() >= 4)
    {
      ShapePoint pt;
      pt.shape_id = tokens[0];
      pt.lat = std::stod(tokens[1]);
      pt.lon = std::stod(tokens[2]);
      pt.sequence = std::stoi(tokens[3]);

      if (tokens.size() >= 5)
      {
        pt.dist_traveled = std::stod(tokens[4]);
      }

      shapes[pt.shape_id].push_back(pt);
    }
  }

  file.close();

  for (std::map<std::string, std::vector<ShapePoint>>::iterator pair = shapes.begin(); pair != shapes.end(); ++pair)
  {
    std::sort(pair->second.begin(), pair->second.end(),
      [](const ShapePoint& a, const ShapePoint& b) {
        return a.sequence < b.sequence;
      });
  }

  return shapes;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// parse_routes_txt
// Parse GTFS routes.txt file
//
// Parameters:
//   filename - Path to routes.txt
//
// Returns:
//   Map of route_id -> Route details
/////////////////////////////////////////////////////////////////////////////////////////////////////

std::map<std::string, Route> parse_routes_txt(const std::string& filename)
{
  std::map<std::string, Route> routes;

  std::ifstream file(filename);
  if (!file.is_open())
  {
    return routes;
  }

  std::string line;
  std::getline(file, line);

  std::istringstream header_iss(line);
  std::string col;
  std::map<std::string, int> col_index;
  int idx = 0;
  while (std::getline(header_iss, col, ','))
  {
    col_index[col] = idx++;
  }

  while (std::getline(file, line))
  {
    std::istringstream iss(line);
    std::string token;
    std::vector<std::string> tokens;

    while (std::getline(iss, token, ','))
    {
      tokens.push_back(token);
    }

    if (tokens.size() > 0)
    {
      Route route;
      route.route_id = tokens[col_index["route_id"]];

      if (col_index.count("route_short_name"))
      {
        std::string short_name = remove_quotes(tokens[col_index["route_short_name"]]); 
        route.short_name = short_name;
      }

      if (col_index.count("route_long_name"))
        route.long_name = tokens[col_index["route_long_name"]];

      if (col_index.count("route_color"))
        route.color = "#" + tokens[col_index["route_color"]];
      else
        route.color = "#E51636";

      if (col_index.count("route_text_color"))
        route.text_color = "#" + tokens[col_index["route_text_color"]];

      routes[route.route_id] = route;
    }
  }

  file.close();
  return routes;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// parse_trips_txt
// Parse GTFS trips.txt to map shapes to routes
//
// Returns:
//   Map of shape_id -> route_id
/////////////////////////////////////////////////////////////////////////////////////////////////////

std::map<std::string, std::string> parse_trips_txt(const std::string& filename)
{
  std::map<std::string, std::string> shape_to_route;

  std::ifstream file(filename);
  if (!file.is_open())
  {
    return shape_to_route;
  }

  std::string line;
  std::getline(file, line);

  std::istringstream header_iss(line);
  std::string col;
  std::map<std::string, int> col_index;
  int idx = 0;
  while (std::getline(header_iss, col, ','))
  {
    col_index[col] = idx++;
  }

  while (std::getline(file, line))
  {
    std::istringstream iss(line);
    std::string token;
    std::vector<std::string> tokens;

    while (std::getline(iss, token, ','))
    {
      tokens.push_back(token);
    }

    if (tokens.size() > col_index["route_id"] &&
      col_index.count("shape_id") &&
      tokens.size() > col_index["shape_id"])
    {
      std::string route_id = tokens[col_index["route_id"]];
      std::string shape_id = tokens[col_index["shape_id"]];

      if (!shape_id.empty())
      {
        shape_to_route[shape_id] = route_id;
      }
    }
  }

  file.close();
  return shape_to_route;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// generate_geojson_by_line
// Generate separate GeoJSON files for each line
//
// Parameters:
//   shapes         - Map of shape_id -> ShapePoints
//   shape_to_route - Map of shape_id -> route_id
//   routes         - Map of route_id -> Route details
//   output_dir     - Output directory for line files
/////////////////////////////////////////////////////////////////////////////////////////////////////

void generate_geojson_by_line(
  const std::map<std::string, std::vector<ShapePoint>>& shapes,
  const std::map<std::string, std::string>& shape_to_route,
  const std::map<std::string, Route>& routes,
  const std::string& output_dir)
{
  std::map<std::string, std::vector<std::pair<std::string, std::vector<ShapePoint>>>> route_shapes;

  for (std::map<std::string, std::vector<ShapePoint>>::const_iterator shape_pair = shapes.begin(); 
       shape_pair != shapes.end(); ++shape_pair)
  {
    const std::string& shape_id = shape_pair->first;
    const std::vector<ShapePoint>& points = shape_pair->second;

    if (points.empty())
      continue;

    std::string route_id = "";
    if (shape_to_route.count(shape_id))
    {
      route_id = shape_to_route.at(shape_id);
      route_shapes[route_id].push_back(std::make_pair(shape_id, points));
    }
  }

  for (std::map<std::string, std::vector<std::pair<std::string, std::vector<ShapePoint>>>>::const_iterator route_pair = route_shapes.begin();
       route_pair != route_shapes.end(); ++route_pair)
  {
    const std::string& route_id = route_pair->first;
    const std::vector<std::pair<std::string, std::vector<ShapePoint>>>& shapes_list = route_pair->second;

    if (shapes_list.empty())
      continue;

    std::string route_name = route_id;
    std::string color = "#E51636";

    if (routes.count(route_id))
    {
      const Route& route = routes.at(route_id);
      route_name = route.short_name;
      if (route_name.empty())
        route_name = route_id;
      color = route.color;
    }

    std::string route_name_ = remove_quotes(route_name);
    std::string name = "line_" + route_name_;
    std::string filename = output_dir + "/" + name + ".geojson";

    std::ofstream out(filename);
    if (!out.is_open())
    {
      continue;
    }

    out << "{\n";
    out << "  \"type\": \"FeatureCollection\",\n";
    out << "  \"features\": [\n";

    bool first_feature = true;

    for (size_t s = 0; s < shapes_list.size(); ++s)
    {
      const std::string& shape_id = shapes_list[s].first;
      const std::vector<ShapePoint>& points = shapes_list[s].second;

      if (!first_feature)
        out << ",\n";
      first_feature = false;

      out << "    {\n";
      out << "      \"type\": \"Feature\",\n";
      out << "      \"properties\": {\n";
      out << "        \"shape_id\": \"" << shape_id << "\",\n";
      out << "        \"route_id\": \"" << route_id << "\",\n";
      out << "        \"route_name\": \"" << route_name << "\",\n";
      out << "        \"color\": \"" << color << "\"\n";
      out << "      },\n";
      out << "      \"geometry\": {\n";
      out << "        \"type\": \"LineString\",\n";
      out << "        \"coordinates\": [\n";

      for (size_t i = 0; i < points.size(); ++i)
      {
        out << "          [" << points[i].lon << ", " << points[i].lat << "]";
        if (i < points.size() - 1)
          out << ",";
        out << "\n";
      }

      out << "        ]\n";
      out << "      }\n";
      out << "    }";
    }

    out << "\n  ]\n";
    out << "}\n";

    out.close();
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// main
// Generate separate GeoJSON files for each line from GTFS data
// Output: line_BL.geojson, etc.
/////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
  std::string gtfs_dir = "data/gtfs";
  std::string output_dir = "data";
  std::map<std::string, std::vector<ShapePoint>> shapes = parse_shapes_txt(gtfs_dir + "/shapes.txt");
  int total_points = 0;
  for (std::map<std::string, std::vector<ShapePoint>>::const_iterator pair = shapes.begin(); pair != shapes.end(); ++pair)
  {
    total_points += pair->second.size();
  }
  std::map<std::string, Route> routes = parse_routes_txt(gtfs_dir + "/routes.txt");
  std::map<std::string, std::string> shape_to_route = parse_trips_txt(gtfs_dir + "/trips.txt");
  generate_geojson_by_line(shapes, shape_to_route, routes, output_dir);
  return 0;
}

