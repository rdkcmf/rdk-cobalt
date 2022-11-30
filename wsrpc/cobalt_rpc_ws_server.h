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
#ifndef __COBALT_RPC_SERVER_H__
#define __COBALT_RPC_SERVER_H__

#include <functional>
#include <memory>
#include <string>

#include <json/json.h>

#include "cobalt_rpc_event_callback.h"
#include "logging.h"

#include <rpcserver/IAbstractRpcServer.h>
#include <rpcserver/IRpcExternalEventListener.h>
#include <rpcserver/WsRpcServerBuilder.h>

#define RPC_METHOD_BASE "com.libertyglobal.rdk.cobalt.1."

using namespace rpcserver;

/***************************************************************
 *
 * *************************************************************/
class cobalt_rpc_wsServer
{
public:
  /***************************************************************
   *
   * *************************************************************/
  explicit cobalt_rpc_wsServer(uint16_t port)
  {
    std::string registerMethodName(RPC_METHOD_REGISTER);
    std::string unregisterMethodName(RPC_METHOD_UNREGISTER);
    std::string getListenersMethodName(RPC_METHOD_GET_LISTENERS);

    WsRpcServerBuilder serverBuilder(port, true);
    wsRpcServer =
      std::shared_ptr<IAbstractRpcServer>(
        serverBuilder
          .enableServerEvents(registerMethodName, unregisterMethodName, getListenersMethodName)
          .numThreads(1)
          .build()
      );

    // Register event callback
    pCallback = std::unique_ptr<cobalt_rpc_event_callback>(new cobalt_rpc_event_callback());
    pCallback->setEventListener(std::dynamic_pointer_cast<IRpcExternalEventListener>(wsRpcServer));
    //Cobalt_RegisterEventHandler(pCallback.get());

    wsRpcServer->bindMethod(RPC_METHOD_TEST,
                            [](const Json::Value& request, Json::Value& response) {
      DBG(RPC_METHOD_TEST << " " << request);
      response = Json::Value();
      response["testreply"] = 101;
      response["testobj"] = Json::Value();
      response["testobj"]["extratest"] = "dummy";
    });
  }

  /***************************************************************
   *
   * *************************************************************/
  cobalt_rpc_wsServer& operator=(const cobalt_rpc_wsServer&) = delete;

  /***************************************************************
   *
   * *************************************************************/
  cobalt_rpc_wsServer(const cobalt_rpc_wsServer&) = delete;

  /***************************************************************
   *
   * *************************************************************/
  virtual ~cobalt_rpc_wsServer()
  {
    stop();
    // Unregister event callback
    //Cobalt_UnregisterEventHandler(pCallback.get());
  }

  /***************************************************************
   *
   * *************************************************************/
  void start() { wsRpcServer->StartListening(); }

  /***************************************************************
   *
   * *************************************************************/
  void stop() { wsRpcServer->StopListening(); }

protected:

  static constexpr auto RPC_METHOD_REGISTER = RPC_METHOD_BASE "register";
  static constexpr auto RPC_METHOD_UNREGISTER = RPC_METHOD_BASE "unregister";
  static constexpr auto RPC_METHOD_GET_LISTENERS = RPC_METHOD_BASE "getListeners";
  static constexpr auto RPC_METHOD_TEST = RPC_METHOD_BASE "test";

  std::shared_ptr<IAbstractRpcServer> wsRpcServer;
  std::unique_ptr<cobalt_rpc_event_callback> pCallback;
};

#endif
