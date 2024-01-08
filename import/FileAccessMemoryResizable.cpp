#include "FileAccessMemoryResizable.h"

Ref<FileAccess> FileAccessMemoryResizable::create() {
	return memnew(FileAccessMemoryResizable);
}

Error FileAccessMemoryResizable::open_resizable(uint64_t p_len) {
    resizableData.resize (p_len);
    return open_custom (&resizableData[0], p_len);
}

void FileAccessMemoryResizable::store_8(uint8_t p_byte) {
	if (get_error() == ERR_FILE_EOF) {
        resize (resizableData.size() * 2);
    }
    FileAccessMemory::store_8 (p_byte);
}

void FileAccessMemoryResizable::store_buffer(const uint8_t *p_src, uint64_t p_length) {
    if (p_length + get_position() > get_length()) {
        resize (p_length + get_position());
    }

    FileAccessMemory::store_buffer (p_src, p_length);
}

void FileAccessMemoryResizable::resize(uint64_t p_len) {
    const size_t old_pos = get_position();
    open_resizable (p_len);
    seek (old_pos);
}