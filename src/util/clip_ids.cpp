#include <mbgl/util/clip_ids.hpp>
#include <mbgl/map/tile.hpp>

#include <mbgl/util/math.hpp>

#include <list>
#include <vector>
#include <bitset>
#include <cassert>
#include <iostream>
#include <algorithm>

namespace mbgl {

uint32_t ceil_log2(uint64_t x) {
    static const uint64_t t[6] = {0xFFFFFFFF00000000, 0x00000000FFFF0000,
                                  0x000000000000FF00, 0x00000000000000F0,
                                  0x000000000000000C, 0x0000000000000002};
    uint32_t y = (((x & (x - 1)) == 0) ? 0 : 1);
    uint32_t j = 32;

    for (int32_t i = 0; i < 6; i++) {
        const uint32_t k = (((x & t[i]) == 0) ? 0 : j);
        y += k;
        x >>= k;
        j >>= 1;
    }

    return y;
}

ClipIDGenerator::Leaf::Leaf(Tile &tile_) : tile(tile_) {}

void ClipIDGenerator::Leaf::add(const Tile::ID &p) {
    if (p.isChildOf(tile.id)) {
        // Ensure that no already present child is a parent of the new p.
        for (const Tile::ID &child : children) {
            if (p.isChildOf(child))
                return;
        }
        children.push_front(p);
    }
}

bool ClipIDGenerator::Leaf::operator==(const Leaf &other) const {
    return tile.id == other.tile.id && children == other.children;
}

bool ClipIDGenerator::reuseExisting(Leaf &leaf) {
    for (const std::vector<Leaf> &pool : pools) {
        auto existing = std::find(pool.begin(), pool.end(), leaf);
        if (existing != pool.end()) {
            leaf.tile.clip = existing->tile.clip;
            return true;
        }
    }
    return false;
}

void ClipIDGenerator::update(std::forward_list<Tile *> tiles) {
    Pool pool;

    tiles.sort([](const Tile *a, const Tile *b) {
        return a->id < b->id;
    });

    const auto end = tiles.end();
    for (auto it = tiles.begin(); it != end; it++) {
        if (!*it) {
            // Handle null pointers.
            continue;
        }

        Tile &tile = **it;
        Leaf clip { tile };

        // Try to add all remaining ids as children. We sorted the tile list
        // by z earlier, so all preceding items cannot be children of the current
        // tile.
        for (auto child_it = std::next(it); child_it != end; child_it++) {
            clip.add((*child_it)->id);
        }
        clip.children.sort();

        // Loop through all existing pools and try to find a matching ClipID.
        if (!reuseExisting(clip)) {
            // We haven't found an existing clip ID
            pool.push_back(std::move(clip));
        }
    }

    if (pool.size()) {
        const uint32_t bit_count = ceil_log2(pool.size() + 1);
        const std::bitset<8> mask = uint64_t(((1 << bit_count) - 1) << bit_offset);

        // We are starting our count with 1 since we need at least 1 bit set to distinguish between
        // areas without any tiles whatsoever and the current area.
        uint8_t count = 1;
        for (Leaf &leaf : pool) {
            leaf.tile.clip.mask = mask;
            leaf.tile.clip.reference = count++ << bit_offset;
        }

        bit_offset += bit_count;
        pools.push_front(std::move(pool));
    }

    if (bit_offset > 8) {
        fprintf(stderr, "stencil mask overflow\n");
    }
}

}
