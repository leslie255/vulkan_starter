#include "utils.hxx"

#include "vk_fmt.hxx"

void assert_vk_success(VkResult result, std::source_location location) {
    if (result != VK_SUCCESS) {
        eprintln(
            "[ERROR] vulkan call returned non-success result {} at {}:{}:{}",
            fmt_vk(result),
            std::string_view(location.file_name(), strlen(location.file_name())),
            location.line(),
            location.column());
    }
}

std::vector<uint8_t> read_file_to_bytes(std::string_view path) {
    auto file = std::ifstream(std::string(path), std::ios::binary);
    if (!file.is_open()) {
        eprintln("error opening file at {}", path);
        exit(1);
    }
    return std::vector<uint8_t>(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
}
