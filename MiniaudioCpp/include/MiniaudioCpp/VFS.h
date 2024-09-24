#pragma once

#include "Core.h"
#include "ErrorReporting.h"
#include "StreamReader.h"

#include "miniaudio/miniaudio.h"

#include <functional>

namespace JPL
{
	class JPLStreamReader;
}

namespace JPL
{
	// Default traits
	// StreamReader type can be overriden by creating VFS with different traits
	struct VFSTraits
	{
		using TStreamReader = StreamReader<JPLStreamReader>;
	};


	/** Our VFS used by miniaudio.
	*/
	template<typename Traits = VFSTraits>
	struct VFS
	{
		ma_vfs_callbacks cb;
		ma_allocation_callbacks allocationCallbacks;    /* Only used for the wchar_t version of open() on non-Windows platforms. */
		std::function<typename Traits::TStreamReader*(const char* filepath)> onCreateReader;
		std::function<size_t(const char* filepath)> onGetFileSize;

		ma_result init(const ma_allocation_callbacks* pAllocationCallbacks);

	private:
		class Callbacks final
		{
			struct VFSFile final
			{
				typename Traits::TStreamReader* Reader = nullptr;
				size_t FileSize = 0;
				~VFSFile() { delete Reader; }
			};

			friend ma_result VFS<Traits>::init(const ma_allocation_callbacks* pAllocationCallbacks);

			static ma_result Open(ma_vfs* pVFS, const char* pFilePath, ma_uint32 openMode, ma_vfs_file* pFile);
			static ma_result OpenW(ma_vfs* pVFS, const wchar_t* pFilePath, ma_uint32 openMode, ma_vfs_file* pFile);
			static ma_result Close(ma_vfs* pVFS, ma_vfs_file file);
			static ma_result Read(ma_vfs* pVFS, ma_vfs_file file, void* pDst, size_t sizeInBytes, size_t* pBytesRead);
			static ma_result Write(ma_vfs* pVFS, ma_vfs_file file, const void* pSrc, size_t sizeInBytes, size_t* pBytesWritten);
			static ma_result Seek(ma_vfs* pVFS, ma_vfs_file file, ma_int64 offset, ma_seek_origin origin);
			static ma_result Tell(ma_vfs* pVFS, ma_vfs_file file, ma_int64* pCursor);
			static ma_result Info(ma_vfs* pVFS, ma_vfs_file file, ma_file_info* pInfo);
			static ma_result copy_callbacks(ma_allocation_callbacks* pDst, const ma_allocation_callbacks* pSrc);
		};
	};
} // namespace JPL

//==============================================================================
//
//   Code beyond this point is implementation detail...
//
//==============================================================================
namespace JPL
{
	template<typename Traits>
	inline ma_result VFS<Traits>::init(const ma_allocation_callbacks* pAllocationCallbacks)
	{
		cb.onOpen = Callbacks::Open;
		cb.onOpenW = Callbacks::OpenW;
		cb.onClose = Callbacks::Close;
		cb.onRead = Callbacks::Read;
		cb.onWrite = Callbacks::Write;
		cb.onSeek = Callbacks::Seek;
		cb.onTell = Callbacks::Tell;
		cb.onInfo = Callbacks::Info;

		Callbacks::copy_callbacks(&allocationCallbacks, pAllocationCallbacks);

		return MA_SUCCESS;
	}

	template<typename Traits>
	inline ma_result VFS<Traits>::Callbacks::Open(ma_vfs* pVFS, const char* pFilePath, ma_uint32 openMode, ma_vfs_file* pFile)
	{
		if (!pFilePath || !pFile)
			return MA_INVALID_ARGS;

		if (openMode == MA_OPEN_MODE_WRITE)
			return MA_NOT_IMPLEMENTED;

		auto* vfs = (VFS<Traits>*)pVFS;

		auto* reader = vfs->onCreateReader(pFilePath);
		if (!reader)
			return MA_ERROR;

		size_t filesize = vfs->onGetFileSize(pFilePath);

		*pFile = new VFSFile(reader, filesize);

		return MA_SUCCESS;
	}

	template<typename Traits>
	inline ma_result VFS<Traits>::Callbacks::OpenW(ma_vfs* pVFS, const wchar_t* pFilePath, ma_uint32 openMode, ma_vfs_file* pFile)
	{
		JPL_ASSERT(false);
		return MA_NOT_IMPLEMENTED;
	}

	template<typename Traits>
	inline ma_result VFS<Traits>::Callbacks::Close(ma_vfs* pVFS, ma_vfs_file file)
	{
		if (!file)
			return MA_INVALID_ARGS;

		delete (VFSFile*)(file);

		return MA_SUCCESS;
	}

	template<typename Traits>
	inline ma_result VFS<Traits>::Callbacks::Read(ma_vfs* pVFS, ma_vfs_file file, void* pDst, size_t sizeInBytes, size_t* pBytesRead)
	{
		if (!file || !pDst)
			return MA_INVALID_ARGS;

		auto* vfsFile = (VFSFile*)file;
		auto* reader = vfsFile->Reader;

		const size_t toRead = std::min(sizeInBytes, vfsFile->FileSize - reader->GetStreamPosition());
		reader->ReadData((char*)pDst, toRead);

		if (pBytesRead)
			*pBytesRead = toRead;

		if (toRead != sizeInBytes)
			return MA_AT_END;
		else
			return MA_SUCCESS;
	}

	template<typename Traits>
	inline ma_result VFS<Traits>::Callbacks::Write(ma_vfs* pVFS, ma_vfs_file file, const void* pSrc, size_t sizeInBytes, size_t* pBytesWritten)
	{
		JPL_ASSERT(false);
		return MA_NOT_IMPLEMENTED;
	}

	template<typename Traits>
	inline ma_result VFS<Traits>::Callbacks::Seek(ma_vfs* pVFS, ma_vfs_file file, ma_int64 offset, ma_seek_origin origin)
	{
		if (!file)
			return MA_INVALID_ARGS;

		auto* vfsFile = (VFSFile*)file;
		auto* reader = vfsFile->Reader;

		size_t position = 0;

		switch (origin)
		{
		case ma_seek_origin_start:		position = offset; break;
		case ma_seek_origin_current:	position = reader->GetStreamPosition() + offset; break;
		case ma_seek_origin_end:		position = vfsFile->FileSize + offset; break;
		default:						position = offset; break;
		}

		reader->SetStreamPosition(position);

		return MA_SUCCESS;
	}

	template<typename Traits>
	inline ma_result VFS<Traits>::Callbacks::Tell(ma_vfs* pVFS, ma_vfs_file file, ma_int64* pCursor)
	{
		if (!file || !pCursor)
			return MA_INVALID_ARGS;

		auto* vfsFile = (VFSFile*)file;
		*pCursor = vfsFile->Reader->GetStreamPosition();

		return MA_SUCCESS;
	}

	template<typename Traits>
	inline ma_result VFS<Traits>::Callbacks::Info(ma_vfs* pVFS, ma_vfs_file file, ma_file_info* pInfo)
	{
		if (!file || !pInfo)
			return MA_INVALID_ARGS;

		auto* vfsFile = (VFSFile*)file;
		pInfo->sizeInBytes = vfsFile->FileSize;

		return MA_SUCCESS;
	}

	template<typename Traits>
	inline ma_result VFS<Traits>::Callbacks::copy_callbacks(ma_allocation_callbacks* pDst, const ma_allocation_callbacks* pSrc)
	{
		if (pDst == nullptr || pSrc == nullptr)
			return MA_INVALID_ARGS;

		if (pSrc->pUserData == nullptr && pSrc->onFree == nullptr && pSrc->onMalloc == nullptr && pSrc->onRealloc == nullptr)
		{
			return MA_INVALID_ARGS;
		}
		else
		{
			if (pSrc->onFree == nullptr || (pSrc->onMalloc == nullptr && pSrc->onRealloc == nullptr))
				return MA_INVALID_ARGS; // Invalid allocation callbacks.
			else
				*pDst = *pSrc;
		}

		return MA_SUCCESS;
	}
} // namespace JPL
