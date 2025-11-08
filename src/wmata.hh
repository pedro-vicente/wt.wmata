#include <Wt/WApplication.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WCompositeWidget.h>
#include <string>

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

