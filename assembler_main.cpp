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

#include <iostream>
#include <fstream>
#include <algorithm>
#include <unordered_map>
#include <regex>

#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>
#include <boost/preprocessor.hpp>

#include "cmdline.hpp"

#include "instruction.hpp"

namespace {
    using instruction_info_t=std::tuple<std::uint8_t, unsigned>;
    using instruction_map_t=std::unordered_map<std::string, instruction_info_t>;
    using register_map_t=std::unordered_map<std::string, std::uint8_t>;
    using system_info_t=std::tuple<instruction_map_t, register_map_t>;
    constexpr int INSTRUCTION_TYPE_NUMBER=1, INSTRUCTION_NUMBER=0, INSTRUCTION_TYPE=1, INSTRUCTION_MAP=0, REGISTER_MAP=1;

    /**
     * read from source file
     * @param input_file source file
     * @return file content
     */
    std::string read_file(const std::string& input_file) {
        std::ifstream fin(input_file);
        if(fin.fail()) {
            std::cerr<<"error: cannot open source file."<<std::endl;
            exit(EXIT_FAILURE);
        }

        return std::string(std::istreambuf_iterator<char>(fin), std::istreambuf_iterator<char>());
    }

    /**
     * reformat source code
     * @param content source code
     * @return formatted source code
     */
    std::vector<std::string> reformat_data(std::string content) {
        namespace ba=boost::algorithm;

        // convert to lower case
        std::transform(std::begin(content), std::end(content), std::begin(content), ::tolower);

        // remove useless spaces
        content=std::regex_replace(content, std::regex(R"(\s*,\s*)"), ",");
        content=std::regex_replace(content, std::regex(R"(\s*:\s*)"), ":");

        // split lines
        std::vector<std::string> lines;
        ba::split(lines, content, boost::is_any_of("\n;"));
        for(auto& line : lines) {
            // remove useless spaces
            ba::trim(line);
        }

        // remove empty line
        std::remove_if(std::begin(lines), std::end(lines), [](const std::string& line) {
            return line.empty();
        });

        return lines;
    }

    /**
     * decode operand from operand string
     * @param reg_map register map (register name -> register number)
     * @param operand operand string
     * @param index line index
     * @return register number, register option, register type
     */
    std::tuple<std::uint8_t /* register */, std::uint64_t /* option */, std::uint8_t /* type */>
            decode_operand(const register_map_t& reg_map, std::string operand, std::size_t index) {
        namespace ba=boost::algorithm;

        std::uint8_t reg=0;
        std::uint64_t option=0;
        std::uint8_t type=0;

        if(operand[0]=='[') {
            // pointer
            operand=std::regex_replace(std::regex_replace(operand, std::regex(R"(\[\s*)"), ""), std::regex(R"(\s*\])"), "");
            std::vector<std::string> op;
            ba::split(op, operand, boost::is_any_of("+"));
            for(auto& o : op) {
                ba::trim(o);
            }
            if(reg_map.count(op[0])<=0) {
                std::cerr<<"error: unknown operand \""<<op[0]<<"\". near line "<<(index+1)<<std::endl;
                exit(EXIT_FAILURE);
            }
            reg=reg_map.at(op[0]);

            if(op.size()>1) {
                option=std::stoull(op[1]);
            }
            type=1U;
        }else{
            // register
            std::vector<std::string> op;
            ba::split(op, operand, boost::is_any_of(" "));
            for(auto& o : op) {
                ba::trim(o);
            }

            // size is specified
            if(op.size()>2) {
                if(op[0]=="byte") {
                    option=1;
                }else if(op[0]=="word") {
                    option=2;
                }else if(op[0]=="dword") {
                    option=3;
                }
            }
            const auto& re=op[op.size()-1];
            if(reg_map.count(re)<=0) {
                std::cerr<<"error: unknown operand \""<<op[0]<<"\". near line "<<(index+1)<<std::endl;
                exit(EXIT_FAILURE);
            }
            reg=reg_map.at(re);
        }

        return std::tuple<std::uint8_t, std::uint64_t, std::uint8_t>(reg, option, type);
    };

    /**
     * assemble one instruction
     * @param sys_info system information (register map and instruction map)
     * @param lines all lines
     * @param index current line index
     * @return assembled instruction (if boost::none, label only line)
     */
    boost::optional<n64::instruction::instruction> assemble_line(const system_info_t& sys_info, const std::vector<std::string>& lines, std::size_t index) {
        namespace ba=boost::algorithm;

        // support functions
        const auto operand_count_check=[&index](std::size_t count, std::size_t passed) {
            if(passed!=count) {
                std::cerr
                        <<"error: operands count mismatch. 3 operands expected. near line "<<(index+1)<<"\n"
                        <<"       you passed "<<passed<<" operands."<<std::endl;
                exit(EXIT_FAILURE);
            }
        };
        const auto decode_immediate=[](std::string num) -> std::uint64_t {
            int base=10;
            if(num[0]=='0' && num.size()!=1) {
                if(num[1]=='x') {
                    num=num.substr(2);
                    base=16;
                }else{
                    num=num.substr(1);
                    base=8;
                }
            }
            return std::stoull(num, nullptr, base);
        };
        const auto get_absolute_label_address=[&lines, &index](std::string label) -> std::uint64_t {
            label+=":";
            const auto label_length=label.size();

            auto itr=std::find_if(std::begin(lines), std::end(lines), [&label, &label_length](const std::string& line) {
                if(label_length>line.size())return false;
                for(std::size_t i=0; i<label_length; ++i) {
                    if(label[i]!=line[i])return false;
                }
                return true;
            });
            if(itr==std::end(lines)) {
                label.pop_back();
                std::cerr<<"error: undefined identifier \""<<label<<"\". near line "<<(index+1)<<std::endl;
                exit(EXIT_FAILURE);
            }

            return static_cast<std::uint64_t>(itr-std::begin(lines));
        };
        // jump or call operators
        static const std::vector<std::string> JUMP_TYPE_OPERATORS={
                "call", "jmp", "jr", "je", "jne", "ja", "jae", "jb", "jbe"
        };
        // decimal, octal or hexadecimal regex
        static const std::regex NUMBERS(R"(0x[0-9a-fA-F]+|0[0-7]+|[0-9][1-9]*|0)");

        // instruction info and register info
        const auto& im=std::get<INSTRUCTION_MAP>(sys_info);
        const auto& rm=std::get<REGISTER_MAP>(sys_info);

        // // // // // // // // // // // // // // // // // // // // // // // // // // // // //
        auto line=lines[index];

        std::vector<std::string> parts;

        /* check line has label */ {
            auto colon_index=line.find(':');
            if(colon_index!=std::string::npos) {
                line=line.substr(colon_index+1);
            }
            boost::algorithm::trim(line);
            if(line.empty())return boost::none;
        }

        ba::split(parts, line, boost::is_any_of(" "));

        // process pseudo-instructions
        if(parts[0]=="nop") {
            parts[0]="xchg";
            parts.emplace_back("r0,r0");
        }else if(parts[0]=="raise") {
            parts[0]="cmp";
            parts.emplace_back("r0,r0");
        }else if(parts[0]=="mov") {
            std::vector<std::string> operand;
            ba::split(operand, parts[1], boost::is_any_of(","));

            operand_count_check(2, operand.size());

            parts[0]="add";
            parts[1]=operand[0]+","+operand[1]+",r0";
        }

        if(im.count(parts[0])<=0) {
            std::cerr<<"error: unknown instruction \""<<parts[0]<<"\". near line "<<(index+1)<<std::endl;
            exit(EXIT_FAILURE);
        }

        // set instruction opcode
        n64::instruction::instruction ins={};
        ins.instruction.type=std::get<INSTRUCTION_TYPE_NUMBER>(im.at(parts[0]));
        ins.instruction.instruction=std::get<INSTRUCTION_NUMBER>(im.at(parts[0]));
        auto type=std::get<INSTRUCTION_TYPE>(im.at(parts[0]));

        std::vector<std::string> operand;
        if(type!=n64::instruction::NO_OPERAND) {
            ba::split(operand, parts[1], boost::is_any_of(","));
        }

        switch(type) {
            case n64::instruction::THREE_ADDRESS:{
                operand_count_check(3, operand.size());

                auto[rd, od, td] = decode_operand(rm, operand[0], index);
                ins.ta.destination=rd;
                ins.ta.destination_option=static_cast<unsigned>(od);
                ins.ta.type |= (td<<2);

                auto[rs1, os1, ts1] = decode_operand(rm, operand[1], index);
                ins.ta.source1=rs1;
                ins.ta.source1_option=static_cast<unsigned>(os1);
                ins.ta.type |= (ts1<<1);

                auto[rs2, os2, ts2] = decode_operand(rm, operand[2], index);
                ins.ta.source2=rs2;
                ins.ta.source2_option=static_cast<unsigned>(os2);
                ins.ta.type |= ts2;
            }
                break;
            case n64::instruction::BINOMIAL:{
                operand_count_check(2, operand.size());

                auto[o1, oo1, ot1] =decode_operand(rm, operand[0], index);
                ins.b.operand1=o1;
                ins.b.operand1_option=static_cast<unsigned>(oo1);
                ins.b.type |= (ot1<<1);

                auto[o2, oo2, ot2] =decode_operand(rm, operand[1], index);
                ins.b.operand2=o2;
                ins.b.operand2_option=static_cast<unsigned>(oo2);
                ins.b.type |= ot2;
            }
                break;
            case n64::instruction::UNARY:{
                operand_count_check(1, operand.size());

                if(std::regex_match(operand[0], NUMBERS)) {
                    ins.u.imm.immediate=decode_immediate(operand[0]);
                    ins.u.type=0b11;
                }else{
                    if(std::find(std::begin(JUMP_TYPE_OPERATORS), std::end(JUMP_TYPE_OPERATORS), parts[0])!=std::end(JUMP_TYPE_OPERATORS)) {
                        auto absolute_address=get_absolute_label_address(operand[0]);
                        ins.u.imm.immediate=absolute_address;
                        ins.u.type=0b11;
                    }else{
                        auto[r, ro, rt]=decode_operand(rm, operand[0], index);
                        ins.u.reg.operand=r;
                        ins.u.reg.operand_option=ro;
                        ins.u.type=rt;
                    }
                }
            }
                break;
            case n64::instruction::REGISTER_IMMEDIATE:{
                operand_count_check(2, operand.size());

                auto r=std::get<0>(decode_operand(rm, operand[0], index));
                ins.ri.reg=r;

                if(!std::regex_match(operand[1], NUMBERS)) {
                    std::cerr<<"error: operand 2 must be immediate. near line "<<(index+1)<<std::endl;
                    exit(EXIT_FAILURE);
                }
                ins.ri.immediate=decode_immediate(operand[1]);
            }
                break;
        }

        return ins;
    }

    /**
     * assemble all lines
     * @param sys_info system information
     * @param input_file input file name
     * @return assembled instruction
     */
    std::vector<n64::instruction::instruction> assemble(const system_info_t& sys_info, const std::string& input_file) {
        auto lines=reformat_data(read_file(input_file));

        std::vector<n64::instruction::instruction> instructions;
        for(std::size_t i=0; i<lines.size(); ++i) {
            auto line=assemble_line(sys_info, lines, i);
            if(line) {
                instructions.emplace_back(line.get());
            }
        }
        return instructions;
    }
} /* anonymous */

int main(int argc, char **argv) {
    std::cout<<"N64 Assembler"<<std::endl;
    std::cout<<"Version: "<<1<<std::endl;

    cmdline::parser parser;
    parser.add<std::string>("output", 'o', "output file", false, "a.n64");

    parser.parse_check(argc, argv);
    auto output=parser.get<std::string>("output");
    auto input=parser.rest()[0];

    // instructions
    instruction_map_t instruction_map={
            {"add",   std::tuple<std::uint8_t, unsigned>(0b00000, n64::instruction::THREE_ADDRESS)},
            {"sub",   std::tuple<std::uint8_t, unsigned>(0b00001, n64::instruction::THREE_ADDRESS)},
            {"mul",   std::tuple<std::uint8_t, unsigned>(0b00010, n64::instruction::THREE_ADDRESS)},
            {"div",   std::tuple<std::uint8_t, unsigned>(0b00011, n64::instruction::THREE_ADDRESS)},
            {"shr",   std::tuple<std::uint8_t, unsigned>(0b00100, n64::instruction::THREE_ADDRESS)},
            {"shl",   std::tuple<std::uint8_t, unsigned>(0b00101, n64::instruction::THREE_ADDRESS)},
            {"inc",   std::tuple<std::uint8_t, unsigned>(0b00000, n64::instruction::UNARY)},
            {"dec",   std::tuple<std::uint8_t, unsigned>(0b00001, n64::instruction::UNARY)},
            {"not",   std::tuple<std::uint8_t, unsigned>(0b00000, n64::instruction::BINOMIAL)},
            {"and",   std::tuple<std::uint8_t, unsigned>(0b00110, n64::instruction::THREE_ADDRESS)},
            {"or",    std::tuple<std::uint8_t, unsigned>(0b00111, n64::instruction::THREE_ADDRESS)},
            {"xor",   std::tuple<std::uint8_t, unsigned>(0b01000, n64::instruction::THREE_ADDRESS)},

            {"call",  std::tuple<std::uint8_t, unsigned>(0b00010, n64::instruction::UNARY)},
            {"jmp",   std::tuple<std::uint8_t, unsigned>(0b00011, n64::instruction::UNARY)},
            {"jr",    std::tuple<std::uint8_t, unsigned>(0b00100, n64::instruction::UNARY)},

            {"je",    std::tuple<std::uint8_t, unsigned>(0b00101, n64::instruction::UNARY)},
            {"jne",   std::tuple<std::uint8_t, unsigned>(0b00110, n64::instruction::UNARY)},
            {"ja",    std::tuple<std::uint8_t, unsigned>(0b00111, n64::instruction::UNARY)},
            {"jae",   std::tuple<std::uint8_t, unsigned>(0b01000, n64::instruction::UNARY)},
            {"jb",    std::tuple<std::uint8_t, unsigned>(0b01001, n64::instruction::UNARY)},
            {"jbe",   std::tuple<std::uint8_t, unsigned>(0b01010, n64::instruction::UNARY)},

            {"push",  std::tuple<std::uint8_t, unsigned>(0b01011, n64::instruction::UNARY)},
            {"pop",   std::tuple<std::uint8_t, unsigned>(0b01100, n64::instruction::UNARY)},

            {"hlt",   std::tuple<std::uint8_t, unsigned>(0b00000, n64::instruction::NO_OPERAND)},
            {"xchg",  std::tuple<std::uint8_t, unsigned>(0b00001, n64::instruction::BINOMIAL)},
            {"ret",   std::tuple<std::uint8_t, unsigned>(0b00001, n64::instruction::NO_OPERAND)},
            {"cmp",   std::tuple<std::uint8_t, unsigned>(0b00010, n64::instruction::BINOMIAL)},
            {"asgn",  std::tuple<std::uint8_t, unsigned>(0b00000, n64::instruction::REGISTER_IMMEDIATE)},
            {"asgnh", std::tuple<std::uint8_t, unsigned>(0b00001, n64::instruction::REGISTER_IMMEDIATE)},
            {"asgnl", std::tuple<std::uint8_t, unsigned>(0b00010, n64::instruction::REGISTER_IMMEDIATE)}
    };

    // registers
    register_map_t register_map={
            {"r0", n64::reg::id::R0},
#define RS_REGISTER(z, n, d) {"rs" #n, n64::reg::id::RS[n]},
            BOOST_PP_REPEAT(32, RS_REGISTER, 0)
#undef RS_REGISTER
#define RT_REGISTER(z, n, d) {"rt" #n, n64::reg::id::RT[n]},
            BOOST_PP_REPEAT(32, RT_REGISTER, 0)
#undef RT_REGISTER
            {"ip", n64::reg::id::IP},
            {"flags", n64::reg::id::FLAGS},
            {"sp", n64::reg::id::SP},
            {"bp", n64::reg::id::BP}
    };

    n64::save_binary(output, assemble(system_info_t(instruction_map, register_map), input));

    return EXIT_SUCCESS;
}
