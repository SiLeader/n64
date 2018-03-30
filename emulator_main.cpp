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
#include <sstream>
#include <vector>
#include <unistd.h>
#include <iomanip>

#include "instruction.hpp"
#include "cmdline.hpp"

namespace {
    /**
     * flags register
     */
    class flags {
    private:
        std::uint64_t _value;

    private:
        /**
         * set bit
         * @param bit bit index
         * @param w 1 (true) or 0 (false)
         */
        void _set(int bit, bool w)noexcept {
            if(w) {
                _value |= 1ULL<<bit;
            }else{
                _value &= ~(1ULL<<bit);
            }
        }
        /**
         * get bit
         * @param bit bit index
         * @return 1 (true) or 0 (false)
         */
        bool _get(int bit)const noexcept {
            return (_value & (1ULL<<bit)) != 0;
        }

    public:
        flags() : _value(0) {}

        /**
         * set equal flag
         * @param w 0 or 1
         */
        void equal(bool w)noexcept {
            _set(0, w);
        }
        /**
         * get equal flag
         * @return 0 or 1
         */
        bool equal()const noexcept {
            return _get(0);
        }

        /**
         * set above flag
         * @param w 0 or 1
         */
        void above(bool w)noexcept {
            _set(1, w);
        }
        /**
         * get above flag
         * @return 0 or 1
         */
        bool above()const noexcept {
            return _get(1);
        }

        /**
         * set halt flag
         * @param w 0 or 1
         */
        void halt(bool w)noexcept {
            _set(2, w);
        }
        /**
         * get halt flag
         * @return 0 or 1
         */
        bool halt()const noexcept {
            return _get(2);
        }

        /**
         * dump flags register data
         * @return dumped string
         */
        std::string dump() {
            std::string s;
            if(equal()) {
                s+="e";
            }else{
                s+="-";
            }
            if(above()) {
                s+="a";
            }else{
                s+="-";
            }
            if(halt()) {
                s+="h";
            }else{
                s+="-";
            }
            return s;
        }
    };

    /**
     * stack emulator
     */
    class stack {
    private:
        std::vector<std::uint8_t> _stack;

    private:
        /**
         * push backend
         * @param data bytes array
         * @param len data length(len bytes)
         */
        void _push_impl(const std::uint8_t *data, std::size_t len) {
            _stack.insert(std::end(_stack), data, data+len);
        }
        /**
         * pop backend
         * @param buf buffer
         * @param len data length(len bytes)
         */
        void _pop_impl(std::uint8_t *buf, std::size_t len) {
            for(std::size_t i=0; i<len; ++i) {
                buf[len-i-1]=_stack.back();
                _stack.pop_back();
            }
        }
    public:
        /**
         * push as 64bit
         * @param _64 64bit data
         */
        void push(std::uint64_t _64) {
            _push_impl(reinterpret_cast<std::uint8_t*>(&_64), 8);
        }
        /**
         * push as 32bit
         * @param _32 32bit data
         */
        void push(std::uint32_t _32) {
            _push_impl(reinterpret_cast<std::uint8_t*>(&_32), 4);
        }
        /**
         * push as 16bit
         * @param _16 16bit data
         */
        void push(std::uint16_t _16) {
            _push_impl(reinterpret_cast<std::uint8_t*>(&_16), 2);
        }
        /**
         * push as 8bit
         * @param _8 8bit data
         */
        void push(std::uint8_t _8) {
            _push_impl(&_8, 1);
        }

    public:
        /**
         * pop as 64bit
         * @param _64 64bit buffer
         */
        void pop(std::uint64_t& _64) {
            _pop_impl(reinterpret_cast<std::uint8_t*>(&_64), 8);
        }
        /**
         * pop as 32bit
         * @param _32 32bit buffer
         */
        void pop(std::uint32_t& _32) {
            _pop_impl(reinterpret_cast<std::uint8_t*>(&_32), 4);
        }
        /**
         * pop as 16bit
         * @param _16 16bit buffer
         */
        void pop(std::uint16_t& _16) {
            _pop_impl(reinterpret_cast<std::uint8_t*>(&_16), 2);
        }
        /**
         * pop as 8bit
         * @param _8 8bit buffer
         */
        void pop(std::uint8_t&_8) {
            _pop_impl(&_8, 1);
        }
    };

    /**
     * cpu emulator
     * remove: default constructor, copy & move constructors and copy & move assign operators.
     */
    class cpu {
    public:
        static constexpr unsigned IP=n64::reg::id::IP, FLAGS=n64::reg::id::FLAGS,
                RS_FIRST=n64::reg::id::RS[0], RS_LAST=n64::reg::id::RS[31],
                RT_FIRST=n64::reg::id::RT[0], RT_LAST=n64::reg::id::RT[31];
        static constexpr int UD=0;
    private:
        std::uint64_t _registers[128];
        flags _flags;

        std::vector<n64::instruction::instruction> _instructions;

        stack _stack;
    public:
        cpu()=delete;
        explicit cpu(std::vector<n64::instruction::instruction> i) : _registers(), _flags(), _instructions(std::move(i)) {}
        cpu(const cpu&)=delete;
        cpu(cpu&&)=delete;

        cpu& operator=(const cpu&)=delete;
        cpu& operator=(cpu&&)=delete;

    public:
        /**
         * check halted
         * @return is halted
         */
        bool halted()const noexcept {
            return _flags.halt();
        }

        /**
         * dump cpu info
         * @return current cpu info
         */
        std::string dump() {
            std::stringstream ss;
            ss
                    <<"======== ======== ======== dump ======== ======== ========\n"<<std::hex
                    <<" IP   = 0x"<<std::setw(16)<<std::setfill('0')<<_registers[IP]<<std::setw(0)<<"  FLAGS = "<<_flags.dump()<<std::endl;
            for(std::size_t i=RS_FIRST; i<=RS_LAST; ++i) {
                ss<<" RS"<<std::setw(2)<<std::setfill(' ')<<std::dec<<(i-RS_FIRST)<<" = 0x"<<std::setw(16)<<std::setfill('0')<<std::hex<<_registers[i];
                ++i;
                ss<<"  RS"<<std::setw(2)<<std::setfill(' ')<<std::dec<<(i-RS_FIRST)<<" = 0x"<<std::setw(16)<<std::setfill('0')<<std::hex<<_registers[i]<<"\n";
            }
            for(std::size_t i=RT_FIRST; i<=RT_LAST; ++i) {
                ss<<" RT"<<std::setw(2)<<std::setfill(' ')<<std::dec<<(i-RT_FIRST)<<" = 0x"<<std::setw(16)<<std::setfill('0')<<std::hex<<_registers[i];
                ++i;
                ss<<"  RT"<<std::setw(2)<<std::setfill(' ')<<std::dec<<(i-RT_FIRST)<<" = 0x"<<std::setw(16)<<std::setfill('0')<<std::hex<<_registers[i]<<"\n";
            }
            ss<<"======== ======== ======== ---- ======== ======== ========"<<std::endl;
            return ss.str();
        }

        /**
         * raise cpu exception
         * @param type exception type
         */
        void raise_exception(int type) {
            static const std::string EXCEPT[]={
                    "undefined instruction"
            };
            std::cerr<<"Exception raised: "<<EXCEPT[type]<<std::endl;
            std::cerr<<dump()<<std::endl;
            _flags.halt(false);
        }

        /**
         * cpu has next instruction
         * @return has next instruction
         */
        bool has_next() {
            return _instructions.size()>_registers[IP];
        }

        /**
         * run next instruction
         */
        void next() {
            if(_flags.halt())return;

            auto ins=_instructions[_registers[IP]++];

            switch(ins.instruction.type) {
                case n64::instruction::THREE_ADDRESS:{
                    std::uint64_t& dest=_registers[ins.ta.destination];
                    std::uint64_t& src1=_registers[ins.ta.source1];
                    std::uint64_t& src2=_registers[ins.ta.source2];

                    switch(ins.instruction.instruction) {
                        case 0b00000: // add
                            dest=src1+src2;
                            break;
                        case 0b00001: // sub
                            dest=src1-src2;
                            break;
                        case 0b00010: // mul
                            dest=src1*src2;
                            break;
                        case 0b00011: // div
                            dest=src1/src2;
                            break;
                        case 0b00100: // shr
                            dest=src1 >> src2;
                            break;
                        case 0b00101: // shl
                            dest=src1 << src1;
                            break;
                        case 0b00110: // and
                            dest=src1 & src2;
                            break;
                        case 0b00111: // or
                            dest=src1 | src2;
                            break;
                        case 0b01000: // xor
                            dest=src1 ^ src2;
                            break;
                        default:
                            raise_exception(UD);
                            return;
                    }
                }
                    break;
                case n64::instruction::BINOMIAL:{
                    std::uint64_t& op1=_registers[ins.b.operand1];
                    std::uint64_t& op2=_registers[ins.b.operand2];

                    switch(ins.instruction.instruction) {
                        case 0b00000: // not
                            op1= ~op2;
                            break;
                        case 0b00001: // xchg
                            if(ins.b.operand1!=ins.b.operand2)std::swap(op1, op2);
                            break;
                        case 0b00010: // cmp
                            _flags.equal(op1==op2);
                            _flags.equal(op1 > op2);
                            break;
                        default:
                            raise_exception(UD);
                            return;
                    }
                }
                    break;
                case n64::instruction::UNARY:{
                    std::uint64_t data=0;
                    switch(ins.u.type) {
                        case 0b00: // register
                            data=_registers[ins.u.reg.operand];
                            break;
                        case 0b01: // pointer
                            // memory not implemented
                            break;
                        case 0b11: // immediate
                            data=ins.u.imm.immediate;
                            break;
                    }

                    bool assign=false, jump=false;

                    switch(ins.instruction.instruction) {
                        case 0b00000: // inc
                            ++data;
                            assign=true;
                            break;
                        case 0b00001: // dec
                            --data;
                            assign=true;
                            break;
                        case 0b01011: // push
                            _stack.push(data);
                            break;
                        case 0b01100: // pop
                            _stack.pop(data);
                            assign=true;
                            break;

                        case 0b00010: // call
                            _stack.push(_registers[IP]);
                        case 0b00011: // jmp
                        case 0b00100: // jr
                            jump=true;
                            break;
                        case 0b00101: // je
                            jump=_flags.equal();
                            break;
                        case 0b00110: // jne
                            jump=!_flags.equal();
                            break;
                        case 0b00111: // ja
                            jump=_flags.above();
                            break;
                        case 0b01000: // jae
                            jump=_flags.equal() || _flags.above();
                            break;
                        case 0b01001: // jb
                            jump=!_flags.equal() && !_flags.above();
                            break;
                        case 0b01010: // jbe
                            jump=!_flags.above();
                            break;
                        default:
                            raise_exception(UD);
                            return;
                    }

                    if(assign) {
                        switch(ins.u.type) {
                            case 0b00: // register
                                _registers[ins.u.reg.operand]=data;
                                break;
                            case 0b01: // pointer
                                // memory not implemented
                                break;
                        }
                    }
                    if(jump) {
                        _registers[IP]=data;
                    }
                }
                    break;
                case n64::instruction::NO_OPERAND:
                    switch(ins.instruction.instruction) {
                        case 0b00000: // hlt
                            _flags.halt(true);
                            break;
                        case 0b00001: // ret
                            _stack.pop(_registers[IP]);
                            break;
                        default:
                            raise_exception(UD);
                            return;
                    }
                    break;
                case n64::instruction::REGISTER_IMMEDIATE:{
                    std::uint64_t& reg=_registers[ins.ri.reg];
                    std::uint64_t imm=ins.ri.immediate;

                    switch(ins.instruction.instruction) {
                        case 0: // asgn
                            break;
                        case 0b00001: // asgnh
                            imm &= 0xffffffff;
                            imm <<= 32;
                            imm |= (reg & 0xffffffff);
                            break;
                        case 0b00010: // asgnl
                            imm &= 0xffffffff;
                            imm |= (reg & (0xffffffffULL<<32));
                            break;
                        default:
                            raise_exception(UD);
                            return;
                    }
                    reg=imm;
                }
                    break;
            }
        }
    };
} /* anonymous */

int main(int argc, char **argv) {
    std::cout<<"N64 CPU Emulator"<<std::endl;

    cmdline::parser parser;

    parser.parse_check(argc, argv);
    auto input=parser.rest()[0];

    auto instructions=n64::load_binary(input);

    cpu c(instructions);
    int i=0;
    while(c.has_next() && !c.halted()) {
        c.next();
        std::cout<<(i++)<<" "<<c.dump()<<std::endl;
    }

    return EXIT_SUCCESS;
}
