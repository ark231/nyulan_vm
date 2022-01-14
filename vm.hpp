#ifndef NYULAN_VM
#define NYULAN_VM
#include <array>
#include <fstream>
#include <map>
#include <optional>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

#include "nyulan.hpp"
namespace nyulan {
class VirtualMachine {
   public:
    VirtualMachine(std::vector<std::uint8_t>);
    VirtualMachine(std::unordered_map<Address, std::uint8_t, Address::Hash>);
    void exec(std::vector<OneStep>);

   private:
    std::array<Register, 16> registers;
    std::stack<std::uint8_t> calculation_stack;
    std::stack<Address> call_stack;
    std::map<Register, std::fstream, Register::Hash> fd_to_file;
    std::unordered_map<Address, std::uint8_t, Address::Hash> memory;
    std::unordered_map<Address, std::uint8_t, Address::Hash> static_datas;

    std::optional<Register> invoke_builtin(Address);
    // builtins
    void read(Register fd, Address buf, std::size_t len);
    void write(Register fd, Address buf, std::size_t len);
    Register open(Address fname);
    void close(Register fd);

    // for debugging
    std::string stringify_vm_state();
};
}  // namespace nyulan
#endif
