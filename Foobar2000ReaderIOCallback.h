/*
 *  Part of the foobar2000 Matroska plugin
 *
 *  Copyright (C) Jory Stone (jcsston at toughguy net) - 2003
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 *  WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
 
#ifndef _FOOBAR2000_IO_CALLBACK_H_
#define _FOOBAR2000_IO_CALLBACK_H_

#include "../SDK/foobar2000.h"
#include "ebml/IOCallback.h"

using namespace LIBEBML_NAMESPACE;

class Foobar2000ReaderIOCallback : public IOCallback {
public:
	Foobar2000ReaderIOCallback(service_ptr_t<file> & source, abort_callback & p_abort)
		: m_abort(p_abort)
	{
		m_Reader = source;
		if (!m_Reader->can_seek()) {
			m_NoSeekReader = source;
		}
	};

	virtual ~Foobar2000ReaderIOCallback() {
		close();
	};

	virtual uint32 read(void*Buffer, size_t Size) {
		return m_Reader->read(Buffer, Size, m_abort);
	};

	virtual void setFilePointer(int64 Offset, seek_mode Mode=seek_beginning) {
		switch (Mode)
		{
			case seek_beginning:
				m_Reader->seek(Offset, m_abort);
				break;
			case seek_current:
				m_Reader->seek_ex(Offset, file::seek_from_current, m_abort);
				break;
			case seek_end:
				m_Reader->seek_ex(Offset, file::seek_from_eof, m_abort);
				break;
			default:
				//throw "Invalid Seek Mode!!!";
				;
		};
	}

	virtual size_t write(const void*Buffer, size_t Size) {
		m_Reader->write(Buffer, Size, m_abort);
		return Size;
	}

	virtual uint64 getFilePointer() {
		return m_Reader->get_position(m_abort);
	};

	virtual void close() {
	}

	bool seekable() {
		if (m_NoSeekReader == NULL)
			return true;
		return false;
	};

	bool truncate()
	{
		m_Reader->set_eof(m_abort);
		return true;
	}

protected:
	service_ptr_t<file> m_Reader;
	service_ptr_t<file> m_NoSeekReader;
	abort_callback & m_abort;
};

#endif // _FOOBAR2000_IO_CALLBACK_H_
