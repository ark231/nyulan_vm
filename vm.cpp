
#include "vm.hpp"

#include <algorithm>
#include <bitset>
#include <cassert>
#include <cmath>
#include <functional>
#include <ios>
#include <iostream>
#include <magic_enum.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "logging.hpp"
namespace nyulan {
namespace {
Register treat_as_double(Register reg0, Register reg1, std::function<double(double, double)> operation) {
    double conved_reg0 = *reinterpret_cast<double *>(&reg0);
    double conved_reg1 = *reinterpret_cast<double *>(&reg1);
    double ope_result = operation(conved_reg0, conved_reg1);
    return *reinterpret_cast<Register *>(&ope_result);
}
template <typename T>
T pop_as(std::stack<std::uint8_t> &stack) {
    TRIVIAL_LOG_WITH_FUNCNAME(debug) << "pop " << typeid(T).name() << " of size " << sizeof(T) << " from the "
                                     << stack.size() << " depth of stack";
    if (stack.size() < sizeof(T)) {
        std::stringstream err_msg;
        err_msg << "error: stack size of " << stack.size() << " is shorter than the required size of " << sizeof(T);
        throw std::runtime_error(err_msg.str());
    }
    std::stringstream debug_msg;
    T value = 0;
    for (size_t i = 0; i < sizeof(T); i++) {
        debug_msg << std::hex << static_cast<std::uint16_t>(stack.top()) << " ";
        value <<= 8;  //最初の一回が無効化される
        value |= (stack.top());
        stack.pop();
    }
    TRIVIAL_LOG_WITH_FUNCNAME(debug) << debug_msg.str();
    TRIVIAL_LOG_WITH_FUNCNAME(debug) << value;
    return value;
}
}  // namespace
VirtualMachine::VirtualMachine(std::vector<std::uint8_t> static_datas) {
    Address head_addr = 0;
    for (const auto &byte : static_datas) {
        this->static_datas[head_addr] = byte;
        head_addr++;
    }
    this->registers.fill(0);
}
VirtualMachine::VirtualMachine(std::unordered_map<Address, std::uint8_t, Address::Hash> static_datas)
    : static_datas(static_datas) {
    this->registers.fill(0);
}
void VirtualMachine::exec(std::vector<OneStep> steps) {
    Address program_counter = 0;
    while (program_counter < steps.size()) {
        const auto &step = steps[static_cast<size_t>(program_counter)];
        auto instruction = (step & 0b1111'1111'0000'0000) >> 8;
        int operand[] = {static_cast<int>((step & 0b1111'0000) >> 4), static_cast<int>((step & 0b1111))};
        BOOST_LOG_TRIVIAL(debug) << stringify_vm_state();
        std::stringstream debug_msg;
        debug_msg << "@" << static_cast<int>(program_counter) << " opecode"
                  << std::to_string(static_cast<int>(instruction)) << "("
                  << magic_enum::enum_name(magic_enum::enum_cast<Instruction>(static_cast<int>(instruction)).value())
                  << ") " << operand[0] << "," << operand[1];
        BOOST_LOG_TRIVIAL(debug) << debug_msg.str();
        switch (instruction.value()) {
            case static_cast<uint8_t>(Instruction::MOV):
                this->registers[operand[0]] = this->registers[operand[1]];
                break;

            case static_cast<uint8_t>(Instruction::AND):
                this->registers[operand[0]] &= this->registers[operand[1]];
                break;

            case static_cast<uint8_t>(Instruction::OR):
                this->registers[operand[0]] |= this->registers[operand[1]];
                break;

            case static_cast<uint8_t>(Instruction::XOR):
                this->registers[operand[0]] ^= this->registers[operand[1]];
                break;

            case static_cast<uint8_t>(Instruction::NOT):
                this->registers[operand[0]] = ~this->registers[operand[0]];
                break;

            case static_cast<uint8_t>(Instruction::ADD):
                this->registers[operand[0]] += this->registers[operand[1]];
                break;

            case static_cast<uint8_t>(Instruction::SUB):
                this->registers[operand[0]] -= this->registers[operand[1]];
                break;

            case static_cast<uint8_t>(Instruction::MUL):
                this->registers[operand[0]] *= this->registers[operand[1]];
                break;

            case static_cast<uint8_t>(Instruction::DIV):
                this->registers[operand[0]] /= this->registers[operand[1]];
                break;

            case static_cast<uint8_t>(Instruction::MOD):
                this->registers[operand[0]] %= this->registers[operand[1]];
                break;

            case static_cast<uint8_t>(Instruction::DADD):
                this->registers[operand[0]] = treat_as_double(this->registers[operand[0]], this->registers[operand[1]],
                                                              [](double a, double b) { return a + b; });
                break;

            case static_cast<uint8_t>(Instruction::DSUB):
                this->registers[operand[0]] = treat_as_double(this->registers[operand[0]], this->registers[operand[1]],
                                                              [](double a, double b) { return a - b; });
                break;

            case static_cast<uint8_t>(Instruction::DMUL):
                this->registers[operand[0]] = treat_as_double(this->registers[operand[0]], this->registers[operand[1]],
                                                              [](double a, double b) { return a * b; });
                break;

            case static_cast<uint8_t>(Instruction::DDIV):
                this->registers[operand[0]] = treat_as_double(this->registers[operand[0]], this->registers[operand[1]],
                                                              [](double a, double b) { return a / b; });
                break;

            case static_cast<uint8_t>(Instruction::DMOD):
                this->registers[operand[0]] = treat_as_double(this->registers[operand[0]], this->registers[operand[1]],
                                                              [](double a, double b) { return std::fmod(a, b); });
                break;

            case static_cast<uint8_t>(Instruction::PUSHR8):
                this->calculation_stack.push(static_cast<std::uint8_t>(this->registers[operand[0]] & 0b1111'1111));
                break;

            case static_cast<uint8_t>(Instruction::PUSHR16):
                for (size_t i = 0; i < sizeof(uint16_t); i++) {
                    this->calculation_stack.push(
                        static_cast<std::uint8_t>((this->registers[operand[0]] >> (8 * i)) & (0b1111'1111)));
                }
                break;

            case static_cast<uint8_t>(Instruction::PUSHR32):
                for (size_t i = 0; i < sizeof(uint32_t); i++) {
                    this->calculation_stack.push(
                        static_cast<std::uint8_t>((this->registers[operand[0]] >> (8 * i)) & (0b1111'1111)));
                }
                break;

            case static_cast<uint8_t>(Instruction::PUSHR64):
                for (size_t i = 0; i < sizeof(uint64_t); i++) {
                    this->calculation_stack.push(
                        static_cast<std::uint8_t>((this->registers[operand[0]] >> (8 * i)) & (0b1111'1111)));
                }
                break;

            case static_cast<uint8_t>(Instruction::PUSHL):
                this->calculation_stack.push(static_cast<std::uint8_t>(step & 0b1111'1111));
                break;
            case static_cast<uint8_t>(Instruction::POP8):
                this->registers[operand[0]] ^= (this->registers[operand[0]] & 0b1111'1111);
                this->registers[operand[0]] |= this->calculation_stack.top();
                break;
            case static_cast<uint8_t>(Instruction::POP16): {
                this->registers[operand[0]] ^= (this->registers[operand[0]] & (~static_cast<std::uint16_t>(0)));
                uint16_t value = 0;
                for (size_t i = 0; i < sizeof(uint16_t); i++) {
                    value <<= 8;
                    value |= this->calculation_stack.top();
                    this->calculation_stack.pop();
                }
                this->registers[operand[0]] |= value;
                break;
            }
            case static_cast<uint8_t>(Instruction::POP32): {
                this->registers[operand[0]] ^= (this->registers[operand[0]] & (~static_cast<std::uint32_t>(0)));
                uint32_t value = 0;
                for (size_t i = 0; i < sizeof(uint32_t); i++) {
                    value <<= 8;
                    value |= this->calculation_stack.top();
                    this->calculation_stack.pop();
                }
                this->registers[operand[0]] |= value;
                break;
            }
            case static_cast<uint8_t>(Instruction::POP64): {
                this->registers[operand[0]] ^= (this->registers[operand[0]] & (~static_cast<std::uint64_t>(0)));
                uint64_t value = 0;
                for (size_t i = 0; i < sizeof(uint64_t); i++) {
                    value <<= 8;
                    value |= this->calculation_stack.top();
                    this->calculation_stack.pop();
                }
                this->registers[operand[0]] |= value;
                break;
            }

            case static_cast<uint8_t>(Instruction::STORE8):
                access_memory(this->registers[operand[0]]) =
                    static_cast<uint8_t>(this->registers[operand[1]] & 0b1111'1111);
                break;
            case static_cast<uint8_t>(Instruction::STORE16):
                for (size_t i = 0; i < sizeof(uint16_t); i++) {
                    access_memory(this->registers[operand[0]]) =
                        static_cast<uint8_t>((this->registers[operand[1]] >> (8 * i)) & 0b1111'1111);
                }
                break;
            case static_cast<uint8_t>(Instruction::STORE32):
                for (size_t i = 0; i < sizeof(uint32_t); i++) {
                    access_memory(this->registers[operand[0]]) =
                        static_cast<uint8_t>((this->registers[operand[1]] >> (8 * i)) & 0b1111'1111);
                }
                break;
            case static_cast<uint8_t>(Instruction::STORE64):
                for (size_t i = 0; i < sizeof(uint64_t); i++) {
                    access_memory(this->registers[operand[0]]) =
                        static_cast<uint8_t>((this->registers[operand[1]] >> (8 * i)) & 0b1111'1111);
                }
                break;

            case static_cast<uint8_t>(Instruction::LOAD8):
                this->registers[operand[0]] ^= (this->registers[operand[0]] & 0b1111'1111);
                this->registers[operand[0]] |= access_memory(this->registers[operand[1]]);
                break;
            case static_cast<uint8_t>(Instruction::LOAD16): {
                this->registers[operand[0]] ^= (this->registers[operand[0]] & (~static_cast<std::uint16_t>(0)));
                uint16_t value = 0;
                for (size_t i = 0; i < sizeof(uint16_t); i++) {
                    value |= access_memory(this->registers[operand[1] + i]) << (8 * i);
                }
                this->registers[operand[0]] |= value;
                break;
            }
            case static_cast<uint8_t>(Instruction::LOAD32): {
                this->registers[operand[0]] ^= (this->registers[operand[0]] & (~static_cast<std::uint32_t>(0)));
                uint32_t value = 0;
                for (size_t i = 0; i < sizeof(uint32_t); i++) {
                    value |= access_memory(this->registers[operand[1] + i]) << (8 * i);
                }
                this->registers[operand[0]] |= value;
                break;
            }
            case static_cast<uint8_t>(Instruction::LOAD64): {
                this->registers[operand[0]] ^= (this->registers[operand[0]] & (~static_cast<std::uint64_t>(0)));
                uint64_t value = 0;
                for (size_t i = 0; i < sizeof(uint64_t); i++) {
                    TRIVIAL_LOG_WITH_FUNCNAME(debug) << "access @" << (this->registers[operand[1]] + i).value();
                    value |= access_memory(this->registers[operand[1]] + i) << (8 * i);
                }
                this->registers[operand[0]] |= value;
                break;
            }

            case static_cast<uint8_t>(Instruction::LSHIFT):
                this->registers[operand[0]] <<= this->registers[operand[1]];
                break;
            case static_cast<uint8_t>(Instruction::RSHIFT):
                this->registers[operand[0]] >>= this->registers[operand[1]];
                break;
            case static_cast<uint8_t>(Instruction::IFZ):
                if (this->registers[operand[0]] == 0) {
                    program_counter = static_cast<size_t>(this->registers[operand[1]]);
                }
                continue;  //プログラムカウンタが上書きされたので、インクリメントは飛ばさなければならない
            case static_cast<uint8_t>(Instruction::IFP):
                if (this->registers[operand[0]] > 0) {
                    program_counter = static_cast<size_t>(this->registers[operand[1]]);
                }
                continue;  //プログラムカウンタが上書きされたので、インクリメントは飛ばさなければならない
            case static_cast<uint8_t>(Instruction::IFN):
                if (this->registers[operand[0]] < 0) {
                    program_counter = static_cast<size_t>(this->registers[operand[1]]);
                }
                continue;  //プログラムカウンタが上書きされたので、インクリメントは飛ばさなければならない
            case static_cast<uint8_t>(Instruction::GOTO):
                program_counter = this->registers[operand[0]];
                continue;  //プログラムカウンタが上書きされたので、インクリメントは飛ばさなければならない
            case static_cast<uint8_t>(Instruction::CALL):
                if (registers[operand[0]] & ((Register)0b1 << 63)) {
                    // built_in
                    auto result =
                        this->invoke_builtin(registers[operand[0]] & ~((Register)0b1 << 63));  //最上位ビットのみを抽出
                    if (result) {
                        for (size_t i = 0; i < sizeof(Register::ValueType); i++) {
                            std::uint8_t byte =
                                static_cast<uint8_t>((result.value() & (0b1111'1111 << (i * 8))) >> (i * 8));
                            calculation_stack.push(byte);
                        }
                    }
                    break;
                } else {
                    this->call_stack.push(program_counter);
                    program_counter = this->registers[operand[0]];
                    continue;  //プログラムカウンタが上書きされたので、インクリメントは飛ばさなければならない
                }
            case static_cast<uint8_t>(Instruction::RET):
                program_counter = this->call_stack.top();
                this->call_stack.pop();
                continue;  //プログラムカウンタが上書きされたので、インクリメントは飛ばさなければならない
            default: {
                std::stringstream err_msg;
                err_msg << "@" << static_cast<int>(program_counter) << " can't understand opecode"
                        << std::to_string(static_cast<int>(instruction)) << "("
                        << magic_enum::enum_name(
                               magic_enum::enum_cast<Instruction>(static_cast<int>(instruction)).value())
                        << ")";
                throw std::runtime_error(err_msg.str());
            } break;
        }
        program_counter++;
    }
}
std::optional<Register> VirtualMachine::invoke_builtin(Address func_addr) {
    TRIVIAL_LOG_WITH_FUNCNAME(debug) << "called" << func_addr.value();
    // std::vector<std::variant<Register::ValueType, Address::ValueType, size_t>> args;
    std::vector<std::variant<size_t>> args;  // currently, all types above is the same
    switch (func_addr.value()) {
        case static_cast<uint8_t>(BuiltinFuncs::READ):
            args.push_back(pop_as<Register::ValueType>(this->calculation_stack));
            args.push_back(pop_as<Address::ValueType>(this->calculation_stack));
            args.push_back(pop_as<size_t>(this->calculation_stack));
            this->read(std::get<Register::ValueType>(args[0]), std::get<Address::ValueType>(args[1]),
                       std::get<size_t>(args[2]));
            break;
        case static_cast<uint8_t>(BuiltinFuncs::WRITE):
            args.push_back(pop_as<Register::ValueType>(this->calculation_stack));
            args.push_back(pop_as<Address::ValueType>(this->calculation_stack));
            args.push_back(pop_as<size_t>(this->calculation_stack));
            this->write(std::get<Register::ValueType>(args[0]), std::get<Address::ValueType>(args[1]),
                        std::get<size_t>(args[2]));
            break;
        case static_cast<uint8_t>(BuiltinFuncs::EXIT):
            args.push_back(pop_as<Register::ValueType>(this->calculation_stack));
            this->exit(std::get<Register::ValueType>(args[0]));
            break;
    }
    return std::nullopt;
}
void VirtualMachine::read(Register fd, Address buf, std::size_t len) {
    TRIVIAL_LOG_WITH_FUNCNAME(debug) << "built-in read invoked";
    if (fd.value() == 0) {
        for (size_t i = 0; i < len; i++) {
            access_memory(Address(buf + i)) = std::cin.get();
        }
    } else if (fd.value() < 3) {
        throw std::runtime_error("error: neither stdout nor stderr is readable");
    } else {
        for (size_t i = 0; i < len; i++) {
            access_memory(Address(buf + i)) = this->fd_to_file.at(fd)->get();
        }
    }
}
void VirtualMachine::write(Register fd, Address buf, std::size_t len) {
    TRIVIAL_LOG_WITH_FUNCNAME(debug) << "built-in write invoked";
    TRIVIAL_LOG_WITH_FUNCNAME(debug) << "fd:" << fd.value() << "buf:" << buf.value() << "len:" << len;
    TRIVIAL_LOG_WITH_FUNCNAME(debug) << "write into " << fd.value();
    switch (fd.value()) {
        case 0:
            throw std::runtime_error("error: stdin is not readable");
            break;
        case 1:
            for (size_t i = 0; i < len; i++) {
                std::cout.put(access_memory(Address(buf + i)));
            }
            break;
        case 2:
            for (size_t i = 0; i < len; i++) {
                std::cerr.put(access_memory(Address(buf + i)));
            }
            break;
        default:
            for (size_t i = 0; i < len; i++) {
                this->fd_to_file.at(fd)->put(access_memory(Address(buf + i)));
            }
            break;
    }
}
Register VirtualMachine::open(Address fname) {}
void VirtualMachine::close(Register fd) {}
void VirtualMachine::exit(Register exit_code) { ::exit(exit_code.value()); }

std::uint8_t &VirtualMachine::access_memory(Address addr) {
    std::stringstream debug_msg;
    debug_msg << std::bitset<8 * sizeof(addr.value())>(addr.value());
    TRIVIAL_LOG_WITH_FUNCNAME(debug) << debug_msg.str();
    TRIVIAL_LOG_WITH_FUNCNAME(debug) << (addr.value() & (UINT64_C(1) << 63));
    TRIVIAL_LOG_WITH_FUNCNAME(debug) << (addr.value() & ~(UINT64_C(1) << 63));
    try {
        if ((addr.value() & (UINT64_C(1) << 63)) == 0) {  //最上位ビットが立ってない
            return this->memory.at(addr);
        } else {
            return this->static_datas.at((addr.value() & ~(UINT64_C(1) << 63)));
        }
    } catch (std::out_of_range) {
        std::string type = ((addr.value() & (UINT64_C(1) << 63)) == 0) ? "dynamic" : "static";
        throw std::runtime_error("invalid memory access to " + type +
                                 " address:" + std::to_string(addr.value() & ~(UINT64_C(1) << 63)));
    }
}

std::string VirtualMachine::stringify_vm_state() {
    std::stringstream result;
    for (size_t i = 0; const auto &register_ : registers) {
        result << "r" << i << ":" << std::hex << static_cast<Register::ValueType>(register_) << " ";
        i++;
    }
    result << std::endl;
    return result.str();
}
}  // namespace nyulan
