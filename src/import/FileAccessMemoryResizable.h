#ifndef FILE_ACCESS_MEMORY_RESIZABLE_H
#define FILE_ACCESS_MEMORY_RESIZABLE_H
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/stream_peer_buffer.hpp>

using FileAccessMemoryResizable = godot::StreamPeerBuffer;

/*class FileAccessMemoryResizable : public godot::StreamPeerBuffer
{
    static godot::Ref<FileAccessMemoryResizable> create();
public:
    godot::Error open_resizable(uint64_t p_len); ///< open a file
    void store_8(uint8_t p_byte); ///< store a byte
    void store_buffer(const uint8_t *p_src, uint64_t p_length);
private:
    void resize(uint64_t p_len);
    godot::PackedByteArray resizableData;
};*/

#endif // FILE_ACCESS_MEMORY_RESIZABLE_H