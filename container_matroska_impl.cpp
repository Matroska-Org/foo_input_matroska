#include "container_matroska_impl.h"

/**
 * container_matroska
 */

void container_matroska_impl::open(const char * p_path, bool p_info_only, foobar2000_io::abort_callback &p_abort) {
    cleanup();
    if (!is_our_path(p_path)) {
        throw exception_io_unsupported_format();
    }
    m_path = p_path;
    m_abort = &p_abort;
    service_ptr_t<file> file_ptr;
    try {
        filesystem::g_open_read(file_ptr, m_path, *m_abort);
        matroska_parser_ptr parser = matroska_parser_ptr(new MatroskaAudioParser(file_ptr, *m_abort));
        parser->Parse(p_info_only);
        for (t_size i = 0; i != parser->GetAttachmentList().get_count(); ++i) {
            MatroskaAttachment & item = parser->GetAttachmentList().get_item(i);
            matroska::attachment attachment(this, *m_abort, item.FileName.GetUTF8().c_str(), item.MimeType.c_str(), item.Description.GetUTF8().c_str(),
                static_cast<t_size>(item.SourceDataLength), static_cast<t_sfilesize>(item.SourceStartPos));
            m_attachment_list.add_item(attachment);
        }
    } catch (...) {
        throw exception_io_unsupported_format();
    }
}

void container_matroska_impl::open_file(service_ptr_t<file> & p_out, const filesystem::t_open_mode p_mode) const {
    filesystem::g_open(p_out, m_path, p_mode, *m_abort);
}

void container_matroska_impl::get_display_path(pfc::string_base & p_out) const {
    p_out = m_path;
}

const matroska::attachment_list * container_matroska_impl::get_attachment_list() const {
    return reinterpret_cast<const matroska::attachment_list *>(&m_attachment_list);
}
