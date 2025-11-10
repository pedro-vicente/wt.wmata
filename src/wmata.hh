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
#include <iostream>
#include <chrono>
#include <string>

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
// Prediction
/////////////////////////////////////////////////////////////////////////////////////////////////////

struct Prediction
{
  std::string Car;
  std::string Destination;
  std::string DestinationCode;
  std::string Group;
  std::string Line;
  std::string LocationCode;
  std::string LocationName;
  std::string Min;

  Prediction(const std::string& car, const std::string& destination, const std::string& dest_code,
    const std::string& group, const std::string& line, const std::string& loc_code,
    const std::string& loc_name, const std::string& min_str)
    : Car(car), Destination(destination), DestinationCode(dest_code), Group(group),
    Line(line), LocationCode(loc_code), LocationName(loc_name), Min(min_str) {
  }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// globals
/////////////////////////////////////////////////////////////////////////////////////////////////////

void parse_stations(const std::string& buf, bool clear = false);
void parse_predictions(const std::string& buf);
std::string fetch_predictions(const std::string& api_key);

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
  Wt::WTimer* timer;
  void update_predictions();
};

