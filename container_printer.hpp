#ifndef NYULAN_CONTAINER_PRINTER
#define NYULAN_CONTAINER_PRINTER

#include <concepts>
#include <ostream>
#include <stack>
namespace nyulan {
namespace container_ostream {
template <typename T>
concept is_implemented_in_ostream = requires(std::ostream stream, T& value) {
    stream.operator<<(value);
};
template <typename T>
concept is_implemented_somewhere = requires(std::ostream stream, T& value) {
    operator<<(stream, value);  //まだこのヘッダで定義されるのは(定義||宣言)されてない
};
template <typename T>
concept is_already_implemented = is_implemented_in_ostream<T> || is_implemented_somewhere<T>;

template <typename T>
concept Ranged = requires(T& value) {
    begin(value);
    end(value);
};
template <typename T>
concept StackLike = requires(T& value) {
    value.top();
    value.pop();
};
template <typename T>
concept has_value_type = requires {
    typename T::value_type;
};
template <typename T>
concept has_key_map_type = requires {
    typename T::key_type;
    typename T::mapped_type;
};
template <typename T>
concept supportedVectorLike = Ranged<T>&& has_value_type<T> && !is_already_implemented<T>;

template <typename T>
concept supportedMapLike = Ranged<T>&& has_value_type<T>&& has_key_map_type<T> && !is_already_implemented<T>;

template <supportedVectorLike Container>
std::ostream& operator<<(std::ostream& stream, const Container& data) {
    stream << "[";
    for (const auto& datum : data) {
        stream << datum << ",";  // TODO: trailing commaどうにかする
    }
    stream << "]";
    return stream;
}
template <supportedMapLike Container>
std::ostream& operator<<(std::ostream& stream, const Container& data) {
    stream << "{";
    for (const auto& datum : data) {
        stream << datum.first << " : " << datum.second << "," << std::endl;
    }
    stream << "}";
    return stream;
}
template <StackLike Container>
std::ostream& operator<<(std::ostream& stream, Container data) {
    for (size_t i = 0; i < data.size(); i++) {
        stream << data.top() << std::endl;
        data.pop();
    }
    return stream;
}
}  // namespace container_ostream
}  // namespace nyulan
#endif
