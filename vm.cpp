
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
VirtualMachine::VirtualMachine(std::vector<std::uint8_t> static_datas, bool enable_debug)
    : debug_enabled(enable_debug) {
    Address head_addr = 0;
    for (const auto &byte : static_datas) {
        this->static_datas[head_addr] = byte;
        head_addr++;
    }
    this->registers.fill(0);
}
VirtualMachine::VirtualMachine(std::unordered_map<Address, std::uint8_t, Address::Hash> static_datas, bool enable_debug)
    : static_datas(static_datas), debug_enabled(enable_debug) {
    this->registers.fill(0);
}
void VirtualMachine::install_callbacks(CallBacks callbacks) { this->callbacks = callbacks; }
using namespace magic_enum::ostream_operators;  // enumをこの名前空間内でストリームに流すため
void VirtualMachine::exec(std::vector<OneStep> steps, Address entry_point) {
    program_counter = entry_point;
    while (program_counter < steps.size()) {
        const auto &step = steps.at(static_cast<size_t>(program_counter));
        auto instruction = (step & 0b1111'1111'0000'0000) >> 8;
        int operand[] = {static_cast<int>((step & 0b1111'0000) >> 4), static_cast<int>((step & 0b1111))};
        std::optional<Address> next_program_counter = std::nullopt;
        BOOST_LOG_TRIVIAL(debug) << stringify_vm_state();
        std::stringstream debug_msg;
        debug_msg << "@" << static_cast<int>(program_counter) << " opecode"
                  << std::to_string(static_cast<int>(instruction)) << "("
                  << magic_enum::enum_cast<Instruction>(static_cast<int>(instruction)).value() << ") " << operand[0]
                  << "," << operand[1];
        BOOST_LOG_TRIVIAL(debug) << debug_msg.str();
        this->callbacks.on_loop_head(this->program_counter, step);
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
                    next_program_counter = static_cast<size_t>(this->registers[operand[1]]);
                }
                break;
            case static_cast<uint8_t>(Instruction::IFP):
                if (this->registers[operand[0]] > 0) {
                    next_program_counter = static_cast<size_t>(this->registers[operand[1]]);
                }
                break;
            case static_cast<uint8_t>(Instruction::IFN):
                if (this->registers[operand[0]] < 0) {
                    next_program_counter = static_cast<size_t>(this->registers[operand[1]]);
                }
                break;
            case static_cast<uint8_t>(Instruction::GOTO):
                next_program_counter = this->registers[operand[0]];
                break;
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
                    break;  //プログラムカウンタは普通に進む
                } else {
                    this->call_stack.push(program_counter);
                    next_program_counter = this->registers[operand[0]];
                }
                break;
            case static_cast<uint8_t>(Instruction::RET):
                next_program_counter = this->call_stack.top().value() + 1;  //呼出の次の命令から再開
                this->call_stack.pop();
                break;
            default: {
                std::stringstream err_msg;
                err_msg << "@" << static_cast<int>(program_counter) << " can't understand opecode"
                        << std::to_string(static_cast<int>(instruction)) << "("
                        << magic_enum::enum_name(
                               magic_enum::enum_cast<Instruction>(static_cast<int>(instruction)).value())
                        << ")";
                throw std::runtime_error(err_msg.str());
            }
        }
        if (next_program_counter.has_value()) {  //変更された
            this->program_counter = next_program_counter.value();
        } else {  //変更されなかった
            this->program_counter++;
        }
        this->callbacks.on_program_counter_updated(this->program_counter);
    }
}
std::optional<Register> VirtualMachine::invoke_builtin(Address func_addr) {
    TRIVIAL_LOG_WITH_FUNCNAME(debug) << "called" << func_addr.value();
    // std::vector<std::variant<Register::ValueType, Address::ValueType, size_t>> args;
    std::vector<std::variant<size_t>> args;  // currently, all types above is the same
    std::optional<Register> result = std::nullopt;
    switch (func_addr.value()) {
        case static_cast<uint8_t>(BuiltinFuncs::READ):
            args.push_back(pop_as<Register::ValueType>(this->calculation_stack));
            args.push_back(pop_as<Address::ValueType>(this->calculation_stack));
            args.push_back(pop_as<size_t>(this->calculation_stack));
            result = this->read(std::get<Register::ValueType>(args[0]), std::get<Address::ValueType>(args[1]),
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
        default: {
            std::stringstream err_msg;
            err_msg << "can't invoke built in function" << func_addr.value();
            throw std::runtime_error(err_msg.str());
        }
    }
    return result;
}
Register VirtualMachine::read(Register fd, Address buf, std::size_t len) {
    TRIVIAL_LOG_WITH_FUNCNAME(debug) << "built-in read invoked";
    Register result;
    // TODO: fd0とそれ以外の処理が、cinかfstream*かしか違わず、ほぼ同じコード。きれいにまとめられるはず
    if (fd.value() == 0) {
        for (size_t i = 0; i < len; i++) {
            auto new_char = std::cin.get();
            access_memory(Address(buf + i)) = new_char;
            result = i + 1;  // NOTE: iは序数なので0始まり、結果は基数なので1始まり
            if (new_char == '\n') {
                break;  // NOTE: resultは、途中でここに入ったらそのときの値のままで、最後まで行ったらちゃんとlen+1になる
            }
        }
    } else if (fd.value() < 3) {
        throw std::runtime_error("error: neither stdout nor stderr is readable");
    } else {
        for (size_t i = 0; i < len; i++) {
            auto new_char = this->fd_to_file.at(fd)->get();
            access_memory(Address(buf + i)) = new_char;
            result = i + 1;  // NOTE: iは序数なので0始まり、結果は基数なので1始まり
            if (new_char == '\n') {
                break;
            }
        }
    }
    return result;
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
        result << std::dec << "r" << i << ":" << std::hex << static_cast<Register::ValueType>(register_) << " ";
        i++;
    }
    result << std::endl;
    return result.str();
}

// REVIEW: 読み込んでいるだけなので、マルチスレッドはないままで大丈夫？
Address VirtualMachine::get_program_counter() { return this->program_counter; }

// TODO: マルチスレッドに対応した実装
/*
void VirtualMachine::set_program_counter(Address new_value) {
}
*/
}  // namespace nyulan
