/* FasTC
 * Copyright (c) 2012 University of North Carolina at Chapel Hill. All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its documentation for educational, 
 * research, and non-profit purposes, without fee, and without a written agreement is hereby granted, 
 * provided that the above copyright notice, this paragraph, and the following four paragraphs appear 
 * in all copies.
 *
 * Permission to incorporate this software into commercial products may be obtained by contacting the 
 * authors or the Office of Technology Development at the University of North Carolina at Chapel Hill <otd@unc.edu>.
 *
 * This software program and documentation are copyrighted by the University of North Carolina at Chapel Hill. 
 * The software program and documentation are supplied "as is," without any accompanying services from the 
 * University of North Carolina at Chapel Hill or the authors. The University of North Carolina at Chapel Hill 
 * and the authors do not warrant that the operation of the program will be uninterrupted or error-free. The 
 * end-user understands that the program was developed for research purposes and is advised not to rely 
 * exclusively on the program for any reason.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE AUTHORS BE LIABLE TO ANY PARTY FOR 
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING OUT OF THE 
 * USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE 
 * AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING, 
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE AND ANY 
 * STATUTORY WARRANTY OF NON-INFRINGEMENT. THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND THE UNIVERSITY 
 * OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS HAVE NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, 
 * ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Please send all BUG REPORTS to <pavel@cs.unc.edu>.
 *
 * The authors may be contacted via:
 *
 * Pavel Krajcevski
 * Dept of Computer Science
 * 201 S Columbia St
 * Frederick P. Brooks, Jr. Computer Science Bldg
 * Chapel Hill, NC 27599-3175
 * USA
 * 
 * <http://gamma.cs.unc.edu/FasTC/>
 */

#include "TexComp.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cassert>
#include <iostream>
#include <string.h>

#include "BPTCCompressor.h"
#include "CompressionFormat.h"
#include "CompressionFuncs.h"
#include "DXTCompressor.h"
#include "ETCCompressor.h"
#include "ImageFile.h"
#include "Pixel.h"
#include "PVRTCCompressor.h"
#include "Thread.h"
#include "ThreadGroup.h"
#include "WorkerQueue.h"

using FasTC::CompressionJob;
using FasTC::CompressionJobList;
using FasTC::ECompressionFormat;

template <typename T>
static void clamp(T &x, const T &minX, const T &maxX) {
  x = std::max(std::min(maxX, x), minX);
}

template <typename T>
static inline T sad(const T &a, const T &b) {
  return (a > b)? a - b : b - a;
}

static BPTCC::CompressionSettings gBPTCSettings;
static void CompressBPTC(const CompressionJob &cj) {
  BPTCC::Compress(cj, gBPTCSettings);
}

static void CompressBPTCWithStats(const CompressionJob &cj,
                                  std::ostream *strm) {
  BPTCC::CompressWithStats(cj, strm, gBPTCSettings);
}

static void CompressPVRTC(const CompressionJob &cj) {
  PVRTCC::Compress(cj);
}

static void CompressPVRTCLib(const CompressionJob &cj) {
#ifdef PVRTEXLIB_FOUND
  PVRTCC::CompressPVRLib(cj);
#else
  fprintf(stderr, "WARNING: PVRTexLib not found, defaulting to FasTC implementation.\n");
  PVRTCC::Compress(cj);
#endif
}

SCompressionSettings:: SCompressionSettings()
  : format(FasTC::eCompressionFormat_BPTC)
  , bUseSIMD(false)
  , iNumThreads(1)
  , iQuality(50)
  , iNumCompressions(1)
{
  clamp(iQuality, 0, 256);
}

static  CompressionFuncWithStats ChooseFuncFromSettingsWithStats(const SCompressionSettings &s) {
  switch(s.format) {

    case FasTC::eCompressionFormat_BPTC:
    {
#ifdef FOUND_NVTT_BPTC_EXPORT
      if(s.bUseNVTT)
       return BPTCC::CompressNVTTWithStats;
      else
#endif
       return CompressBPTCWithStats;
    }
    break;
    
    default:
    {
      assert(!"Not implemented!");
      return NULL;
    }
  }
  return NULL;
}

static CompressionFunc ChooseFuncFromSettings(const SCompressionSettings &s) {
  switch(s.format) {
    case FasTC::eCompressionFormat_BPTC:
    {
      gBPTCSettings.m_NumSimulatedAnnealingSteps = s.iQuality;
#ifdef HAS_SSE_41
      if(s.bUseSIMD) {
        return BPTCC::CompressImageBPTCSIMD;
      }
#endif

#ifdef FOUND_NVTT_BPTC_EXPORT
      if(s.bUseNVTT)
       return BPTCC::CompressNVTT;
      else
#endif
       return CompressBPTC;
    }
    break;

    case FasTC::eCompressionFormat_DXT1:
      return DXTC::CompressImageDXT1;

    case FasTC::eCompressionFormat_DXT5:
      return DXTC::CompressImageDXT5;

    case FasTC::eCompressionFormat_PVRTC4:
    {
      if(s.bUsePVRTexLib) {
        return CompressPVRTCLib;
      } else {
        return CompressPVRTC;
      }
    }

    case FasTC::eCompressionFormat_ETC1:
      return ETCC::Compress_RG;

    default:
    {
      assert(!"Not implemented!");
      return NULL;
    }
  }
  return NULL;
}

static void ReportError(const char *msg) {
  fprintf(stderr, "TexComp -- %s\n", msg);
}

static double CompressImageInSerial(
  const CompressionJob &job,
  const SCompressionSettings &settings
) {
  CompressionFunc f = ChooseFuncFromSettings(settings);
  CompressionFuncWithStats fStats = NULL;
  if (settings.logStream) {
    fStats = ChooseFuncFromSettingsWithStats(settings);
  }

  double cmpTimeTotal = 0.0;

  StopWatch stopWatch = StopWatch();
  for(int i = 0; i < settings.iNumCompressions; i++) {

    stopWatch.Reset();
    stopWatch.Start();

    if(fStats && settings.logStream) {
      (*fStats)(job, settings.logStream);
    } else {
      (*f)(job);
    }

    stopWatch.Stop();

    cmpTimeTotal += stopWatch.TimeInMilliseconds();
  }

  double cmpTime = cmpTimeTotal / double(settings.iNumCompressions);
  return cmpTime;
}

#ifdef HAS_ATOMICS
class AtomicThreadUnit : public TCCallable {
  CompressionJobList &m_CompressionJobList;
  TCBarrier *m_Barrier;
  CompressionFunc m_CmpFnc;

 public:
  AtomicThreadUnit(
    CompressionJobList &_cjl,
    TCBarrier *barrier,
    CompressionFunc f
  ) : TCCallable(),
    m_CompressionJobList(_cjl),
    m_Barrier(barrier),
    m_CmpFnc(f)
  { }

  virtual ~AtomicThreadUnit() { }
  virtual void operator()() {
    m_Barrier->Wait();
    if(m_CmpFnc == CompressBPTC) {
      BPTCC::CompressAtomic(m_CompressionJobList);
    }
    else {
      assert(!"I don't know what we're compressing...");
    }
  }
};

static double CompressImageWithAtomics(
  const CompressionJob &cj,
  const SCompressionSettings &settings
) {
  CompressionFunc f = ChooseFuncFromSettings(settings);
  
  // Setup compression list...
  const int nTimes = settings.iNumCompressions;
  CompressionJobList cjl (nTimes);
  for(int i = 0; i < nTimes; i++) {
    if(!cjl.AddJob(cj)) {
      assert(!"Error adding compression job to job list!");
    }
  }

  const int nThreads = settings.iNumThreads;

  // Allocate resources...
  TCBarrier barrier (nThreads+1);
  TCThread **threads = (TCThread **)malloc(nThreads * sizeof(TCThread *));
  AtomicThreadUnit **units = (AtomicThreadUnit **)malloc(nThreads * sizeof(AtomicThreadUnit *));

  // Launch threads...
  for(int i = 0; i < nThreads; i++) {
    AtomicThreadUnit *u = new AtomicThreadUnit(cjl, &barrier, f);
    threads[i] = new TCThread(*u);
    units[i] = u;
  }

  // Wait here to make sure that our timer is correct...
  barrier.Wait();

  StopWatch sw;
  sw.Start();

  // Wait for threads to finish
  for(int i = 0; i < nThreads; i++) {
    threads[i]->Join();
  }
  sw.Stop();

  // Cleanup
  for(int i = 0; i < nThreads; i++)
    delete threads[i];
  free(threads);
  for(int i = 0; i < nThreads; i++)
    delete units[i];
  free(units);

  // Compression time
  double cmpTimeTotal = sw.TimeInMilliseconds();
  return cmpTimeTotal / double(settings.iNumCompressions);
}
#else  // HAS_ATOMICS
static double CompressImageWithAtomics(
  const CompressionJob &cj,
  const SCompressionSettings &settings
) {
  fprintf(stderr, "Compiler does not support atomic operations!");
}
#endif

static double CompressThreadGroup(ThreadGroup &tgrp, const SCompressionSettings &settings) {
  if(!(tgrp.PrepareThreads())) {
    assert(!"Thread group failed to prepare threads?!");
    return -1.0f;
  }

  double cmpTimeTotal = 0.0;
  for(int i = 0; i < settings.iNumCompressions; i++) {
    if(i > 0)
      tgrp.PrepareThreads();

    tgrp.Start();
    tgrp.Join();

    StopWatch stopWatch = tgrp.GetStopWatch();
    cmpTimeTotal += tgrp.GetStopWatch().TimeInMilliseconds();
  }

  tgrp.CleanUpThreads();
  return cmpTimeTotal;
}

static double CompressImageWithThreads(
  const CompressionJob &job,                                     
  const SCompressionSettings &settings
) {

  CompressionFunc f = ChooseFuncFromSettings(settings);
  CompressionFuncWithStats fStats = ChooseFuncFromSettingsWithStats(settings);

  double cmpTimeTotal = 0.0;
  if(fStats && settings.logStream) {
    ThreadGroup tgrp (settings.iNumThreads, job, fStats, settings.logStream);
    cmpTimeTotal = CompressThreadGroup(tgrp, settings);
  }
  else {
    ThreadGroup tgrp (settings.iNumThreads, job, f);
    cmpTimeTotal = CompressThreadGroup(tgrp, settings);
  }

  double cmpTime = cmpTimeTotal / double(settings.iNumCompressions);
  return cmpTime;
}

static double RunWorkerQueue(WorkerQueue &wq) {
  wq.Run();
  return wq.GetStopWatch().TimeInMilliseconds();
}

static double CompressImageWithWorkerQueue(
  const CompressionJob &job,
  const SCompressionSettings &settings
) {
  CompressionFunc f = ChooseFuncFromSettings(settings);
  CompressionFuncWithStats fStats = ChooseFuncFromSettingsWithStats(settings);

  double cmpTimeTotal = 0.0;
  if(fStats && settings.logStream) {
    WorkerQueue wq (
      settings.iNumCompressions,
      settings.iNumThreads,
      settings.iJobSize,
      job,
      fStats,
      settings.logStream
    );
    cmpTimeTotal = RunWorkerQueue(wq);
  }
  else {
    WorkerQueue wq (
      settings.iNumCompressions,
      settings.iNumThreads,
      settings.iJobSize,
      job,
      f
    );
    cmpTimeTotal = RunWorkerQueue(wq);
  }

  return cmpTimeTotal / double(settings.iNumCompressions);
}

template<typename PixelType>
CompressedImage *CompressImage(
  FasTC::Image<PixelType> *img, const SCompressionSettings &settings
) {
  if(!img) return NULL;

  uint32 width = img->GetWidth();
  uint32 height = img->GetHeight();
  assert(width > 0);
  assert(height > 0);

  // Make sure that the width and height of the image is a multiple of
  // the block size of the format
  uint32 blockDims[2];
  FasTC::GetBlockDimensions(settings.format, blockDims);
  if ((width % blockDims[0]) != 0 || (height % blockDims[1]) != 0) {
    ReportError("WARNING - Image size is not a multiple of block size. Padding with zeros...");
    uint32 newWidth = ((width + (blockDims[0] - 1)) / blockDims[0]) * blockDims[0];
    uint32 newHeight = ((height + (blockDims[1] - 1)) / blockDims[1]) * blockDims[1];

    assert(newWidth > width || newHeight > height);
    assert(newWidth % blockDims[0] == 0);
    assert(newHeight % blockDims[1] == 0);

    width = newWidth;
    height = newHeight;
  }

  uint32 dataSz = width * height * 4;
  uint32 *data = new uint32[dataSz / 4];
  memset(data, 0, dataSz);

  CompressedImage *outImg = NULL;

  // Allocate data based on the compression method
  uint32 cmpDataSz = CompressedImage::GetCompressedSize(dataSz, settings.format);

  // Make sure that we have RGBA data...
  img->ComputePixels();
  for(uint32 j = 0; j < img->GetHeight(); j++) {
    for(uint32 i = 0; i < img->GetWidth(); i++) {
      data[j * width + i] = (*img)(i, j).Pack();
    }
  }

  unsigned char *cmpData = new unsigned char[cmpDataSz];
  uint8 *dataPtr = reinterpret_cast<uint8 *>(data);
  if (CompressImageData(dataPtr, width, height, cmpData, cmpDataSz, settings)) {
    outImg = new CompressedImage(width, height, settings.format, cmpData);
  }

  delete [] data;
  delete [] cmpData;
  return outImg;
}

// !FIXME! Ideally, we wouldn't have to do this because there would be a way to instantiate this
// function in the header or using some fancy template metaprogramming. I can't think of the way 
// at the moment.
template CompressedImage *CompressImage(FasTC::Image<FasTC::Pixel> *, const SCompressionSettings &settings);

bool CompressImageData(
  const uint8 *data, 
  const uint32 width,
  const uint32 height,
  uint8 *compressedData,
  const uint32 cmpDataSz,
  const SCompressionSettings &settings
) {

  uint32 dataSz = width * height * 4;

  // Make sure that platform supports SSE if they chose this
  // option...
  #ifndef HAS_SSE_41
  if(settings.bUseSIMD) {
    ReportError("Platform does not support SIMD!\n");
    return false;
  }
  #endif

  #ifndef HAS_ATOMICS
  if(settings.bUseAtomics) {
    ReportError("Compiler's atomic operations are not supported!\n");
    return false;
  }
  #endif

  if(dataSz <= 0) {
    ReportError("No data sent to compress!");
    return false;
  }

  uint32 numThreads = settings.iNumThreads;
  if(settings.format == FasTC::eCompressionFormat_PVRTC4 &&
     (settings.iNumThreads > 1 || settings.logStream)) {
    if(settings.iNumThreads > 1) {
      ReportError("WARNING - PVRTC compressor does not support multithreading.");
      numThreads = 1;
    }

    if(settings.logStream) {
      ReportError("WARNING - PVRTC compressor does not support stat collection.");
    }
  }

  uint32 blockDims[2];
  FasTC::GetBlockDimensions(settings.format, blockDims);
  if ((width % blockDims[0]) != 0 || (height % blockDims[1]) != 0) {
    ReportError("ERROR - CompressImageData: width or height is not multiple of block dimension");
    return false;
  } else if (settings.format == FasTC::eCompressionFormat_PVRTC4 &&
             ((width & (width - 1)) != 0 ||
              (height & (height - 1)) != 0 ||
              width != height)) {
    ReportError("ERROR - CompressImageData: PVRTC4 images must be square and power-of-two.");
    return false;
  }

  // Allocate data based on the compression method
  uint32 compressedDataSzNeeded =
    CompressedImage::GetCompressedSize(dataSz, settings.format);

  if(compressedDataSzNeeded == 0) {
    ReportError("Unknown compression format");
    return false;
  }
  else if(compressedDataSzNeeded > cmpDataSz) {
    ReportError("Not enough space for compressed data!");
    return false;
  }

  if(ChooseFuncFromSettings(settings)) {

    CompressionJob cj(settings.format, data, compressedData, width, height);

    double cmpMSTime = 0.0;
    if(numThreads > 1) {
      if(settings.bUseAtomics) {
        cmpMSTime = CompressImageWithAtomics(cj, settings);
      } else if(settings.iJobSize > 0) {
        cmpMSTime = CompressImageWithWorkerQueue(cj, settings);
      } else {
        cmpMSTime = CompressImageWithThreads(cj, settings);
      }
    }
    else {
      cmpMSTime = CompressImageInSerial(cj, settings);
    }

    // Report compression time
    fprintf(stdout, "Compression time: %0.3f ms\n", cmpMSTime);
  }
  else {
    ReportError("Could not find adequate compression function for specified settings");
    return false;
  }

  return true;
}

void YieldThread() {
  TCThread::Yield();
}
