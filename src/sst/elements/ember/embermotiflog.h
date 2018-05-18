// Copyright 2009-2018 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_SST_EMBER_MOTIF_LOG
#define _H_SST_EMBER_MOTIF_LOG

#include <stdio.h>
#include <sst/core/simulation.h>

namespace SST {
namespace Ember {

class EmberMotifLogRecord {
	public:
		EmberMotifLogRecord(const char* filePath) {
			loggerFile = fopen(filePath, "wt");
		}

		~EmberMotifLogRecord() {
			if(NULL != loggerFile) {
				fclose(loggerFile);
			}
		}

		void increment() {
#ifndef _SST_EMBER_DISABLE_PARALLEL
			__sync_fetch_and_add(&motifCount, 1);
#else
			motifCount++;
#endif
		}

		void decrement() {
#ifndef _SST_EMBER_DISABLE_PARALLEL
			__sync_fetch_and_sub(&motifCount, 1);
#else
			motifCount--;
#endif
		}

		uint32_t getCount() const {
			return motifCount;
		}

		FILE* getFile() {
			return loggerFile;
		}

		void invalidateFile() {
			loggerFile = NULL;
		}

	protected:
		FILE* loggerFile;
		uint32_t motifCount;
};

class EmberMotifLog {
	public:
		EmberMotifLog(const std::string logPath, const uint32_t jobID);
		~EmberMotifLog();
		void logMotifStart(const std::string name, const int motifNum);
	protected:
		EmberMotifLogRecord* logRecord;

};

}
}

#endif
