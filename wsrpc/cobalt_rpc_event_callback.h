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
#ifndef __COBALT_RPC_EVENT_CALLBACK_H__
#define __COBALT_RPC_EVENT_CALLBACK_H__

#include <exception>
#include <memory>
#include <string>

#include <json/json.h>

#include <rpcserver/IRpcExternalEventListener.h>

using namespace rpcserver;

/***************************************************************
 *
 * *************************************************************/
class cobalt_rpc_event_callback
{
private:
  std::shared_ptr<IRpcExternalEventListener> m_eventListener = nullptr;

  /***************************************************************
   *
   * *************************************************************/
  void emitEvent(const char* eventInfo)
  {
    if (m_eventListener) {
      try {
        Json::Value eventJson;
        JSONCPP_STRING err;

        Json::CharReaderBuilder builder;
        const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        if (!reader->parse(eventInfo, eventInfo + strlen(eventInfo), &eventJson,
                           &err)) {
          DBGE("Error parsing eventinfo " << eventInfo << " : " << err);
        } else {
          std::string eventName = eventJson.begin().key().asString();
          m_eventListener->onEvent(eventName, eventJson);
        }
      } catch (std::exception& e) {
        DBGE("Exception parsing eventinfo " << eventInfo << " : " << e.what());
      }
    }
  }

public:
  /***************************************************************
   *
   * *************************************************************/
  cobalt_rpc_event_callback() {}

  /***************************************************************
   *
   * *************************************************************/
  virtual ~cobalt_rpc_event_callback(void) {}

  /***************************************************************
   *
   * *************************************************************/
  virtual void cobaltEvent(const char* eventInfo)  { emitEvent(eventInfo); }

  /***************************************************************
   *
   * *************************************************************/
  void setEventListener(std::shared_ptr<IRpcExternalEventListener> eventListener)
  {
    m_eventListener = eventListener;
  }
};

#endif
