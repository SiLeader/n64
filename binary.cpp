// Copyright 2018 SiLeader.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//         http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <vector>
#include <string>
#include <fstream>

#include <endian.h>

#include "instruction.hpp"


namespace n64 {
    /**
     * load from binary file.
     * @param file input file name
     * @return instructions in the input file
     */
    std::vector<n64::instruction::instruction> load_binary(const std::string& file) {
        std::fstream fin(file, std::ios::in | std::ios::binary);

        std::vector<n64::instruction::instruction> instructions;

        while(!fin.eof()) {
            n64::instruction::instruction ins={};
            fin.read(reinterpret_cast<char*>(&ins), n64::instruction::WIDTH);
            ins.data=be64toh(ins.data);
            instructions.emplace_back(ins);
        }
        return instructions;
    }

    /**
     * save to binary file.
     * @param file output file name
     * @param instructions instructions
     */
    void save_binary(const std::string& file, const std::vector<n64::instruction::instruction>& instructions) {
        std::fstream fout(file, std::ios::out | std::ios::binary);

        for(const auto& ins : instructions) {
            std::uint64_t be=htobe64(ins.data);
            fout.write(reinterpret_cast<const char*>(&be), n64::instruction::WIDTH);
        }
    }
} /* n64 */
