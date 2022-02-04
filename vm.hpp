#ifndef NYULAN_VM
#define NYULAN_VM
#include <array>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

#include "nyulan.hpp"
namespace nyulan {
template <class R, class... Args>
R NOP(Args...) {
    return R();  // TODO:デフォルトコンストラクタがない型をどうにかする?
}
template <class... Args>
void NOP(Args...) {}

struct CallBacks {
    std::function<void(Address)> on_program_counter_updated = NOP<void, Address>;
    std::function<void(Address, OneStep)> on_loop_head = NOP<void, Address, OneStep>;
};
class VirtualMachine {
   public:
    VirtualMachine(std::vector<std::uint8_t>, bool enable_debug = false);
    VirtualMachine(std::unordered_map<Address, std::uint8_t, Address::Hash>, bool enable_debug = false);
    void install_callbacks(CallBacks callbacks);
    void exec(std::vector<OneStep>, Address entry_point = 0);

    // for debugging
    std::string stringify_vm_state();
    Address get_program_counter();
    void set_program_counter(Address new_value);

   private:
    std::array<Register, 16> registers;
    std::stack<std::uint8_t> calculation_stack;
    std::stack<Address> call_stack;
    std::map<Register, std::unique_ptr<std::fstream>> fd_to_file;
    std::unordered_map<Address, std::uint8_t, Address::Hash> memory;
    std::unordered_map<Address, std::uint8_t, Address::Hash> static_datas;
    bool debug_enabled;
    Address program_counter;
    CallBacks callbacks = CallBacks();

    std::optional<Register> invoke_builtin(Address);
    // builtins
    Register read(Register fd, Address buf, std::size_t len);
    void write(Register fd, Address buf, std::size_t len);
    Register open(Address fname);
    void close(Register fd);
    void exit(Register exit_code);

    // wrapper
    std::uint8_t& access_memory(Address addr);
};
}  // namespace nyulan
#endif
