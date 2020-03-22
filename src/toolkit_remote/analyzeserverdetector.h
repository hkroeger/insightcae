/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef INSIGHT_ANALYZESERVERDETECTOR_H
#define INSIGHT_ANALYZESERVERDETECTOR_H

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include "boost/thread.hpp"

namespace insight {

class AnalyzeServerDetector
{
  boost::asio::io_service ios_;
  boost::asio::ip::udp::endpoint listenEndpoint_;
  boost::asio::ip::udp::socket socket_;
  boost::array<char, 65535> buffer_;

  boost::thread serverDetectorThread_;

public:
  struct AnalyzeInstance {
    std::string serverAddress;
    int serverPort;
    std::string analysisName;
  };

  typedef std::vector<AnalyzeInstance> AnalyzeInstanceList;

protected:
  AnalyzeInstanceList detectedInstances_;

  void receiveDone(const boost::system::error_code& /*error*/, std::size_t /*bytes_transferred*/);

public:
  AnalyzeServerDetector(
      int port = 8090
      );
  ~AnalyzeServerDetector();

  static AnalyzeInstanceList detectInstances(int wait_msec, int port=8090);
};

} // namespace insight

#endif // INSIGHT_ANALYZESERVERDETECTOR_H
