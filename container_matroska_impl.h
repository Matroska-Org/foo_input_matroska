#ifndef _CONTAINER_MATROSKA_IMPL_H_
#define _CONTAINER_MATROSKA_IMPL_H_

#include "matroska_parser.h"
#include "container_matroska.h"

typedef boost::shared_ptr<MatroskaAudioParser> matroska_parser_ptr;

class container_matroska_impl : public container_matroska
{
private:
    typedef pfc::list_t<matroska::attachment> attachment_list_impl;

    abort_callback * m_abort;
    pfc::string8 m_path;
    attachment_list_impl m_attachment_list;

    void cleanup() {
        m_path.reset();
        m_attachment_list.remove_all();
    };

protected:
    container_matroska_impl() {
    };
    ~container_matroska_impl() {
        cleanup();
    };

public:
    virtual void open(const char * p_path, bool p_info_only, abort_callback & p_abort);
    virtual void open_file(service_ptr_t<file> & p_out, const filesystem::t_open_mode p_mode = filesystem::open_mode_read) const;
    virtual void get_display_path(pfc::string_base & p_out) const;
    virtual bool is_our_path(const char * p_path) const {
        if (stricmp_utf8(pfc::string_extension(p_path), "mka") != 0 && stricmp_utf8(pfc::string_extension(p_path), "mkv") != 0) {
            return false;
        }
        return true;
    }
    virtual const matroska::attachment_list * get_attachment_list() const;
};

static container_matroska_factory_t<container_matroska_impl> g_container_matroska_impl_factory;

#endif // _CONTAINER_MATROSKA_IMPL_H_