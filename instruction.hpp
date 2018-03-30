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


#ifndef N64_EMU_INSTRUCTION_HPP
#define N64_EMU_INSTRUCTION_HPP

#include <cstdint>

namespace n64 {
    namespace instruction {
        constexpr unsigned THREE_ADDRESS=0b011, BINOMIAL=0b010, UNARY=0b001, REGISTER_IMMEDIATE=0b100, NO_OPERAND=0b000;
        constexpr unsigned WIDTH=8;

        struct three_address {
            unsigned instruction:8;
            unsigned type:3;
            unsigned destination:7;
            unsigned source1:7;
            unsigned source2:7;
            unsigned destination_option:10;
            unsigned source1_option:10;
            unsigned source2_option:10;
            unsigned _reserved:2;
        } __attribute__((__packed__));

        struct binomial {
            unsigned instruction:8;
            unsigned type:2;
            unsigned operand1:7;
            unsigned operand2:7;
            unsigned operand1_option:20;
            unsigned operand2_option:20;
        } __attribute__((__packed__));

        struct unary {
            unsigned instruction:8;
            unsigned type:2;
            union {
                struct {
                    unsigned operand:7;
                    unsigned long long operand_option:47;
                } reg;
                struct {
                    unsigned long long immediate:54;
                } imm;
            };
        } __attribute__((__packed__));

        struct register_immediate {
            unsigned instruction:8;
            unsigned reg:7;
            unsigned long long immediate:49;
        } __attribute__((__packed__));

        struct no_operand {
            unsigned instruction:8;
            unsigned long long _reserved:56;
        } __attribute__((__packed__));

        union instruction {
            three_address ta;
            binomial b;
            unary u;
            register_immediate ri;
            no_operand no;
            struct {
                unsigned type:3;
                unsigned instruction:5;
                unsigned long long _reserved:56;
            }__attribute__((__packed__)) instruction;
            std::uint64_t data;
        };
    } /* instruction */

    namespace reg {
        namespace id {
            constexpr std::uint8_t R0=0;
            constexpr std::uint8_t RS[32]={1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32};
            constexpr std::uint8_t RT[32]={33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64};
            constexpr std::uint8_t IP=65, FLAGS=66, SP=67, BP=68;
        } /* id */
    } /* reg */

    std::vector<n64::instruction::instruction> load_binary(const std::string& file);
    void save_binary(const std::string& file, const std::vector<n64::instruction::instruction>& instructions);
} /* n64 */

#endif //N64_EMU_INSTRUCTION_HPP
