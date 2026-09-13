#include <melee_host/dol.h>

#include <array>
#include <cstdint>
#include <fstream>

namespace {

/* The header lists seven text and eleven data sections: file offsets, then
 * load addresses, then sizes, each a big-endian table. */
constexpr std::size_t kHeaderSize = 0x100;
constexpr std::size_t kSectionCount = 18;
constexpr std::size_t kAddressTable = 0x48;
constexpr std::size_t kSizeTable = 0x90;

std::uint32_t read_be32(const unsigned char* bytes)
{
    return (static_cast<std::uint32_t>(bytes[0]) << 24U) |
           (static_cast<std::uint32_t>(bytes[1]) << 16U) |
           (static_cast<std::uint32_t>(bytes[2]) << 8U) |
           static_cast<std::uint32_t>(bytes[3]);
}

} // namespace

extern "C" MeleeHostStatus melee_host_dol_read(const char* dol_path,
                                               mh_u32 address, void* output,
                                               size_t length)
{
    if (dol_path == nullptr || (output == nullptr && length != 0)) {
        return MELEE_HOST_INVALID_ARGUMENT;
    }
    std::ifstream stream(dol_path, std::ios::binary);
    std::array<unsigned char, kHeaderSize> header{};
    if (!stream ||
        !stream.read(reinterpret_cast<char*>(header.data()),
                     static_cast<std::streamsize>(header.size())))
    {
        return MELEE_HOST_IO_ERROR;
    }

    for (std::size_t section = 0; section < kSectionCount; ++section) {
        const std::uint32_t file_offset = read_be32(header.data() + section * 4);
        const std::uint32_t start =
            read_be32(header.data() + kAddressTable + section * 4);
        const std::uint32_t size =
            read_be32(header.data() + kSizeTable + section * 4);
        if (size == 0 || address < start || address - start >= size) {
            continue;
        }
        const std::uint32_t within = address - start;
        if (length > size - within) {
            /* The range starts in this section and runs past its end. */
            return MELEE_HOST_INVALID_ARGUMENT;
        }
        stream.seekg(static_cast<std::streamoff>(
            static_cast<std::uint64_t>(file_offset) + within));
        if (!stream ||
            !stream.read(static_cast<char*>(output),
                         static_cast<std::streamsize>(length)))
        {
            return MELEE_HOST_IO_ERROR;
        }
        return MELEE_HOST_OK;
    }
    return MELEE_HOST_INVALID_ARGUMENT;
}
