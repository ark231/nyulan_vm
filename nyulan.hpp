#ifndef NYULAN_NYULAN
#define NYULAN_NYULAN
#include <cstdint>

#include "utils.hpp"
namespace nyulan {
struct OneStep_ {};
struct Register_ {};
struct Address_ {};
using OneStep = utils::Phantom<OneStep_, std::uint16_t>;
using Register = utils::Phantom<Register_, std::uint64_t>;  // registerはc++の予約語なので、アッパーキャメルで回避
struct Address : public utils::Phantom<Address_, std::uint64_t> {
    using super = utils::Phantom<Address_, std::uint64_t>;
    using super::PhantomBase_;
    Address(Register value) : super::PhantomBase_(static_cast<std::uint64_t>(value)) {}
};
constexpr std::uint64_t CURRENT_INSTRUCTION_SET_VERSION = 1;
enum class Instruction : std::uint8_t {
    NOP = 0,
    MOV,      // dst,src
    AND,      // dst,src
    OR,       // dst,src
    XOR,      // dst,src
    NOT,      // dst
    ADD,      // dst,src
    SUB,      // dst,src
    MUL,      // dst,src
    DIV,      // dst,src
    MOD,      // dst,src
    DADD,     // dst,src ;ADDの小数バージョン レジスタの中身が小数として扱われる
    DSUB,     // dst,src
    DMUL,     // dst,src
    DDIV,     // dst,src
    DMOD,     // dst,src
    PUSHR8,   // src    ;srcレジスタの下位8bitをスタックに積む
    PUSHR16,  // src    ;srcレジスタの下位16bitをスタックに積む
    PUSHR32,  // src
    PUSHR64,  // src    ;1バイトごとに下位----上位--->スタック伸長方向 となるように積む
    PUSHL,    // literal;1バイトスタックにプッシュする
    POP8,     // dst      ;dstレジスタの下位8bitにスタックトップをポップする
    POP16,    // dst
    POP32,    // dst
    POP64,    // dst
    STORE8,  // addr,src ;addrレジスタの中身のさすメモリ上の1バイトににsrcレジスタの中身の下位8bitをストアする
    STORE16,  // addr,src
    STORE32,  // addr,src
    STORE64,  // addr,src
    LOAD8,    // dst,addr
    LOAD16,   // dst,addr
    LOAD32,   // dst,addr
    LOAD64,   // dst,addr
    LSHIFT,   // dst,src ;dstレジスタの中身をsrcレジスタの中身bit左シフトする
    RSHIFT,   // dst,src
    IFZ,      // addr,src
              // ;srcレジスタの中身が0ならaddrレジスタの中身のアドレスにジャンプする
    IFP,      // addr,src ;正
    IFN,      // addr,src ;負
    GOTO,     // addr
    CALL,     // addr ;アドレスの上位1bitが1のものは全て予約されている
    RET,      // コールスタックの最上位にジャンプする
};

enum class BuiltinFuncs : Register::ValueType {  //これの最上位ビットを立てたものが実際にコードで指定される値
    READ = 0,
    WRITE = 1,
    OPEN = 2,
    CLOSE = 3,
    EXIT = 60,
    MALLOC = 512,
    FREE = 513,
};
}  // namespace nyulan
#endif
