/*
 *  Part of the foobar2000 Matroska plugin
 *
 *  Copyright (C) Jory Stone (jcsston at toughguy net) - 2003-2004
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
    \file matroska_parser.h
		\version $Id$
    \brief An audio slated Matroska Reader+Parser
		\author Jory Stone     <jcsston @ toughguy.net>
*/

#ifndef _MATROSKA_PARSER_H_
#define _MATROSKA_PARSER_H_

#define MULTITRACK 1

#include "../SDK/foobar2000.h"
#include "../helpers/helpers.h"
#include "../../pfc/pfc.h"
#include "Foobar2000ReaderIOCallback.h"
#include "DbgOut.h"
#include <queue>
#include <deque>
#include <boost/shared_ptr.hpp>

// libebml includes
#include "ebml/StdIOCallback.h"
#include "ebml/EbmlTypes.h"
#include "ebml/EbmlHead.h"
#include "ebml/EbmlVoid.h"
#include "ebml/EbmlCrc32.h"
#include "ebml/EbmlSubHead.h"
#include "ebml/EbmlStream.h"
#include "ebml/EbmlBinary.h"
#include "ebml/EbmlString.h"
#include "ebml/EbmlUnicodeString.h"
#include "ebml/EbmlContexts.h"
#include "ebml/EbmlVersion.h"

// libmatroska includes
#include "matroska/KaxConfig.h"
#include "matroska/KaxBlock.h"
#include "matroska/KaxSegment.h"
#include "matroska/KaxContexts.h"
#include "matroska/KaxSeekHead.h"
#include "matroska/KaxTracks.h"
#include "matroska/KaxInfo.h"
#include "matroska/KaxInfoData.h"
#include "matroska/KaxTags.h"
#include "matroska/KaxTag.h"
#include "matroska/KaxTagMulti.h"
#include "matroska/KaxCluster.h"
#include "matroska/KaxClusterData.h"
#include "matroska/KaxTrackAudio.h"
#include "matroska/KaxTrackVideo.h"
#include "matroska/KaxAttachments.h"
#include "matroska/KaxAttached.h"
#include "matroska/KaxChapters.h"
#include "matroska/KaxVersion.h"

#define TIMECODE_SCALE  1000000
#define MAX_UINT64 0xFFFFFFFFFFFFFFFF
#define _DELETE(__x) if (__x) { delete __x; __x = NULL; }

//Memory Leak Debuging define
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC 
#include <stdlib.h>
#include <crtdbg.h>
#endif

#ifdef _DEBUG
   #define DEBUG_CLIENTBLOCK   new( _CLIENT_BLOCK, __FILE__, __LINE__)
#else
   #define DEBUG_CLIENTBLOCK
#endif // _DEBUG

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

#ifdef _DEBUG
#endif

using namespace LIBEBML_NAMESPACE;
using namespace LIBMATROSKA_NAMESPACE;

typedef std::vector<uint8> ByteArray;
typedef boost::shared_ptr<EbmlElement> ElementPtr;

class MatroskaVersion {
public:
    static const char * lib_ebml() { return EbmlCodeVersion.c_str(); };
    static const char * lib_matroska() { return KaxCodeVersion.c_str(); };
};

class MatroskaAttachment {
public:
	MatroskaAttachment();

	UTFstring FileName;
	std::string MimeType;
	UTFstring Description;
	UTFstring SourceFilename;
	uint64 SourceStartPos;
	uint64 SourceDataLength;
};

class MatroskaAudioFrame {
public:
	MatroskaAudioFrame();
	void Reset();
    double get_duration() {
        return static_cast<double>(duration / 1000000000.0);
    }

	uint64 timecode;
	uint64 duration;
	std::vector<ByteArray> dataBuffer;
	/// Linked-list for laced frames
    uint64 add_id;
    ByteArray additional_data_buffer;
};


struct MatroskaMetaSeekClusterEntry {
	uint32 clusterNo;
	uint64 filePos;
	uint64 timecode;
};

class MatroskaSimpleTag {
public:
	MatroskaSimpleTag();

	UTFstring name;
	UTFstring value;
	uint32 defaultFlag;
	std::string language;

	bool hidden;
	bool removalPending;
};

class MatroskaTagInfo {
public:
	MatroskaTagInfo();
	void SetTagValue(const char *name, const char *value, int index = 0);
	void MarkAllAsRemovalPending();
	void RemoveMarkedTags();
	
	uint64 targetTrackUID;
	uint64 targetEditionUID;
	uint64 targetChapterUID;
	uint64 targetAttachmentUID;
	uint32 targetTypeValue;
	std::string targetType;

	std::vector<MatroskaSimpleTag> tags;
};

struct MatroskaChapterDisplayInfo {
	MatroskaChapterDisplayInfo();

	UTFstring string;
	std::string lang;
	std::string country;
};

class MatroskaChapterInfo {
public:
	MatroskaChapterInfo();

	uint64 chapterUID;
	uint64 timeStart;
	uint64 timeEnd;
	/// Vector of all the tracks this chapter applies to
	/// if it's empty then this chapter applies to all tracks
	std::vector<uint64> tracks;
	/// Vector of strings we can display for chapter
	std::vector<MatroskaChapterDisplayInfo> display;
	std::vector<MatroskaChapterInfo> subChapters;
};

class MatroskaEditionInfo {
public:
	MatroskaEditionInfo();

	uint64 editionUID;
	/// Vector of all the tracks this edition applies to
	/// if it's empty then this edition applies to all tracks
	std::vector<uint64> tracks;
};

class MatroskaTrackInfo {
	public:
		/// Initializes the class
		MatroskaTrackInfo();
		/// Initializes the class with a UID
		/// \param trackUID The UID to use for the new MatroskaTrackInfo
		//MatroskaTrackInfo(uint64 trackUID);
		/// Destroys the class
		//~MatroskaTrackInfo();


		uint16 trackNumber;
		uint64 trackUID;		
		std::string codecID;
		std::vector<BYTE> codecPrivate;
		bool codecPrivateReady;
		
		UTFstring name;
		std::string language;
		double duration;

        uint8 channels; 
        double samplesPerSec; 
        double samplesOutputPerSec;     
        uint8 bitsPerSample;
        uint32 avgBytesPerSec; 
        uint64 defaultDuration;
};

typedef boost::shared_ptr<MatroskaMetaSeekClusterEntry> cluster_entry_ptr;

class MatroskaAudioParser {
public:
	MatroskaAudioParser(service_ptr_t<file> input, abort_callback & p_abort);
	~MatroskaAudioParser();

	/// The main header parsing function
	/// \return 0 File parsed ok
	/// \return 1 Failed
	int Parse(bool bInfoOnly = false, bool bBreakAtClusters = true);
	/// Writes the tags to the current matroska file
	/// \param info All the tags we need to write
	/// \return 0 Tags written A OK
	/// \return 1 Failed to write tags
	int WriteTags();
	/// Set the info tags to the current tags file in memory
	void SetTags(const file_info &info);

	MatroskaTrackInfo &GetTrack(uint16 trackNo) { return m_Tracks.at(trackNo); };
	uint64 GetTimecodeScale() { return m_TimecodeScale; };
	/// Returns an adjusted duration of the file
	double GetDuration();
	/// Returns the track index of the first decodable track
	int32 GetFirstAudioTrack();

	double TimecodeToSeconds(uint64 code,unsigned samplerate_hint = 44100);
	uint64 SecondsToTimecode(double seconds);

	/// Set the fb2k info from the matroska file
	/// \param info This will be filled up with tags ;)
	bool SetFB2KInfo(file_info &info, t_uint32 p_subsong);

	/// Get the foobar2000 style format string
//	const char *GetFoobar2000Format(uint16 trackNo, bool bSetupCodecPrivate = true);

	/// Set the current track to read data from
	void SetCurrentTrack(uint32 newTrackNo);
	/// Set the subsong to play, this adjusts all the duration/timecodes 
	/// reported in public functions. So only use this if you are expecting that to happen
	/// \param subsong This should be within the range of the chapters vector
	void SetSubSong(int subsong);

	std::vector<MatroskaEditionInfo> &GetEditions() { return m_Editions; };
	std::vector<MatroskaChapterInfo> &GetChapters() { return m_Chapters; };
	std::vector<MatroskaTrackInfo> &GetTracks() { return m_Tracks; };
	uint32 GetAudioTrackCount();
	uint32 GetAudioTrackIndex(uint32 index);

	int32 GetAvgBitrate();
	/// Seek to a position
	/// \param seconds The absolute position to seek to, in seconds			
	/// \return Current postion offset from where it was requested
	/// If you request to seek to 2.0 and we can only seek to 1.9
	/// the return value would be 100 * m_TimcodeScale

	bool skip_frames_until(double destination,unsigned & frames,double & last_timecode_delta,unsigned hint_samplerate);
	void flush_queue();
	uint64 get_current_frame_timecode();
	bool Seek(double seconds,unsigned & frames_to_skip,double & time_to_skip,unsigned samplerate_hint);

	/// Seek to a position
	/// \param frame The MatroskaAudioFrame struct to store the frame	
	/// \return 0 If read ok	
	/// \return 1 If file could not be read or it not open	
	/// \return 2 End of track (EOT)
	MatroskaAudioFrame * ReadSingleFrame();
    MatroskaAudioFrame * ReadFirstFrame();

	UTFstring GetSegmentFileName() { return m_SegmentFilename; }
    typedef pfc::list_t<MatroskaAttachment> attachment_list;
	attachment_list &GetAttachmentList() { return m_AttachmentList; }

protected:
	void Parse_MetaSeek(ElementPtr metaSeekElement, bool bInfoOnly);
	void Parse_Chapters(KaxChapters *chaptersElement);
	void Parse_Chapter_Atom(KaxChapterAtom *ChapterAtom);
	void Parse_Chapter_Atom(KaxChapterAtom *ChapterAtom, std::vector<MatroskaChapterInfo> &p_chapters);
	void Parse_Tags(KaxTags *tagsElement);
	int FillQueue();
	uint64 GetClusterTimecode(uint64 filePos);
	cluster_entry_ptr FindCluster(uint64 timecode);
	void CountClusters();
	void FixChapterEndTimes();
	// See if the edition uid is already in our vector
	// \return true Yes, we already have this uid
	// \return false Nope
	bool FindEditionUID(uint64 uid);
	// See if the chapter uid is already in our vector
	// \return true Yes, we already have this uid
	// \return false Nope
	bool FindChapterUID(uint64 uid);
	/// Adds the info tags to the current file in memory 
	void AddTags(file_info &info);
	
	MatroskaTagInfo *FindTagWithTrackUID(uint64 trackUID);
	MatroskaTagInfo *FindTagWithEditionUID(uint64 editionUID, uint64 trackUID = 0);
	MatroskaTagInfo *FindTagWithChapterUID(uint64 chapterUID, uint64 trackUID = 0);
	bool AreTagsIdenticalAtAllLevels(const char * name);
	bool AreTagsIdenticalAtEditionLevel(const char * name);
	bool AreTagsIdenticalAtChapterLevel(const char * name);
	void MarkHiddenTags();

	void SetAlbumTags(file_info &info, MatroskaTagInfo* AlbumTags, MatroskaTagInfo* TrackTags);
	void SetTrackTags(file_info &info, MatroskaTagInfo* TrackTags);
		

	Foobar2000ReaderIOCallback m_IOCallback;
	EbmlStream m_InputStream;
	/// The main/base/master element, should be the segment
	ElementPtr m_ElementLevel0;

	MatroskaEditionInfo *m_CurrentEdition;
	MatroskaChapterInfo *m_CurrentChapter;
	uint32 m_CurrentTrackNo;
	std::vector<MatroskaTrackInfo> m_Tracks;
	std::vector<MatroskaEditionInfo> m_Editions;
	std::vector<MatroskaChapterInfo> m_Chapters;
	std::vector<MatroskaTagInfo> m_Tags;
	
	/// This is the queue of buffered frames to deliver
	std::queue<MatroskaAudioFrame *> m_Queue;

	/// This is the index of clusters in the file, it's used to seek in the file
	// std::vector<MatroskaMetaSeekClusterEntry> m_ClusterIndex;
    std::vector<cluster_entry_ptr> m_ClusterIndex;

    attachment_list m_AttachmentList;

	uint64 m_CurrentTimecode;
	double m_Duration;
	uint64 m_TimecodeScale;
	UTFstring m_WritingApp;
	UTFstring m_MuxingApp;
	UTFstring m_FileTitle;
	int32 m_FileDate;
	UTFstring m_SegmentFilename;

	uint64 m_FileSize;
	uint64 m_TagPos;
	uint32 m_TagSize;
	uint32 m_TagScanRange;

	//pfc::alloc_fast<BYTE> m_framebuffer;
	//mem_block_factalloc<BYTE> m_framebuffer;
	//int UpperElementLevel;
private:
	// We have no need for these
	//MatroskaAudioParser(const MatroskaAudioParser &refParser) { };
	//MatroskaAudioParser &operator=(const MatroskaAudioParser &refParser) { return *this; };
};

typedef boost::shared_ptr<MatroskaAudioParser> matroska_parser_ptr;

void PrintChapters(std::vector<MatroskaChapterInfo> &theChapters);

class MatroskaSearch
{
private:
	static const int SEARCH_SOURCE_SIZE = 1024*64;
	static const int SEARCH_TABLE_SIZE = SEARCH_SOURCE_SIZE;
	static const int SEARCH_PATTERN_SIZE = 3;
    binary * source, * pattern;
	int pos;
    int  next[SEARCH_TABLE_SIZE], skip[SEARCH_TABLE_SIZE];
	bool Skip();
	bool Next();
public:
	MatroskaSearch(binary * p_source, binary * p_pattern)
	{
		ZeroMemory(next, sizeof(next));
		ZeroMemory(skip, sizeof(skip));
		source = p_source;
		pattern = p_pattern;
		Skip();
		Next();
	}
	~MatroskaSearch() {};
	int Match(unsigned int start = 0);
};

#endif // _MATROSKA_PARSER_H_
