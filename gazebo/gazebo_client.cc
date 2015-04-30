/*
 * Copyright (C) 2015 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <string.h>
#include <vector>
#include <boost/thread/mutex.hpp>

#include "gazebo/transport/transport.hh"
#include "gazebo/common/common.hh"
#include "gazebo/util/LogRecord.hh"
#include "gazebo/gazebo_shared.hh"
#include "gazebo/gazebo_client.hh"

boost::mutex fini_mutex;
std::vector<gazebo::SystemPluginPtr> g_plugins;

/////////////////////////////////////////////////
struct g_vectorStringDup
{
  char *operator()(const std::string &_s)
  {
    return strdup(_s.c_str());
  }
};

/////////////////////////////////////////////////
void gazebo::client::printVersion()
{
  gazebo_shared::printVersion();
}

/////////////////////////////////////////////////
void gazebo::client::addPlugin(const std::string &_filename)
{
  gazebo_shared::addPlugin(_filename, g_plugins);
}

/////////////////////////////////////////////////
bool gazebo::client::setup(int _argc, char **_argv)
{
  if (!gazebo_shared::setup("client-", _argc, _argv, g_plugins))
  {
    gzerr << "Unable to setup Gazebo\n";
    return false;
  }

  common::Time waitTime(1, 0);
  int waitCount = 0;
  int maxWaitCount = 10;

  // Wait for namespaces.
  while (!gazebo::transport::waitForNamespaces(waitTime) &&
      (waitCount++) < maxWaitCount)
  {
    gzwarn << "Waited " << waitTime.Double() << "seconds for namespaces.\n";
  }

  if (waitCount >= maxWaitCount)
  {
    gzerr << "Waited " << (waitTime * waitCount).Double()
      << " seconds for namespaces. Giving up.\n";
  }

  return true;
}

/////////////////////////////////////////////////
bool gazebo::client::setup(const std::vector<std::string> &_args)
{
  std::vector<char *> pointers(_args.size());
  std::transform(_args.begin(), _args.end(), pointers.begin(),
                 g_vectorStringDup());
  pointers.push_back(0);
  bool result = gazebo::client::setup(_args.size(), &pointers[0]);

  // Deallocate memory for the command line arguments allocated with strdup.
  for (size_t i = 0; i < pointers.size(); ++i)
    free(pointers.at(i));

  return result;
}

/////////////////////////////////////////////////
bool gazebo::client::shutdown()
{
  // Stop log recording
  util::LogRecord::Instance()->Stop();

  // Stop transport
  gazebo::transport::stop();

  // Make sure to shut everything down.
  boost::mutex::scoped_lock lock(fini_mutex);
  util::LogRecord::Instance()->Fini();
  g_plugins.clear();
  gazebo::transport::fini();

  // Cleanup model database.
  common::ModelDatabase::Instance()->Fini();

  return true;
}
