﻿//
//      ██╗██████╗     ██╗     ██╗██████╗ ███████╗
//      ██║██╔══██╗    ██║     ██║██╔══██╗██╔════╝		** MiniaudioCpp **
//      ██║██████╔╝    ██║     ██║██████╔╝███████╗
// ██   ██║██╔═══╝     ██║     ██║██╔══██╗╚════██║		https://github.com/Jaytheway/MiniaudioCpp
// ╚█████╔╝██║         ███████╗██║██████╔╝███████║
//  ╚════╝ ╚═╝         ╚══════╝╚═╝╚═════╝ ╚══════╝
//
//   Copyright 2024 Jaroslav Pevno, MiniaudioCpp is offered under the terms of the ISC license:
//
//   Permission to use, copy, modify, and/or distribute this software for any purpose with or
//   without fee is hereby granted, provided that the above copyright notice and this permission
//   notice appear in all copies. THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//   WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
//   AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
//   CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
//   WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
//   CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#pragma once

namespace JPL
{
	template<class TReader>
	class StreamReader
	{
	public:
		using InternalReader = TReader;

		StreamReader() requires (InternalReader()) { mReader = new InternalReader(); }
		virtual ~StreamReader() { delete mReader; }
		
		explicit StreamReader(const char* filePath) requires (InternalReader(filePath)) { mReader = new InternalReader(filePath); }
		explicit StreamReader(InternalReader* reader) { JPL_ASSERT(reader != nullptr); mReader = reader; }

		JPL_INLINE size_t GetStreamPosition() const { return mReader->GetStreamPosition(); }
		JPL_INLINE void SetStreamPosition(size_t position) { mReader->SetStreamPosition(position); }
		
		void ReadData(char* destination, size_t size) { mReader->ReadData(destination, size); }
		
		InternalReader& GetInternal() { return *mReader; }

	private:
		InternalReader* mReader = nullptr;
	};

} // namespace JPL