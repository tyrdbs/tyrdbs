#include <common/system_error.h>
#include <storage/disk.h>

#include <mutex>
#include <sys/mount.h>


namespace tyrtech::storage {


void disk::read(uint32_t page, char* buff)
{
    m_file.pread(static_cast<uint64_t>(page) << page_bits, buff, page_size);
}

uint32_t disk::write(uint32_t page, iovec* iovec, uint32_t size)
{
    return m_file.pwritev(static_cast<uint64_t>(page) << page_bits, iovec, size);
}

uint32_t disk::allocate(uint32_t pages)
{
    uint32_t block_page = m_blocks.allocate(pages);

    while (unlikely(block_page == invalid_handle))
    {
        allocate_space();

        block_page = m_blocks.allocate(pages);
    }

    return block_page;
}

void disk::free(uint32_t page, uint32_t pages)
{
    m_blocks.free(page, pages);
}

void disk::remove(const extents_t& extents)
{
    for (auto&& extent : extents)
    {
        uint32_t extent_page = extent >> 32;
        uint32_t extent_pages = extent & 0xffffffffU;

        free(extent_page, extent_pages);
    }
}

uint32_t disk::size() const
{
    return m_blocks.size();
}

uint32_t disk::capacity() const
{
    return m_blocks.capacity();
}

std::string_view disk::path() const
{
    return m_file.path();
}

uint64_t disk::new_cache_id()
{
    return m_next_cache_id++;
}

disk::disk(io::file file, bool preallocate_space)
  : m_file(std::move(file))
  , m_preallocate_space(preallocate_space)
{
    auto stat = m_file.stat();

    if (S_ISBLK(stat.st_mode) == false)
    {
        if ((stat.st_size & page_mask) != 0)
        {
            throw error("{}: invalid data image", m_file.path());
        }

        m_blocks.extend(stat.st_size >> 12);
    }
    else
    {
        m_preallocate_space = false;

        uint64_t size;

        if (ioctl(m_file.fd(), BLKGETSIZE64, &size) == -1)
        {
            throw error("{}: {}", m_file.path(), system_error().message);
        }

        if ((size & page_mask) != 0)
        {
            throw error("{}: invalid data image", m_file.path());
        }

        m_blocks.extend(size >> 12);
    }
}

void disk::allocate_space()
{
    if (unlikely(m_allocation_in_progress == true))
    {
        m_allocation_cond.wait();
    }
    else
    {
        m_allocation_in_progress = true;

        if (m_preallocate_space == true)
        {
            uint64_t offset = static_cast<uint64_t>(m_blocks.capacity()) << page_bits;
            uint64_t size = 32768UL << page_bits;

            m_file.allocate(0, offset, size);
        }

        m_blocks.extend(32768);

        m_allocation_in_progress = false;
        m_allocation_cond.signal_all();
    }
}

}
