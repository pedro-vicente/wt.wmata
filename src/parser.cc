#include <iostream>
#include "geojson.hh"

/////////////////////////////////////////////////////////////////////////////////////////////////////
// main
/////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::cout << "Usage: " << argv[0] << " <geojson_file>" << std::endl;
    return 1;
  }

  const char* file_name = argv[1];
  geojson_t parser;
  int result = parser.convert(file_name);

  if (result != 0)
  {
    return 1;
  }

  std::cout << parser.features.size() << " features" << std::endl << std::endl;

  for (size_t idx = 0; idx < parser.features.size(); ++idx)
  {
    const feature_t& feature = parser.features[idx];

    std::cout << "Feature #" << (idx + 1) << std::endl;
    std::cout << "  Name: " << (feature.name.empty() ? "(unnamed)" : feature.name) << std::endl;
    std::cout << "  Geometries: " << feature.geometry.size() << std::endl;

    for (size_t jdx = 0; jdx < feature.geometry.size(); ++jdx)
    {
      const geometry_t& geometry = feature.geometry[jdx];

      std::cout << "    Geometry #" << (jdx + 1) << ": " << geometry.type << std::endl;
      std::cout << "      Polygons/Rings: " << geometry.polygons.size() << std::endl;

      for (size_t kdx = 0; kdx < geometry.polygons.size(); ++kdx)
      {
        const polygon_t& polygon = geometry.polygons[kdx];
        std::cout << "        Polygon #" << (kdx + 1) << " - Coordinates: " << polygon.coord.size() << std::endl;
      }
    }
    std::cout << std::endl;
  }

  return 0;
}
