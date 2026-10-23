// A target leased by a Buster shot (flags_04 lease bit) must re-register with
// the 0x800BE478 owner table: 0x800341BC then writes the target point into the
// leasing owner. Without the table the registration is refused.
#include "legends3ds/child_runtime.h"
#include <cassert>
#include <iostream>

int main(){
    using namespace l3;
    ChildRuntime rt;
    const uint32_t shot=0x8009cf88u;   // any 4-aligned owner identity
    assert(rt.bind_proxy_owner(0,shot,SourceHitProxyOwnerPoint{{1,2}}));
    assert(rt.bind_proxy_owner(3,shot,SourceHitProxyOwnerPoint{{1,2}}));   // alias of the same owner
    SourceHitProxyTarget target;target.source_address=0x80122620u;target.flags_04=0x80000000u|0x10000000u;
    SourceHitProxyRegistration r;r.pointed_words={{0x00100020u,0x00000030u}};r.a1=0x50;r.a3=0x8000;
    r.stack10=0xE0;r.stack18=0x04000000u;r.stack1c=0x00800000u;r.target=&target;r.mask_80098798=0xffffff00u|0x90000000u;
    // Without owners: refused.
    assert(source_hit_proxy_register_800341bc(rt.hit_proxy_bank(),r).status==SourceHitProxyStatus::NeedsOwnerPoint);
    // With the runtime's table: registered, and the owner point now holds the target point.
    const auto owners=rt.proxy_owner_pointers();
    assert(owners[0]&&owners[3]==owners[0]&&!owners[1]);
    r.owners_800be478=owners;
    assert(source_hit_proxy_register_800341bc(rt.hit_proxy_bank(),r).status==SourceHitProxyStatus::SourceBackedPartial);
    rt.publish_proxy_owner_points();
    assert(rt.proxy_owner_point(0)->raw_bc_words[0]==0x00100020u&&rt.proxy_owner_point(3)->raw_bc_words[1]==0x00000030u);
    std::cout<<"PASS: leased target re-registers with the owner table; aliases synced\n";
}
