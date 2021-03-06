/**
 * Implements kernels for flow densification
 */

// System
#include <iostream>
#include <chrono>
#include <string>
#include <stdexcept>

// CUDA
#include <cuda.h>
#include <cuda_runtime.h>

#include "densify.h"

__global__ void kernelDensifyPatch(
    float* pDeviceCostDiff, float* pDeviceFlowOut, float* pDeviceWeights,
    dev_patch_state* states, int ip,
    int midpointX, int midpointY,
    int width, int height,
    int patchSize, float minErrVal) {

  float flowX = states[ip].p_curx;
  float flowY = states[ip].p_cury;
  int lower_bound = -patchSize / 2;

  int x = threadIdx.x + lower_bound;
  int y = blockIdx.x  + lower_bound;

  int xt = x + midpointX;
  int yt = y + midpointY;

  if (xt >= 0 && yt >= 0 && xt < width && yt < height) {

    int i = yt * width + xt;
    int j = blockIdx.x * patchSize + threadIdx.x;

    float absw = (float) (fmaxf(minErrVal, pDeviceCostDiff[3 * j]));
    absw += (float) (fmaxf(minErrVal, pDeviceCostDiff[3 * j + 1]));
    absw += (float) (fmaxf(minErrVal, pDeviceCostDiff[3 * j + 2]));
    absw = 1.0 / absw;

    // Weight contribution RGB
    pDeviceWeights[i] += absw;

    pDeviceFlowOut[2 * i] += flowX * absw;
    pDeviceFlowOut[2 * i + 1] += flowY * absw;
  }

}


__global__ void kernelDensifyPatches(
    float** costs, float* flow, float* weights,
    dev_patch_state* states,
    int width, int height,
    int patch_size, float minErrVal) {

  int patchId = blockIdx.x;
  int tid = threadIdx.x;

  int lower_bound = -patch_size / 2;
  int xt = states[patchId].midpoint_orgx + lower_bound;
  int yt = states[patchId].midpoint_orgy + lower_bound;
  int offset = (xt + yt * width) + tid;

  float* cost = costs[patchId];

  for (int i = 3 * tid, j = offset; i < patch_size * patch_size * 3;
      i += 3 * patch_size, j += width) {

    if (j >= 0 && j < width * height) {

      float absw = (float) (fmaxf(minErrVal, cost[i]));
      absw += (float) (fmaxf(minErrVal, cost[i + 1]));
      absw += (float) (fmaxf(minErrVal, cost[i + 2]));
      absw = 1.0 / absw;

      // Weight contribution RGB
      atomicAdd(&weights[j], absw);

      atomicAdd(&flow[2 * j], states[patchId].p_curx * absw);
      atomicAdd(&flow[2 * j + 1], states[patchId].p_cury * absw);
    }

  }

}


__global__ void kernelNormalizeFlow(
    float* pDeviceFlowOut, float* flow,
    float* pDeviceWeights, int N, int numBlocks) {

  int i = blockIdx.x * blockDim.x + threadIdx.x;

  for (; i < N; i+= blockDim.x * numBlocks) {
    if (pDeviceWeights[i / 2] > 0) 
      flow[i] = pDeviceFlowOut[i] / pDeviceWeights[i / 2];
  }

}

namespace cu {

  void densifyPatch(
      float* pDeviceCostDiff, float* pDeviceFlowOut, float* pDeviceWeights,
      dev_patch_state* states, int ip,
      int midpointX, int midpointY,
      int width, int height,
      int patchSize, float minErrVal) {

    int nBlocks = patchSize;
    int nThreadsPerBlock = patchSize;

    kernelDensifyPatch<<<nBlocks, nThreadsPerBlock>>>(
        pDeviceCostDiff, pDeviceFlowOut, pDeviceWeights,
        states, ip,
        midpointX, midpointY,
        width, height,
        patchSize, minErrVal);
  }

  void normalizeFlow(float* pHostFlowOut,
      float* pDeviceFlowOut, float* pDeviceWeights, int N) {

    int nThreadsPerBlock = 64;
    int nBlocks = 10;

    kernelNormalizeFlow<<<nBlocks, nThreadsPerBlock>>>(pDeviceFlowOut,
        pHostFlowOut, pDeviceWeights, N, nBlocks);
  }

  void densifyPatches(
      float** costs, float* flow, float* weights,
      dev_patch_state* states, int n_patches,
      const opt_params* op, const img_params* i_params) {

    int nBlocks = n_patches;
    int nThreadsPerBlock = op->patch_size;

    kernelDensifyPatches<<<nBlocks, nThreadsPerBlock>>>(
        costs, flow, weights,
        states,
        i_params->width, i_params->height,
        op->patch_size, op->min_errval);

  }

}
