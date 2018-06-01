/**
 * @file	DIGITSClassifier.cpp
 * @author	Carroll Vance
 * @brief	Loads and manages a DIGITS ImageNet graph with TensorRT
 *
 * Copyright (c) 2018 Carroll Vance.
 * Copyright (c) 2017, NVIDIA CORPORATION. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <cassert>
#include <string>
#include <vector>

#include "NvInfer.h"
#include "NvCaffeParser.h"
#include "NvUtils.h"

#include "DIGITSClassifier.h"

#include "RTExceptions.h"

namespace jetson_tensorrt {

const std::string DIGITSClassifier::INPUT_NAME = "data";
const std::string DIGITSClassifier::OUTPUT_NAME = "prob";


DIGITSClassifier::DIGITSClassifier(std::string prototextPath,
		std::string modelPath, std::string cachePath, size_t nbChannels,
		size_t width, size_t height, size_t nbClasses, size_t maximumBatchSize,
		float3 imageNetMean, nvinfer1::DataType dataType, size_t maxNetworkSize) :
		CaffeRTEngine() {

	addInput(INPUT_NAME, nvinfer1::DimsCHW(nbChannels, height, width),
			sizeof(float));

	nvinfer1::Dims outputDims;
	outputDims.nbDims = 1;
	outputDims.d[0] = nbClasses;
	addOutput(OUTPUT_NAME, outputDims, sizeof(float));

	try {
		loadCache(cachePath);
	} catch (ModelDeserializeException& e) {
		loadModel(prototextPath, modelPath, maximumBatchSize, dataType,
				maxNetworkSize);
		saveCache(cachePath);
	}

	modelWidth = width;
	modelHeight = height;
	modelDepth = nbChannels;

	preprocessor = new ImageNetPreprocessor(imageNetMean);
}


DIGITSClassifier::~DIGITSClassifier() {
}

float* DIGITSClassifier::classifyRBGA(float* rbga, size_t width,
		size_t height) {

	//Load the image to device
	preprocessor->inputFromHost(rbga, width * height * sizeof(float4));

	//Convert to BGR
	float* preprocessedImageDevice = preprocessor->RBGAtoBGR(width, height,
			modelWidth, modelHeight);

	//Setup inference
	std::vector<std::vector<void*>> batchInputs(1);
	batchInputs[0].push_back((void*) preprocessedImageDevice);
	LocatedExecutionMemory predictionInputs(LocatedExecutionMemory::DEVICE,
			batchInputs);

	//Execute inference
	LocatedExecutionMemory predictionOutputs = predict(predictionInputs, true);

	//We only are classifying a single batch and image
	return (float*) predictionOutputs[0][0];
}

}

