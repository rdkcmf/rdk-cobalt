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
#ifndef WSRPC_LOGGING_H
#define WSRPC_LOGGING_H

#include <iostream>

#define PREFIX "[" <<__FUNCTION__ << "]:[" << __LINE__ <<"] "

#define IF_DBG_ON(dbg...) if (true) { dbg }

#define DBGE(x...)  IF_DBG_ON( std::cout << PREFIX << "[ERROR] " <<  x << std::endl; )
#define DBGW(x...)  IF_DBG_ON( std::cout << PREFIX << "[WARN] " << x <<std::endl; )
#define DBG(x...)   IF_DBG_ON( std::cout << PREFIX << "[DEBUG] " <<  x << std::endl; )

#define DBG_IN() DBG( __FUNCTION__ << " ++" );
#define DBG_OUT() DBG( __FUNCTION__ << " --" );

#endif //WSRPC_LOGGING_H
