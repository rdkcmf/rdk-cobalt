/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Liberty Global Service B.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <exception>
#include <memory>
#include "unistd.h"
#include <cstring>

#include "logging.h"
#include "libcobalt.h"
#include "cobalt_rpc_ws.h"
#include "cobalt_rpc_ws_server.h"

#define WSPORT_COBALT_ENV "COBALT_WS_PORT"
#define WSPORT_COBALT_DEFAULT_VALUE 10111

static std::unique_ptr<cobalt_rpc_wsServer> server = nullptr;

/***************************************************************
 *
 * *************************************************************/
int init_ws_interface(void)
{
  DBG_IN();

  long wsPort = 0;
  const char* wsPortSetting;
  wsPortSetting = std::getenv(WSPORT_COBALT_ENV);
  if (wsPortSetting) {
    char* endptr;
    errno = 0;
    wsPort = strtol(wsPortSetting, &endptr, 10);
    if (errno != 0 || *endptr != '\0') {
      DBGE("bad value for port setting "
                    << wsPortSetting << " of " << WSPORT_COBALT_ENV);
      return -1;
    }
  } else {
    wsPort = WSPORT_COBALT_DEFAULT_VALUE;
  }

  if (wsPort <= 0 || wsPort > 65535) {
    if (wsPort == 0) {
      DBG("websocket interface not enabled because port set to 0");
    } else {
      DBGE("websocket interface not enabled. The port out of range value "
                    << wsPortSetting << " " << wsPort);
    }

    return -1;
  }

  try {
    DBG("Starting websocket interface on port " << wsPort);
    server = std::unique_ptr<cobalt_rpc_wsServer>(new cobalt_rpc_wsServer(wsPort));
    server->start();
  } catch (const std::exception& e) {
    server = nullptr;
    DBGE(e.what());
    return -1;
  } catch (...) {
    server = nullptr;
    DBGE("unknown exception");
    return -1;
  }

  return 0;
}

int init_ws_rpc(int argc, char** argv) {

  if (init_ws_interface()) {
    return -1;
  }

  std::vector<const char*> args;
  args.push_back("cobalt-wsrpc");
#ifdef ENABLE_EVERGREEN_LITE
  args.push_back("--evergreen_lite");
#endif
  for (int i=1;i<argc && argv != nullptr;i++) {
    args.push_back(argv[i]);
    if (strncmp(argv[i], "--url=", 6) == 0) {
      server->setUrl(std::string(argv[i]+6));
    }
  }

  std::string args_str;
  for (const char *arg: args) {
    args_str += arg;
    args_str += " ";
  }
  DBG("StarboardMain args: " << args_str);

  server->emitCobaltStateEvent(STARTED, 0);
  auto exitcode = StarboardMain(args.size(), const_cast<char **>(args.data()));
  server->emitCobaltStateEvent(STOPPED, exitcode);
  server = nullptr;
  DBG("Quitting...");

  return 0;
}
