#pragma once
#include <string_view>

namespace l3 {
enum class TestBootPolicy { Invalid, Manual, SelectedRenderer };
// Optional development harness only. Absence/invalid input keeps the manual
// launch menu, including its independent safe0143 selection.
inline TestBootPolicy parse_test_boot_policy(std::string_view text) noexcept {
    if(!text.empty()&&text.back()=='\n')text.remove_suffix(1);
    if(text=="AUTOSTART_SELECTED=1")return TestBootPolicy::SelectedRenderer;
    if(text=="AUTOSTART_SELECTED=0")return TestBootPolicy::Manual;
    return TestBootPolicy::Invalid;
}
}
