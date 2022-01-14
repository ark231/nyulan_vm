
#include "vm.hpp"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <cmath>
#include <functional>
#include <magic_enum.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
namespace nyulan {
namespace {
Register treat_as_double(Register reg0, Register reg1, std::function<double(double, double)> operation) {
    double conved_reg0 = *reinterpret_cast<double *>(&reg0);
    double conved_reg1 = *reinterpret_cast<double *>(&reg1);
    double ope_result = operation(conved_reg0, conved_reg1);
    return *reinterpret_cast<Register *>(&ope_result);
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
    : static_datas(static_datas) {}
void VirtualMachine::exec(std::vector<OneStep> steps) {
    Address program_counter = 0;
    while (program_counter < steps.size()) {
        const auto &step = steps[static_cast<size_t>(program_counter)];
        auto instruction = (step & 0b1111'1111'0000'0000) >> 8;
        int operand[] = {static_cast<int>((step & 0b1111'0000) >> 4), static_cast<int>((step & 0b1111))};
        BOOST_LOG_TRIVIAL(debug) << stringify_vm_state();
        switch (static_cast<int>(instruction)) {
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

            case static_cast<uint8_t>(Instruction::PUSHR):
                this->calculation_stack.push(static_cast<std::uint8_t>(this->registers[operand[0]] & 0b1111'1111));
                break;

            case static_cast<uint8_t>(Instruction::PUSHR16):
                this->calculation_stack.push(static_cast<std::uint8_t>(this->registers[operand[0]] & 0b1111'1111));
                break;

            case static_cast<uint8_t>(Instruction::PUSHL):
                this->calculation_stack.push(static_cast<std::uint8_t>(step & 0b1111'1111));
                break;
            case static_cast<uint8_t>(Instruction::POP):
                this->registers[operand[0]] ^= (this->registers[operand[0]] & 0b1111'1111);
                this->registers[operand[0]] |= this->calculation_stack.top();
                break;

            case static_cast<uint8_t>(Instruction::STORE):
                this->memory[this->registers[operand[0]]] =
                    static_cast<uint8_t>(this->registers[operand[1]] & 0b1111'1111);
                break;

            case static_cast<uint8_t>(Instruction::LOAD):
                this->registers[operand[0]] &= this->memory[this->registers[operand[1]]];
                break;

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
                err_msg << "can't understand opecode" << std::to_string(static_cast<int>(instruction)) << "("
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
    switch (static_cast<Address::ValueType>(func_addr)) {}
    return std::nullopt;
}

std::string VirtualMachine::stringify_vm_state() {
    std::stringstream result;
    for (size_t i = 0; const auto &register_ : registers) {
        result << "r" << i << ":" << static_cast<Register::ValueType>(register_) << " ";
        i++;
    }
    result << std::endl;
    return result.str();
}
}  // namespace nyulan
