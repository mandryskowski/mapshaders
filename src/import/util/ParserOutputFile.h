#ifndef MAPSHADERS_UTIL_PARSER_OUTPUT_FILE_H
#define MAPSHADERS_UTIL_PARSER_OUTPUT_FILE_H
#include <vector>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/classes/stream_peer_buffer.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/file_access.hpp>

class ParserOutputFile : public godot::RefCounted {
    GDCLASS(ParserOutputFile, godot::RefCounted);
public:
    ParserOutputFile() : parser_count(0), filename("") {} // Required by godot; do not use
    /**
     * @brief Construct a new ParserOutputFile object
     * @param filename The filename to save to
     * @param parser_counts The number of parsers per slot
     */
    ParserOutputFile(const godot::String& filename, const std::vector<unsigned int>& parsers_in_slot) : filename(filename), parser_count(0) {
        // Calculate offsets (prefix sum)
        for (int i = 0; i < parsers_in_slot.size(); i++) {
            const_cast<std::vector<unsigned int>&>(slot_offsets).push_back(parser_count);
            const_cast<unsigned int&>(parser_count) += parsers_in_slot[i];
        }
    }
    godot::StreamPeerBuffer* get_parser(const godot::Vector2i& tile, unsigned int slot, unsigned int parser);
    void save();
    unsigned int get_parser_count(unsigned int slot) const {
        if (slot == slot_offsets.size() - 1) {
            return parser_count - slot_offsets[slot];
        }

        return slot_offsets[slot + 1] - slot_offsets[slot];
    }

    /**
     * @brief Get tile file access. It is your responsibility to close the file access.
     * @param index The tile index
     * @return The file access which you must close after use
     */
    godot::Ref<godot::FileAccess> load_tile_fa(unsigned int index);
    unsigned int load_tile_count() const;

protected:
    static void _bind_methods() {}

private:
    const std::vector<unsigned int> slot_offsets;
    const unsigned int parser_count;
    const godot::String filename;
    godot::Dictionary tile_bytes; // Vector2i -> Array of Ref<StreamPeerBuffer>
};

class ParserOutputFileHandle : public godot::RefCounted {
    GDCLASS(ParserOutputFileHandle, godot::RefCounted);
public:
    ParserOutputFileHandle() {} // Required by godot; do not use
    ParserOutputFileHandle(godot::Ref<ParserOutputFile> file, unsigned int slot) : file(file), slot(slot) {}
    godot::StreamPeerBuffer* get_parser(const godot::Vector2i& tile, unsigned int parser) {
        return file->get_parser(tile, slot, parser);
    }
    unsigned int get_parser_count() const {
        return file->get_parser_count(slot);
    }

protected:
    static void _bind_methods() {}
private:
    godot::Ref<ParserOutputFile> file;
    unsigned int slot;
};

#endif // MAPSHADERS_UTIL_PARSER_OUTPUT_FILE_H