#!/bin/bash
##############################################################################
# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2022 Liberty Global Service BV
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##############################################################################

TEMPDIR=tempdir

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

if [ "$1" == "reset" ]
then
  rm -rf $DIR/$TEMPDIR
  exit 0
fi

mkdir -p $DIR/$TEMPDIR

(cd $DIR/$TEMPDIR && \
    [ ! -d Thunder ] && \
    git clone git@github.com:rdkcentral/Thunder.git && \
    (cd Thunder && git checkout 31858b496b97fe41fcffdd341b1d3357836af985) && \
    (cd Thunder && git apply ../../patch/0001-Simplified-docs-generation.patch) \
)

mkdir -p doc
$DIR/$TEMPDIR/Thunder/Tools/JsonGenerator/JsonGenerator.py --simple --docs -o doc *.json

#mkdir -p confluence
#$DIR/$TEMPDIR/Thunder/Tools/JsonGenerator/JsonGenerator.py --confluence --docs -o confluence *.json
