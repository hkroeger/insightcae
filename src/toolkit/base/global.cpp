
#include <locale>

class GlobalSettings
{
public:
 GlobalSettings()
 {
  std::locale::global(std::locale::classic());
 }
} insight_perform_GlobalSettings;
