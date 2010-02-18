/*
 *  Part of the foobar2000 Matroska plugin
 *
 *  Copyright (C) Jory Stone (jcsston at toughguy net) - 2003-2004
 *
 *	Based on mp4_parser.cpp from foo_std_input
 *	Copyright (c) 2001-2003, Peter Pawlowski
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

/*!
    \file foo_input_matroska.cpp
		\version $Id$
    \brief A foobar2000 plugin for Matroska
		\author Jory Stone     <jcsston @ toughguy.net>
*/

//Memory Leak Debuging define
#ifdef _DEBUG
//#define _CRTDBG_MAP_ALLOC 
#include <stdlib.h>
#include <crtdbg.h>

// Debug Memory Alloc hook
int YourAllocHook(int allocType, void *userData, size_t size, int blockType, long requestNumber, const unsigned char *filename, int lineNumber)
{
	/*static int64 totalSize;
	if (allocType == 1)
		totalSize += size;
	else if (allocType == 3)
		totalSize -= _msize(userData);
	else if (allocType == 500)
		totalSize = 0;
	else if (allocType == 1000)
		*(int64 *)userData = totalSize;*/
	//I can use this to find exactly where a leak started
	if (size == 136)
	{
		//NOTE(_T("Memory Alloc Hook tripped"));
		return 1;
	}
	return 1;
};
#endif

#include "../SDK/foobar2000.h"

#include <commctrl.h>
#include <Shlwapi.h>
#include <time.h>
#include "matroska_parser.h"
#include "resource.h"
#include "DbgOut.h"

class input_matroska
{
    matroska_parser_ptr m_parser;
	service_ptr_t<packet_decoder> m_decoder;
	t_input_open_reason m_reason;

	int m_TrackNo;
	int m_subsong;
	// Compressed data frame
	MatroskaAudioFrame * m_frame;
	unsigned m_frame_remaining;

	uint64 m_skip_samples;
	uint64 m_skip_frames;
	unsigned m_expected_sample_rate;
	unsigned m_expected_channels;
	unsigned m_expected_bitspersample;
	uint64 m_timescale;
	
	// VBR bitrate display stuff
	double m_vbr_last_duration;
	unsigned m_vbr_update_frames,m_vbr_update_bytes;
	double m_vbr_update_time;
	bool m_vbr_update_enabled;
	unsigned m_vbr_update_interval;
	uint64 m_length,m_position;

	// A temp decoding buffer
	audio_chunk_i m_tempchunk;

public:
	service_ptr_t<file> m_file;
	pfc::array_t<t_uint8> m_buffer;
	
public:
	input_matroska()
	{
		hprintf(L"Matroska: input_matroska()\n");
		//m_parser = NULL;
		m_decoder = NULL;
		m_TrackNo = 0;
		m_skip_samples = 0;
		m_skip_frames = 0;	
		m_timescale = TIMECODE_SCALE;
		m_frame = 0;
		m_frame_remaining = 0;
	
		m_vbr_last_duration = 0;
		m_vbr_update_frames = 0;
		m_vbr_update_bytes = 0;
		m_vbr_update_time = 0;
		m_vbr_update_enabled = true;
		m_vbr_update_interval = 0;
		m_position = 0;
		m_length = 0;
	}

	~input_matroska()
	{
		hprintf(L"Matroska: ~input_matroska()\n");
        hprintf(L"--destruction-- position=%d frame=0x%p\n", m_position, m_frame);
		cleanup();
	}

	void open(service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort)
	{
		// TODO : Use the Cueing Data if we can seek

		/*if (!r->can_seek())
		{
			console::error("Matroska: unseekable streams not supported.");
			return false;
		}*/
		hprintf(L"Matroska: open() p_reason=%d\n", p_reason);

		cleanup();

		m_file = p_filehint;
		input_open_file_helper(m_file, p_path, p_reason, p_abort);
        m_reason = p_reason;

        m_parser = matroska_parser_ptr(new MatroskaAudioParser(m_file, p_abort));
        if (m_parser->Parse(!(m_reason & input_open_decode))) {
		    console::error("Matroska: Invalid Matroska file.");
		    cleanup();
		    throw exception_io_unsupported_format();
	    }

		m_TrackNo = m_parser->GetFirstAudioTrack();
		if (m_TrackNo == -1) {
			console::error("Matroska: no decodable streams found.");
			cleanup();
			throw exception_io_unsupported_format();
		}
	}

	unsigned int get_subsong_count() {
		hprintf(L"Matroska: get_subsong_count() chapters=%d, AudioTrackCount=%d\n", m_parser->GetChapters().size(), m_parser->GetAudioTrackCount());
		if (m_parser->GetChapters().size() > 0) {
			return m_parser->GetChapters().size();
		} else if(m_parser->GetAudioTrackCount() > 0) {
			return m_parser->GetAudioTrackCount();
		}
		return 0;
	}

	t_uint32 get_subsong(unsigned p_index) {
		hprintf(L"Matroska: get_subsong() chapters= %d, p_index=%d\n", m_parser->GetChapters().size(), p_index);
		return p_index;
        /*
		if(m_parser->GetChapters().size() > 0)
		{
			int tr = 0;
			int idx = p_index / m_parser->GetChapters().size();
			m_TrackNo = m_parser->GetAudioTrackIndex(idx);
			m_subsong = p_index % m_parser->GetChapters().size();
			m_parser->SetCurrentTrack(m_TrackNo);
			return p_index;
		} else {
			m_TrackNo = m_parser->GetAudioTrackIndex(p_index);
			m_subsong = 0;
			m_parser->SetCurrentTrack(m_TrackNo);
			return p_index;
		}
        */
	}

	void get_info(t_uint32 p_subsong,file_info & p_info,abort_callback & p_abort) {
		hprintf(L"Matroska: get_info() = %d\n", p_subsong);
		if (m_reason != input_open_decode || m_decoder == NULL) {
			set_current_track(p_subsong);
			initialize_decorder(p_abort);
		}
		p_info.info_set_int("channels", m_expected_channels);
		p_info.info_set_int("samplerate", m_expected_sample_rate);

		m_decoder->get_info(p_info);
		
		m_expected_channels = (unsigned)p_info.info_get_int("channels");
		m_expected_sample_rate = (unsigned)p_info.info_get_int("samplerate");
		m_expected_bitspersample = (unsigned)p_info.info_get_int("bitspersample");
		
		m_decoder->set_stream_property(packet_decoder::property_channels, m_expected_channels, 0, 0);
		m_decoder->set_stream_property(packet_decoder::property_samplerate, m_expected_sample_rate, 0, 0);
		m_decoder->set_stream_property(packet_decoder::property_bitspersample, m_expected_bitspersample, 0, 0);

		m_parser->SetFB2KInfo(p_info, m_subsong);

		hprintf(L"Matroska: m_parser->GetDuration(): %f\n", m_parser->GetDuration());
		p_info.set_length(m_parser->GetDuration());

		if (p_info.info_get_bitrate() == 0) {
			p_info.info_set_bitrate(m_parser->GetAvgBitrate());
		}
	}

	t_filestats get_file_stats(abort_callback & p_abort) {
		hprintf(L"Matroska: get_file_stats()\n");
		return m_file->get_stats(p_abort);
	}

	void decode_initialize(t_uint32 p_subsong, unsigned p_flags, abort_callback & p_abort) {
		hprintf(L"Matroska: decode_initialize() = %d\n", p_subsong);
		set_current_track(p_subsong);
		initialize_decorder(p_abort);
		// The timecode scale in Matroska is in milliseconds, but foobar deals in seconds
		m_timescale = m_parser->GetTimecodeScale() * 1000;
		m_length = duration_to_samples(m_parser->GetDuration());
		m_position = 0;
		m_skip_samples = 0;
		m_skip_frames = 0;
		m_frame_remaining = 0;
		m_frame = 0;
		if (decode_can_seek()) {
			decode_seek(0, p_abort);
		}
	}

	bool decode_run(audio_chunk & p_chunk, abort_callback & p_abort) {
		//hprintf(L"Matroska: decode_run(): m_skip_samples=%d, m_skip_frames=%d, m_frame_remaining=%d, m_frame=%p\n", (int)m_skip_samples, (int)m_skip_frames, m_frame_remaining, m_frame);
		if (m_decoder == NULL)
		{
			hprintf(L"Matroska: attempting to decode without a loaded decoder.\n");
			return false;
		}

		if (m_position >= m_length)
		{
            hprintf(L"Matroska: decode_run() return false: m_position=%d\n", m_position);
			return false;
		}

		bool done = false;

		do
		{
			//unsigned char *buffer = NULL;
			unsigned int buffer_size = 0;
			bool skip_this_frame = false;
/*
			{
				int64 delta = duration_to_samples(m_frame.timecode) - m_position;
				console::info(uStringPrintf("drift: %d",(int)delta));
			}
*/

			if (m_frame_remaining==0 || m_frame==0)
			{
                if (m_frame!=0) {
					delete m_frame;
                    m_frame = NULL;
                }
                hprintf(L"Matroska: decode_run() start ReadSingleFrame()\n");
                try {
    				m_frame = m_parser->ReadSingleFrame();
                } catch (const pfc::exception & e) {
                    hprintf(L"Matroska: ReadSingleFrame(): exception=%s\n", e.what());
                    cleanup();
                    return false;
                }
                if (m_frame==0) {
                    hprintf(L"Matroska: decode_run() return false: m_frame=0\n");
					return false;
                }
				m_frame_remaining = m_frame->dataBuffer.size();
                if (m_frame_remaining == 0) {
                    hprintf(L"Matroska: decode_run() return false: m_frame_remaining=0\n");
                    cleanup();
					return false;
                }
			}

			{
                hprintf(L"Matroska: decode_run() buffer set\n");
				unsigned ptr = m_frame->dataBuffer.size() - (m_frame_remaining--);
				//buffer = (unsigned char*)&m_frame->dataBuffer.at(ptr)[0];
				buffer_size = m_frame->dataBuffer.at(ptr).size();
				m_buffer.set_size(buffer_size);
				m_buffer.set_data_fromptr(&m_frame->dataBuffer.at(ptr)[0], buffer_size);
			}
			
			m_tempchunk.reset();
			try {
                hprintf(L"Matroska: decode_run() start decode()\n");
				m_decoder->decode(m_buffer.get_ptr(), buffer_size, m_tempchunk, p_abort);
                if (m_tempchunk.is_empty() && m_frame->add_id > 0) {
                    m_decoder->decode(&m_frame->additional_data_buffer.at(0), m_frame->additional_data_buffer.size(), m_tempchunk, p_abort);
                }
			} catch (...) {
				MatroskaTrackInfo &currentTrack = m_parser->GetTrack(m_TrackNo);
				console::error(uStringPrintf("Matroska: '%s' decode error.", (const char*)currentTrack.codecID.c_str()));
				//console::error(uStringPrintf("buffer_size = %d, m_frame_remaining = %d", buffer_size, m_frame_remaining));
				//console::error(uStringPrintf("buffer 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X", m_buffer[0], m_buffer[1], m_buffer[2], m_buffer[3], m_buffer[4], m_buffer[5], m_buffer[6], m_buffer[7]));				
				hprintf(L"Matroska: decode_run() return false: decode error\n");
                cleanup();
				return false;
			}
			if (m_tempchunk.is_valid())
			{
				m_vbr_update_frames++;
				m_vbr_update_bytes += buffer_size;
				//m_vbr_update_bytes += m_buffer.get_size();
				m_vbr_update_time += (m_vbr_last_duration = m_tempchunk.get_duration());
			}

			// TODO : maybe we could not decode the skipped frame to get faster seeking ???

			if (m_skip_frames>0)
			{
				skip_this_frame = true;
				m_skip_frames--;
			}


			if (!skip_this_frame)
			{				
				uint64 offset = duration_to_samples(0);//m_frame.timecode);
				uint64 duration = duration_to_samples(m_frame->get_duration());
				//			console::info(uStringPrintf("duration: %u, offset: %u",duration,offset));

				if (m_tempchunk.is_empty())
				{
					if (duration > 0)
					{
						m_tempchunk.set_srate(m_expected_sample_rate);
						m_tempchunk.set_channels(m_expected_channels);
						m_tempchunk.pad_with_silence((t_size)duration);
#ifdef _DEBUG
						console::warning("Matroska: decoder returned empty chunk from a non-empty Matroska frame.");
#endif
					}
				}
				else if (!m_tempchunk.is_valid())
				{
					console::error("Matroska: decoder produced invalid chunk.");
					cleanup();
                    hprintf(L"Matroska: decode_run() return false: invalid chunk\n");
					return false;
				}
				unsigned samplerate,channels,decoded_sample_count;

				samplerate = m_tempchunk.get_srate();
				channels = m_tempchunk.get_channels();
				decoded_sample_count = m_tempchunk.get_sample_count();

				// !!!!!!!!!!!!!!! HACK ALERT !!!!!!!!!!!!!!!!!!!!!!!!
				duration = decoded_sample_count;
				// !!!!!!!!!!!!!!! HACK ALERT !!!!!!!!!!!!!!!!!!!!!!!!

				if (decoded_sample_count < duration)
				{
#ifdef _DEBUG
					console::warning("Matroska: decoded frame smaller than expected.");
#endif
					decoded_sample_count = (unsigned)duration;
					m_tempchunk.pad_with_silence(decoded_sample_count);
				}

				if (duration < offset)
					duration = 0;
				else
					duration -= offset;

				if (m_skip_samples>0)
				{
					uint64 delta = m_skip_samples;
					if (delta > duration) delta = duration;
					offset += delta;
					duration -= delta;
					m_skip_samples -= delta;
				}

				{
					uint64 max = m_length - m_position;
					if (duration>max)
						duration = max;
					m_position += duration;
                    if (m_position==m_length && duration==0) {
                        hprintf(L"Matroska: decode_run() return false: duration=0\n");
						return false;
                    }
				}


				if (duration > 0)
				{
					p_chunk.set_data(m_tempchunk.get_data() + offset * channels,
						(t_size)duration,channels,samplerate);
					//p_chunk.set_data_fixedpoint(m_tempchunk.get_data() + offset * channels, buffer_size, m_expected_sample_rate,m_expected_channels,m_expected_bitspersample,audio_chunk::g_guess_channel_config(m_expected_channels));
					done = true;
				}
			}
		}
		while(!done);

		return done;
	}

	void decode_seek(double p_seconds, abort_callback & p_abort) {
		hprintf(L"Matroska: decode_seek() = %f\n", p_seconds);

		if (m_decoder == NULL)
		{
			console::error("Matroska: attempting to seek while not open.");
			cleanup();
			throw exception_io_object_not_seekable();
		}

		{
			double srate = (double)m_expected_sample_rate;
			p_seconds = floor(p_seconds * srate + 0.5) / srate;
		}

		double max_frame_dependency_time = m_decoder->get_max_frame_dependency_time();
		if (max_frame_dependency_time>p_seconds) max_frame_dependency_time = p_seconds;
		unsigned max_frame_dependency_samples = (unsigned)(m_expected_sample_rate * max_frame_dependency_time + 0.5);

		unsigned frames_to_skip = 0;

		m_position = (uint64)(m_expected_sample_rate * p_seconds + 0.5);

		double time_to_skip = 0;
		if (!m_parser->Seek(p_seconds-max_frame_dependency_time,frames_to_skip,time_to_skip,m_expected_sample_rate)) return;
		
		m_skip_frames = frames_to_skip;

		m_skip_samples = (unsigned)(time_to_skip * m_expected_sample_rate + 0.5) + max_frame_dependency_samples;
		m_frame_remaining = 0;
        m_decoder->reset_after_seek();
		//console::info(uStringPrintf("skip samples: %u",m_skip_samples));
	}

	bool decode_can_seek() {
		hprintf(L"Matroska: decode_can_seek()\n");
		return m_file->can_seek();
	}

	bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta) {
		//hprintf(L"Matroska: decode_get_dynamic_info()\n");
		bool ret = false;
		if (m_vbr_update_enabled)
		{
			if (m_vbr_update_time > 0.5 && m_vbr_update_frames >= m_vbr_update_interval)
			{
				int val = (int) ( ((double)m_vbr_update_bytes * 8.0 / m_vbr_update_time + 500.0) / 1000.0 );
				if (val != p_out.info_get_bitrate_vbr())
				{
					p_timestamp_delta = - (m_vbr_update_time - m_vbr_last_duration);	//relative to last frame beginning;
					p_out.info_set_bitrate_vbr(val);
					ret = true;
				}
				m_vbr_update_frames = 0; m_vbr_update_bytes = 0;
				m_vbr_update_time = 0;
			}
		}
		return ret;
	}

	bool decode_get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta) {
		//hprintf(L"Matroska: decode_get_dynamic_info_track()\n");
		return false;
	}

	void decode_on_idle(abort_callback & p_abort) {
		m_file->on_idle(p_abort);
	}

	void retag_set_info(t_uint32 p_subsong,const file_info & p_info,abort_callback & p_abort) {
		hprintf(L"Matroska: retag_set_info()\n");

		if (m_parser == NULL) throw exception_io_unsupported_format();
					
		int TrackNo = m_parser->GetFirstAudioTrack();
		if (TrackNo != -1) {
			// Set the current track
			m_parser->SetCurrentTrack(m_TrackNo);
			m_parser->SetSubSong(p_subsong);
			m_parser->SetTags(p_info);
		}
	}

	void retag_commit(abort_callback & p_abort) {
		hprintf(L"Matroska: retag_commit()\n");
		m_parser->WriteTags();
	}

	static bool g_is_our_content_type(const char * p_content_type) {
		hprintf(L"Matroska: g_is_our_content_type() p_content_type=%S\n", p_content_type);
		return !stricmp_utf8(p_content_type, "audio/x-matroska") 
			|| !stricmp_utf8(p_content_type, "video/x-matroska")
			// Just to be safe check for ones without x-
			// However, You're supposed to use x-* unless it's offically registered
			|| !stricmp_utf8(p_content_type, "audio/matroska") 
			|| !stricmp_utf8(p_content_type, "video/matroska");
	}

	static bool g_is_our_path(const char * p_path,const char * p_extension) {
		hprintf(L"Matroska: g_is_our_path() p_path=%S p_extension=%S\n", p_path, p_extension);
		if (stricmp_utf8_partial(p_path, "http://", 7) == 0) {
			// HTTP streams and chapters togther are not supported, requires seeking
			return false;
		} else {
			if (stricmp_utf8(p_extension, "MKA") != 0 && stricmp_utf8(p_extension, "MKV") != 0)
				return false;
		}
		return true;
	}

protected:
	void cleanup()
	{
		/*
		if (m_decoder != NULL)
		{
			//m_decoder->service_release();
			m_decoder = NULL;
		}
		*/
		if (m_frame)
		{
			delete m_frame;
			m_frame = NULL;
		}
        /*
		if (m_parser != NULL) {
			delete m_parser;
			m_parser = NULL;
		}
        */
	}
	int64 duration_to_samples(double val)
	{
        return audio_math::time_to_samples(val, m_expected_sample_rate);
	}

	double samples_to_duration(int64 val)
	{
        return audio_math::samples_to_time(val, m_expected_sample_rate);
	}

	void set_current_track(unsigned int p_index) {
		if(m_parser->GetChapters().size() > 0)
		{
			int idx = p_index / m_parser->GetChapters().size();
			m_TrackNo = m_parser->GetAudioTrackIndex(idx);
			m_subsong = p_index % m_parser->GetChapters().size();
		} else {
			m_TrackNo = m_parser->GetAudioTrackIndex(p_index);
			m_subsong = 0;
		}
		m_parser->SetCurrentTrack(m_TrackNo);
		m_parser->SetSubSong(m_subsong);
		hprintf(L"Matroska: set_current_track(): m_TrackNo=%d, m_subsong=%d\n", m_TrackNo, m_subsong);
	}

	void initialize_decorder(abort_callback & p_abort) {
		bool p_decode = false;
		if (m_reason == input_open_decode) {
			p_decode = true;
		}
		hprintf(L"Matroska: initialize_decoder(): m_TrackNo=%d\n", m_TrackNo);
		MatroskaTrackInfo &currentTrack = m_parser->GetTrack(m_TrackNo);
		{
			packet_decoder::matroska_setup setup;
			pfc::string8 codec_temp(currentTrack.codecID.c_str());
			setup.codec_id = codec_temp;
			setup.sample_rate = (unsigned)currentTrack.samplesPerSec;
			setup.sample_rate_output = (unsigned)currentTrack.samplesOutputPerSec;
			setup.channels = (unsigned)currentTrack.channels;
			setup.codec_private_size = currentTrack.codecPrivate.size();
            if (setup.codec_private_size) {
				setup.codec_private = &currentTrack.codecPrivate.at(0);
            } else {
				setup.codec_private = NULL;
            }

			packet_decoder::g_open(m_decoder, p_decode, packet_decoder::owner_matroska, 0, &setup, sizeof(setup), p_abort);
			if (m_decoder == NULL)
			{
				console::error(uStringPrintf("Matroska: unable to find a \"%s\" packet decoder object.", (const char*)codec_temp));
				cleanup();
				throw exception_io_data();
			} else {
				hprintf(L"Matroska: using '%S' decoder.\n", (const char*)codec_temp);
			}
		}
		m_expected_channels = currentTrack.channels;
		m_expected_sample_rate = (unsigned)(currentTrack.samplesOutputPerSec == 0 ? currentTrack.samplesPerSec : currentTrack.samplesOutputPerSec);
		m_expected_bitspersample = currentTrack.bitsPerSample;
		//*
		if (!p_decode && m_decoder->analyze_first_frame_supported()) {
			if (m_frame != NULL) {
				delete m_frame;
				m_frame = NULL;
			}
			// The timecode scale in Matroska is in milliseconds, but foobar deals in seconds
			m_timescale = m_parser->GetTimecodeScale() * 1000;
			m_length = duration_to_samples(m_parser->GetDuration());
			m_position = 0;
			m_skip_samples = 0;
			m_skip_frames = 0;
			m_frame_remaining = 0;
			m_frame = 0;
			m_frame = m_parser->ReadFirstFrame();
			if (m_frame != NULL) {
				unsigned int buffer_size = m_frame->dataBuffer.at(0).size();
				m_buffer.set_size(buffer_size);
				m_buffer.set_data_fromptr(&m_frame->dataBuffer.at(0).at(0), buffer_size);
				m_decoder->analyze_first_frame(m_buffer.get_ptr(), buffer_size, p_abort);
			}
		}
		//*/
	}
};

static input_factory_t<input_matroska> g_input_matroska_factory;

#ifdef ARCH_SSE
#define INPUT_MATROSKA_NAME "Matroska Plugin (/arch:SSE)"
#else
#ifdef _DEBUG
#define INPUT_MATROSKA_NAME "Matroska Plugin (debug) build. "__DATE__" "__TIME__
#else
#define INPUT_MATROSKA_NAME "Matroska Plugin"
#endif
#endif

namespace {
static const pfc::string8 version = pfc::string_printf(
    "ported to 0.9 by Haru Ayana <ayana@matroska.org>\n"
    "Copyright (C) 2003-2004 Jory Stone (jcsston@toughguy.net)\n"
    "Copyright (C) 2003-2004 Peter Pawlowski\n"
    "libebml %s + libmatroska %s", MatroskaVersion::lib_ebml(), MatroskaVersion::lib_matroska()
);
}

DECLARE_COMPONENT_VERSION(
    INPUT_MATROSKA_NAME,
    "0.9.2.1",
    version
);

DECLARE_FILE_TYPE("Matroska Audio files","*.MKA");
