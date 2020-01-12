#include "progressrelay.h"
#include <iostream>

ProgressRelay::ProgressRelay()
{}

void ProgressRelay::update(const insight::ProgressState &pi)
{
  Q_EMIT progressUpdate(pi);
}
