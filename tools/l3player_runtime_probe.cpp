#include "legends3ds/pbd_model.h"
#include "legends3ds/player_composition.h"
#include "legends3ds/pbd_animation.h"
#include "legends3ds/player_pose.h"
#include "legends3ds/visual_bridge.h"
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

static std::vector<uint8_t> read_file(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open " + p.string());
    f.seekg(0, std::ios::end);
    const auto n = f.tellg();
    if (n < 0) throw std::runtime_error("cannot size " + p.string());
    f.seekg(0, std::ios::beg);
    std::vector<uint8_t> b(static_cast<size_t>(n));
    if (!b.empty()) f.read(reinterpret_cast<char*>(b.data()), static_cast<std::streamsize>(b.size()));
    if (!f && !b.empty()) throw std::runtime_error("cannot read " + p.string());
    return b;
}

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            std::cerr << "usage: l3player_runtime_probe <tutorial-pack-root>\n";
            return 2;
        }
        std::filesystem::path root = argv[1];
        if (std::filesystem::exists(root / "legends3ds" / "tutorial")) root /= "legends3ds/tutorial";
        const auto player = root / "player";
        const std::array<const char*, l3::kPlayerComponentSlots> names{{
            "body.l3model", "head.l3model", "right.l3model", "left.l3model", "feet.l3model"
        }};
        std::array<l3::PbdModel, l3::kPlayerComponentSlots> models{};
        for (size_t i = 0; i < names.size(); ++i) models[i] = l3::load_l3model(read_file(player / names[i]), true);
        const auto composition = l3::load_l3player_composition(read_file(player / "player.l3pc"), true);
        const auto animation = l3::load_l3animation(read_file(player / "main.l3anim"), true);
        const auto skeleton = l3::load_l3skeleton(read_file(player / "main.l3skel"), true);

        l3::PlayerVisualStats vs{};
        const auto local = l3::build_player_visuals(models, composition, &vs);
        const l3::PbdAnimationAction* action = nullptr;
        for (const auto& a : animation.actions) {
            if (a.m_is_frame_table && !a.angle_frames.empty()) { action = &a; break; }
        }
        if (!action) throw std::runtime_error("main animation contains no runtime frame-table action");
        const auto pose_bank = l3::decode_pbd_main_pose_bank(action->angle_frames.front());
        const auto matrices = l3::build_player_pose_matrices(skeleton, pose_bank);
        const auto posed = l3::pose_player_visuals(local, matrices);

        float minx=std::numeric_limits<float>::max(), miny=minx, minz=minx;
        float maxx=std::numeric_limits<float>::lowest(), maxy=maxx, maxz=maxx;
        uint32_t refs=0;
        for (const auto& p : posed) {
            for (uint8_t i=0;i<p.vertex_count;++i) {
                const auto& v=p.position[i];
                minx=std::min(minx,v.x); miny=std::min(miny,v.y); minz=std::min(minz,v.z);
                maxx=std::max(maxx,v.x); maxy=std::max(maxy,v.y); maxz=std::max(maxz,v.z); ++refs;
            }
        }
        std::cout << "L3 player runtime probe: PASS\n"
                  << "  components: 5\n"
                  << "  attachments: " << composition.attachments.size() << "\n"
                  << "  visuals: " << local.size() << " (tri " << vs.triangles << ", quad " << vs.quads << ")\n"
                  << "  animation slots: " << animation.action_slots << "\n"
                  << "  selected action: " << action->action_index << " frames " << action->angle_frames.size() << "\n"
                  << "  posed vertex refs: " << refs << "\n"
                  << "  first-pose bounds: [" << minx << ',' << miny << ',' << minz << "]..["
                  << maxx << ',' << maxy << ',' << maxz << "]\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "L3 player runtime probe: FAIL: " << e.what() << "\n";
        return 1;
    }
}
