#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <fstream>
#include <sstream>
#include "geojson.hh"

const int SHIFT_WIDTH = 4;
bool DATA_NEWLINE = false;
bool OBJECT_NEWLINE = false;

/////////////////////////////////////////////////////////////////////////////////////////////////////
//geojson_t::convert
/////////////////////////////////////////////////////////////////////////////////////////////////////

int geojson_t::convert(const char* file_name)
{
  std::ifstream file(file_name);
  if (!file.is_open())
  {
    return -1;
  }

  std::stringstream buf;
  buf << file.rdbuf();
  std::string json = buf.str();
  file.close();

  try
  {
    Wt::Json::Object root;
    Wt::Json::parse(json, root);
    parse_root(root);
    return 0;
  }
  catch (const Wt::Json::ParseError& e)
  {
    std::cout << e.what() << std::endl;
    return -1;
  }
  catch (const Wt::Json::TypeException& e)
  {
    std::cout << e.what() << std::endl;
    return -1;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//geojson_t::parse_root
/////////////////////////////////////////////////////////////////////////////////////////////////////

int geojson_t::parse_root(const Wt::Json::Object& root)
{
  //JSON organized in hierarchical levels
  //level 0, root with objects: "type", "features"
  //FeatureCollection is not much more than an object that has "type": "FeatureCollection" 
  //and then an array of Feature objects under the key "features". 

  if (root.contains("type"))
  {
    const Wt::Json::Value& value = root.get("type");
    if (value.type() == Wt::Json::Type::String)
    {
      std::string str = static_cast<Wt::WString>(value).toUTF8();
      if (str == "Feature")
      {
        //parse the root, contains only one "Feature"
        parse_feature(root);
      }
      else if (str == "FeatureCollection")
      {
        //has "features" array
        if (root.contains("features"))
        {
          const Wt::Json::Value& features = root.get("features");
          if (features.type() == Wt::Json::Type::Array)
          {
            const Wt::Json::Array& features_array = static_cast<const Wt::Json::Array&>(features);
            parse_features(features_array);
          }
        }
      }
    }
  }

  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//geojson_t::parse_features
//array of Feature objects under the key "features". 
/////////////////////////////////////////////////////////////////////////////////////////////////////

int geojson_t::parse_features(const Wt::Json::Array& features)
{
  for (size_t idx = 0; idx < features.size(); ++idx)
  {
    const Wt::Json::Value& value = features[idx];
    if (value.type() == Wt::Json::Type::Object)
    {
      const Wt::Json::Object& feature = static_cast<const Wt::Json::Object&>(value);
      parse_feature(feature);
    }
  }

  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//geojson_t::parse_feature
/////////////////////////////////////////////////////////////////////////////////////////////////////

int geojson_t::parse_feature(const Wt::Json::Object& obj)
{
  feature_t feature;

  //3 objects with keys: 
  // "type", 
  // "properties", 
  // "geometry"
  //"type" has a string value "Feature"
  //"properties" has a list of objects
  //"geometry" has 2 objects: 
  //key "type" with value string geometry type (e.g."Polygon") and
  //key "coordinates" an array

  if (obj.contains("type"))
  {
    const Wt::Json::Value& type = obj.get("type");
    assert(type.type() == Wt::Json::Type::String);
  }

  if (obj.contains("properties"))
  {
    const Wt::Json::Value& properties_value = obj.get("properties");
    if (properties_value.type() == Wt::Json::Type::Object)
    {
      const Wt::Json::Object& properties_object = static_cast<const Wt::Json::Object&>(properties_value);

      //parse properties - look for NAME or name
      if (properties_object.contains("NAME"))
      {
        const Wt::Json::Value& name_value = properties_object.get("NAME");
        if (name_value.type() == Wt::Json::Type::String)
        {
          feature.name = static_cast<Wt::WString>(name_value).toUTF8();
        }
      }
      else if (properties_object.contains("name"))
      {
        const Wt::Json::Value& name_value = properties_object.get("name");
        if (name_value.type() == Wt::Json::Type::String)
        {
          feature.name = static_cast<Wt::WString>(name_value).toUTF8();
        }
      }
    }
  }

  if (obj.contains("geometry"))
  {
    const Wt::Json::Value& geometry_value = obj.get("geometry");
    if (geometry_value.type() == Wt::Json::Type::Object)
    {
      const Wt::Json::Object& geometry_object = static_cast<const Wt::Json::Object&>(geometry_value);
      parse_geometry(geometry_object, feature);
    }
  }

  features.push_back(feature);
  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//geojson_t::parse_geometry
//"geometry" has 2 objects: 
//key "type" with value string geometry type (e.g."Polygon") and
//key "coordinates" an array
/////////////////////////////////////////////////////////////////////////////////////////////////////

int geojson_t::parse_geometry(const Wt::Json::Object& geometry_object, feature_t& feature)
{
  std::string type; //"Polygon", "MultiPolygon", "Point"

  if (geometry_object.contains("type"))
  {
    const Wt::Json::Value& type_value = geometry_object.get("type");
    if (type_value.type() == Wt::Json::Type::String)
    {
      type = static_cast<Wt::WString>(type_value).toUTF8();
    }
  }

  if (geometry_object.contains("coordinates"))
  {
    const Wt::Json::Value& value = geometry_object.get("coordinates");
    if (value.type() == Wt::Json::Type::Array)
    {
      const Wt::Json::Array& coordinates = static_cast<const Wt::Json::Array&>(value);

      if (type == "Point")
      {
        /////////////////////////////////////////////////////////////////////////////////////////////////////
        //store geometry locally for points
        /////////////////////////////////////////////////////////////////////////////////////////////////////

        geometry_t geometry;
        geometry.type = type;

        polygon_t polygon;

        if (coordinates.size() >= 2)
        {
          const Wt::Json::Value& lon_value = coordinates[0];
          const Wt::Json::Value& lat_value = coordinates[1];

          double lon = 0.0;
          double lat = 0.0;

          if (lon_value.type() == Wt::Json::Type::Number)
          {
            lon = static_cast<double>(lon_value);
          }
          if (lat_value.type() == Wt::Json::Type::Number)
          {
            lat = static_cast<double>(lat_value);
          }

          coord_t coord(lon, lat);
          polygon.coord.push_back(coord);
        }

        geometry.polygons.push_back(polygon);
        feature.geometry.push_back(geometry);
      }

      /////////////////////////////////////////////////////////////////////////////////////////////////////
      //store geometry locally for LineString
      /////////////////////////////////////////////////////////////////////////////////////////////////////

      if (type == "LineString")
      {
        geometry_t geometry;
        geometry.type = type;

        polygon_t polygon;

        // LineString coordinates are just an array of [lon, lat] pairs
        for (size_t idx = 0; idx < coordinates.size(); ++idx)
        {
          const Wt::Json::Value& coord_value = coordinates[idx];
          if (coord_value.type() == Wt::Json::Type::Array)
          {
            const Wt::Json::Array& coord_array = static_cast<const Wt::Json::Array&>(coord_value);

            if (coord_array.size() >= 2)
            {
              const Wt::Json::Value& lon_value = coord_array[0];
              const Wt::Json::Value& lat_value = coord_array[1];

              double lon = 0.0;
              double lat = 0.0;

              if (lon_value.type() == Wt::Json::Type::Number)
              {
                lon = static_cast<double>(lon_value);
              }
              if (lat_value.type() == Wt::Json::Type::Number)
              {
                lat = static_cast<double>(lat_value);
              }

              coord_t coord(lon, lat);
              polygon.coord.push_back(coord);
            }
          }
        }

        geometry.polygons.push_back(polygon);
        feature.geometry.push_back(geometry);
      }

      /////////////////////////////////////////////////////////////////////////////////////////////////////
      //store geometry in parse_coordinates() for polygons
      /////////////////////////////////////////////////////////////////////////////////////////////////////

      if (type == "Polygon" || type == "MultiPolygon")
      {
        parse_coordinates(coordinates, type, feature);
      }
    }
  }

  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//geojson_t::parse_coordinates
//"parse_coordinates" 
//for "Polygon"
//is an array of size 1 that contains another array and then an array of 2 numbers (lat, lon)
//for "MultiPolygon"
//is an array that contains another array of size 1, that contains another array,
//and then an array of 2 numbers (lat, lon)
/////////////////////////////////////////////////////////////////////////////////////////////////////

int geojson_t::parse_coordinates(const Wt::Json::Array& coordinates, const std::string& type, feature_t& feature)
{
  geometry_t geometry;
  geometry.type = type;

  if (type == "Polygon")
  {
    // Array of rings (first is exterior, rest are holes)
    for (size_t idx = 0; idx < coordinates.size(); ++idx)
    {
      const Wt::Json::Value& value = coordinates[idx];
      if (value.type() == Wt::Json::Type::Array)
      {
        const Wt::Json::Array& ring = static_cast<const Wt::Json::Array&>(value);

        polygon_t polygon;
        for (size_t jdx = 0; jdx < ring.size(); ++jdx)
        {
          const Wt::Json::Value& coord_value = ring[jdx];
          if (coord_value.type() == Wt::Json::Type::Array)
          {
            const Wt::Json::Array& coord = static_cast<const Wt::Json::Array&>(coord_value);

            if (coord.size() >= 2)
            {
              const Wt::Json::Value& lon_value = coord[0];
              const Wt::Json::Value& lat_value = coord[1];

              double lon = 0.0;
              double lat = 0.0;

              if (lon_value.type() == Wt::Json::Type::Number)
              {
                lon = static_cast<double>(lon_value);
              }
              if (lat_value.type() == Wt::Json::Type::Number)
              {
                lat = static_cast<double>(lat_value);
              }

              coord_t coord(lon, lat);
              polygon.coord.push_back(coord);
            }
          }
        }
        geometry.polygons.push_back(polygon);
      }
    }
  }

  if (type == "MultiPolygon")
  {
    // Array of polygons, each polygon is an array of rings
    for (size_t idx = 0; idx < coordinates.size(); ++idx)
    {
      const Wt::Json::Value& polygon_value = coordinates[idx];
      if (polygon_value.type() == Wt::Json::Type::Array)
      {
        const Wt::Json::Array& polygon = static_cast<const Wt::Json::Array&>(polygon_value);

        // Each polygon has rings
        for (size_t jdx = 0; jdx < polygon.size(); ++jdx)
        {
          const Wt::Json::Value& ring = polygon[jdx];
          if (ring.type() == Wt::Json::Type::Array)
          {
            const Wt::Json::Array& ring_array = static_cast<const Wt::Json::Array&>(ring);

            polygon_t polygon;
            for (size_t kdx = 0; kdx < ring_array.size(); ++kdx)
            {
              const Wt::Json::Value& coord_value = ring_array[kdx];
              if (coord_value.type() == Wt::Json::Type::Array)
              {
                const Wt::Json::Array& coord_array = static_cast<const Wt::Json::Array&>(coord_value);

                if (coord_array.size() >= 2)
                {
                  const Wt::Json::Value& lon_value = coord_array[0];
                  const Wt::Json::Value& lat_value = coord_array[1];

                  double lon = 0.0;
                  double lat = 0.0;

                  if (lon_value.type() == Wt::Json::Type::Number)
                  {
                    lon = static_cast<double>(lon_value);
                  }
                  if (lat_value.type() == Wt::Json::Type::Number)
                  {
                    lat = static_cast<double>(lat_value);
                  }

                  coord_t coord(lon, lat);
                  polygon.coord.push_back(coord);
                }
              }
            }
            geometry.polygons.push_back(polygon);
          }
        }
      }
    }
  }

  //store
  feature.geometry.push_back(geometry);
  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//geojson_t::dump_value
/////////////////////////////////////////////////////////////////////////////////////////////////////

int geojson_t::dump_value(const Wt::Json::Value& value, int indent)
{
  switch (value.type())
  {
  case Wt::Json::Type::Number:
    fprintf(stdout, "%f", static_cast<double>(value));
    break;

  case Wt::Json::Type::String:
    dump_string(static_cast<Wt::WString>(value).toUTF8());
    break;

  case Wt::Json::Type::Array:
  {
    const Wt::Json::Array& arr = static_cast<const Wt::Json::Array&>(value);
    if (arr.size() == 0)
    {
      fprintf(stdout, "[]");
      break;
    }
    fprintf(stdout, "[");
    if (DATA_NEWLINE) fprintf(stdout, "\n");

    for (size_t i = 0; i < arr.size(); ++i)
    {
      if (DATA_NEWLINE) fprintf(stdout, "%*s", indent + SHIFT_WIDTH, "");
      dump_value(arr[i], indent + SHIFT_WIDTH);
      if (DATA_NEWLINE)
        fprintf(stdout, (i < arr.size() - 1) ? ",\n" : "\n");
      else
        fprintf(stdout, (i < arr.size() - 1) ? "," : "");
    }

    if (DATA_NEWLINE)
      fprintf(stdout, "%*s]", indent, "");
    else
      fprintf(stdout, "]");
  }
  break;

  case Wt::Json::Type::Object:
  {
    const Wt::Json::Object& obj = static_cast<const Wt::Json::Object&>(value);
    std::set<std::string> keys = obj.names();

    if (keys.empty())
    {
      fprintf(stdout, "{}");
      break;
    }

    fprintf(stdout, "{\n");
    size_t count = 0;
    for (const auto& key : keys)
    {
      fprintf(stdout, "%*s", indent + SHIFT_WIDTH, "");
      dump_string(key);
      fprintf(stdout, ": ");
      dump_value(obj.get(key), indent + SHIFT_WIDTH);
      fprintf(stdout, (count < keys.size() - 1) ? ",\n" : "\n");
      count++;
    }
    fprintf(stdout, "%*s}", indent, "");
  }
  break;

  case Wt::Json::Type::Bool:
    fprintf(stdout, static_cast<bool>(value) ? "true" : "false");
    break;

  case Wt::Json::Type::Null:
    fprintf(stdout, "null");
    break;
  }

  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//geojson_t::dump_string
/////////////////////////////////////////////////////////////////////////////////////////////////////

void geojson_t::dump_string(const std::string& s)
{
  fputc('"', stdout);
  for (char c : s)
  {
    switch (c)
    {
    case '\b':
      fprintf(stdout, "\\b");
      break;
    case '\f':
      fprintf(stdout, "\\f");
      break;
    case '\n':
      fprintf(stdout, "\\n");
      break;
    case '\r':
      fprintf(stdout, "\\r");
      break;
    case '\t':
      fprintf(stdout, "\\t");
      break;
    case '\\':
      fprintf(stdout, "\\\\");
      break;
    case '"':
      fprintf(stdout, "\\\"");
      break;
    default:
      fputc(c, stdout);
    }
  }
  fputc('"', stdout);
}
