# Copyright 2018 SiLeader.
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

cmake_minimum_required(VERSION 3.10)
project(n64_emu)

set(CMAKE_CXX_STANDARD 17)

add_executable(n64emu emulator_main.cpp instruction.hpp binary.cpp cmdline.hpp)
add_executable(n64as assembler_main.cpp instruction.hpp binary.cpp cmdline.hpp)
