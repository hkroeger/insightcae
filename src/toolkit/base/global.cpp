
#include <locale>

class GlobalSettings
{
public:
 GlobalSettings()
 {
     setlocale(LC_NUMERIC, "C");
 }
} insight_perform_GlobalSettings;
