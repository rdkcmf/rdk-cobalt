# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2020 RDK Management
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

import glob
import os
import sys

def EscapePath(path):
  return path.replace(' ', '\\ ')


def List(path):
  output = []

  if not os.path.isdir(path):
    for file in glob.glob(path):
      output.append(file)
    return output

  output.extend(path)
  return output


def Expand(inputs):
  output = []
  for input in inputs:
    list = List(input)
    output.extend(list)
  return output


def DoMain(argv):
  escaped = [EscapePath(x) for x in argv]
  expanded = Expand(escaped)
  return '\n'.join([x for x in expanded])


def main(argv):
  try:
    result = DoMain(argv[1:])
  except e:
    print >> sys.stderr, e
    return 1
  if result:
    print result
  return 0


if __name__ == '__main__':
  sys.exit(main(sys.argv))
