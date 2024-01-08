#ifndef FILE_ACCESS_MEMORY_RESIZABLE_H
#define FILE_ACCESS_MEMORY_RESIZABLE_H
#include "core/io/file_access_memory.h"

class FileAccessMemoryResizable : public FileAccessMemory
{
    static Ref<FileAccess> create();
public:
    virtual Error open_resizable(uint64_t p_len); ///< open a file
    virtual void store_8(uint8_t p_byte) override; ///< store a byte
    virtual void store_buffer(const uint8_t *p_src, uint64_t p_length) override;
private:
    void resize(uint64_t p_len);
    PackedByteArray resizableData;
};

#endif // FILE_ACCESS_MEMORY_RESIZABLE_H