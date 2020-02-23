#include "tools.h"

#include <QCoreApplication>
#include <QDir>
#include <QtDebug>


/*! Helper function to get possible location of layout files.
By default the KB_LAYOUT_DIR is used (linux/BSD/macports).
But in some cases (apple bundle) there can be more locations).
*/
QString get_kb_layout_dir()
{
  return QString(":/kb-layouts/");
}

/*! Helper function to get possible location of layout files.
By default the COLORSCHEMES_DIR is used (linux/BSD/macports).
But in some cases (apple bundle) there can be more locations).
*/
QString get_color_schemes_dir()
{
  return QString(":/color-schemes/");
}
