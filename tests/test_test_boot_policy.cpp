#include "legends3ds/test_boot_policy.h"
#include <cassert>
int main() {
    using namespace l3;
    assert(parse_test_boot_policy("AUTOSTART_SELECTED=1")==TestBootPolicy::SelectedRenderer);
    assert(parse_test_boot_policy("AUTOSTART_SELECTED=1\n")==TestBootPolicy::SelectedRenderer);
    assert(parse_test_boot_policy("AUTOSTART_SELECTED=0\n")==TestBootPolicy::Manual);
    for(auto text:{"", "1", "AUTOSTART_SELECTED=10", "AUTOSTART_SELECTED=1\nJUNK",
                   "AUTOSTART_SELECTED=1\n\n", " AUTOSTART_SELECTED=1"})
        assert(parse_test_boot_policy(text)==TestBootPolicy::Invalid);
    const char binary[]="AUTOSTART_SELECTED=1\0JUNK";
    assert(parse_test_boot_policy(std::string_view(binary,sizeof(binary)-1))==TestBootPolicy::Invalid);
}
