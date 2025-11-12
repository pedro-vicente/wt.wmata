#include <Wt/WApplication.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WCompositeWidget.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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
// TrainPosition
/////////////////////////////////////////////////////////////////////////////////////////////////////

struct TrainPosition
{
  double Lng;
  double Lat;
  std::string Destination;
  std::string LocationName;
  std::string Min;
  std::string Car;
  std::string LineColor;
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
