#ifndef TRANSLATIONS_H
#define TRANSLATIONS_H


#include <libintl.h>

// GETTEXT_DOMAIN comes from the compiler command line
#define _(String) dgettext(GETTEXT_DOMAIN, String)


class GettextInit
{
public:
    enum InitType
    {
        Application, Library
    };

    GettextInit(const char* domain, const char* directory, InitType it);

};

#endif // TRANSLATIONS_H
