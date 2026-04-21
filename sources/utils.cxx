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
