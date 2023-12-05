
#include "translations.h"
#include "base/boost_include.h"

GettextInit::GettextInit(const char* domain, const char* directory, InitType it)
{
    //#if WIN32
    //    // LocaleNameToLCID requires a LPCWSTR so we need to convert from char to wchar_t
    //    const auto wStringSize = MultiByteToWideChar(CP_UTF8, 0, locale.data(), static_cast<int>(locale.length()), nullptr, 0);
    //    std::wstring localeName;
    //    localeName.reserve(wStringSize);
    //    MultiByteToWideChar(CP_UTF8, 0, locale.data(), static_cast<int>(locale.length()), localeName.data(), wStringSize);

    //    _configthreadlocale(_DISABLE_PER_THREAD_LOCALE);
    //    const auto localeId = LocaleNameToLCID(localeName.c_str(), LOCALE_ALLOW_NEUTRAL_NAMES);
    //    SetThreadLocale(localeId);
    //#else
    //    setlocale(LC_MESSAGES, locale.data());
    //#endif

    setlocale(LC_NUMERIC, "C");

    boost::filesystem::path tdir;
    if (auto sd=getenv("INSIGHT_INSTDIR"))
    {
        tdir=sd;
        tdir=tdir/"share"/"insight";
    }
    tdir/=directory;

    if (it==Application) textdomain(domain);
    bindtextdomain(domain, tdir.string().c_str());
    bind_textdomain_codeset(domain, "UTF-8");
}



GettextInit toolkit_gettextInit(GETTEXT_DOMAIN, GETTEXT_OUTPUT_DIR, GettextInit::Library);
