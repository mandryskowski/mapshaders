#include "ParserOutputFile.h"
#include <godot_cpp/classes/file_access.hpp>
#include "../Parser.h"
#include <algorithm>
using namespace godot;

StreamPeerBuffer* ParserOutputFile::get_parser( const Vector2i &tile, unsigned int slot, unsigned int parser ) {
    if (slot >= slot_offsets.size()) {
        ERR_PRINT("Invalid slot " + String::num_int64(slot) + " requested.");
        return nullptr;
    }
    if (parser > get_parser_count(slot)) {
        ERR_PRINT("Invalid parser " + String::num_int64(parser) + " requested.");
        return nullptr;
    }
    Array fas;
    if (!tile_bytes.has(tile)) {
        for (int j = 0; j < parser_count; j++) {
            fas.append(memnew(StreamPeerBuffer));
        }
        tile_bytes[tile] = fas;
    } else {
        fas = tile_bytes[tile];
    }
    return Object::cast_to<StreamPeerBuffer>(fas[slot_offsets[slot] + parser]);
}

void ParserOutputFile::save()
{
    Ref<FileAccess> out = FileAccess::open(filename.replace(".osm", ".sgdmap"), FileAccess::WRITE);
    PackedInt64Array tile_offs, tile_lens;

    // Tile space rect
    Vector2i min_tile(std::numeric_limits<int>::max(), std::numeric_limits<int>::max()), max_tile(std::numeric_limits<int>::min(), std::numeric_limits<int>::min());

    for (int i = 0; i < tile_bytes.size(); i++) {
        if (static_cast<Vector2i>(tile_bytes.keys()[i]).x == std::numeric_limits<int>::min() || static_cast<Vector2i>(tile_bytes.keys()[i]).y == std::numeric_limits<int>::min() || static_cast<Vector2i>(tile_bytes.keys()[i]).x == 0 || static_cast<Vector2i>(tile_bytes.keys()[i]).y == 0)
            continue;

        Array fas = tile_bytes[tile_bytes.keys()[i]];
        bool empty = true;
        for (int j = 0; j < fas.size(); j++) {
            if (static_cast<Ref<StreamPeerBuffer>>(fas[j])->get_position() != 0) {
                empty = false;
                break;
            }
        }

        if (empty)
            continue;

            
        WARN_PRINT(String::num_int64(static_cast<Vector2i>(tile_bytes.keys()[i]).x) + " " + String::num_int64(static_cast<Vector2i>(tile_bytes.keys()[i]).y));
        min_tile = Vector2i(std::min(min_tile.x, static_cast<Vector2i>(tile_bytes.keys()[i]).x), std::min(min_tile.y, static_cast<Vector2i>(tile_bytes.keys()[i]).y));
        max_tile = Vector2i(std::max(max_tile.x, static_cast<Vector2i>(tile_bytes.keys()[i]).x), std::max(max_tile.y, static_cast<Vector2i>(tile_bytes.keys()[i]).y));
    }
    WARN_PRINT("Tile space rect: " + String::num_int64(min_tile.x) + " " + String::num_int64(min_tile.y) + " " + String::num_int64(max_tile.x) + " " + String::num_int64(max_tile.y));

    Vector2i tile_space_size = max_tile - min_tile + Vector2i(1, 1);
    if (tile_space_size.x > 500 || tile_space_size.y > 500) {
        WARN_PRINT("Tile space bounds too large: " + String::num_int64(tile_space_size.x) + " " + String::num_int64(tile_space_size.y));
        return;
    }
    // We will store tile offsets at the beginning
    
    tile_offs.resize (tile_space_size.x * tile_space_size.y);
    tile_lens.resize (tile_space_size.x * tile_space_size.y);
    out->store_var (tile_offs);
    out->store_var (tile_lens);

    for (int y = 0; y < tile_space_size.y; y++) {
        for (int x = 0; x < tile_space_size.x; x++) {
            const Vector2i tile = min_tile + Vector2i(x, y);
            const int i = y * tile_space_size.x + x;
            if (!tile_bytes.has(tile))
                continue;

            const_cast<int64_t&>(tile_offs[i]) = out->get_position();
            auto tile_fas = static_cast<Array>(tile_bytes[tile]);

            for (int j = 0; j < tile_fas.size(); j++) {
                Ref<StreamPeerBuffer> fa = static_cast<Ref<StreamPeerBuffer>>(tile_fas[j]);

                size_t len = fa->get_position();
                fa->seek(0);
                PackedByteArray arr = fa->get_data_array();

                if (arr.size() != len)
                    WARN_PRINT("Bug in OSMParser::import: buffer length mismatch.");

                // Empty flag
                out->store_8 (len == 0 ? 1 : 0);
                // Contents
                out->store_buffer (arr);
            }
            const_cast<int64_t&>(tile_lens[i]) = out->get_position() - tile_offs[i];
        }
    }
    
    // Overwrite tile offsets at the beginning
    out->seek(0);
    out->store_var (tile_offs);
    out->store_var (tile_lens);
}

Ref<FileAccess> ParserOutputFile::load_tile_fa( unsigned int index ) {
    Ref<FileAccess> fa = FileAccess::open(filename, FileAccess::READ);
    PackedInt64Array tile_offs, tile_lens;

    tile_offs = fa->get_var();
    tile_lens = fa->get_var();

    if (tile_offs.size() != tile_lens.size())
        WARN_PRINT ("Tile offsets count different from tile lengths count. File might be corrupted.");

    if (tile_lens[index] == 0) {
        fa->close();
        return nullptr;
    }

    fa->seek (tile_offs[index]);

    return fa;
}

unsigned int ParserOutputFile::load_tile_count() const {
    Ref<FileAccess> fa = FileAccess::open(filename, FileAccess::READ);
    PackedInt64Array tile_offs = fa->get_var();
    fa->close();
    return tile_offs.size();
}