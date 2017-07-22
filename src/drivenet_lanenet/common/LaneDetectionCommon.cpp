/////////////////////////////////////////////////////////////////////////////////////////
// This code contains NVIDIA Confidential Information and is disclosed
// under the Mutual Non-Disclosure Agreement.
//
// Notice
// ALL NVIDIA DESIGN SPECIFICATIONS AND CODE ("MATERIALS") ARE PROVIDED "AS IS" NVIDIA MAKES
// NO REPRESENTATIONS, WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ANY IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
//
// NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. No third party distribution is allowed unless
// expressly authorized by NVIDIA.  Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2017 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//
/////////////////////////////////////////////////////////////////////////////////////////

#include "LaneDetectionCommon.hpp"

#include <iostream>

namespace laneDetection
{
    //------------------------------------------------------------------------------
    void drawLaneDetectionROI(float32_t drawScaleX, float32_t drawScaleY, const dwRect &roi,\
                              dwRenderBufferHandle_t renderBuffer, dwRendererHandle_t renderer)
    {
        float32_t x_start = static_cast<float32_t>(roi.x)*drawScaleX;
        float32_t x_end   = static_cast<float32_t>(roi.x + roi.width)*drawScaleX;
        float32_t y_start = static_cast<float32_t>(roi.y)*drawScaleY;
        float32_t y_end   = static_cast<float32_t>(roi.y + roi.height)*drawScaleY;
        float32_t *coords     = nullptr;
        uint32_t maxVertices  = 0;
        uint32_t vertexStride = 0;
        dwRenderBuffer_map(&coords, &maxVertices, &vertexStride, renderBuffer);
        coords[0]  = x_start;
        coords[1]  = y_start;
        coords    += vertexStride;
        coords[0]  = x_start;
        coords[1]  = y_end;
        coords    += vertexStride;
        coords[0]  = x_start;
        coords[1]  = y_end;
        coords    += vertexStride;
        coords[0]  = x_end;
        coords[1]  = y_end;
        coords    += vertexStride;
        coords[0]  = x_end;
        coords[1]  = y_end;
        coords    += vertexStride;
        coords[0] = x_end;
        coords[1] = y_start;
        coords    += vertexStride;
        coords[0] = x_end;
        coords[1] = y_start;
        coords    += vertexStride;
        coords[0] = x_start;
        coords[1] = y_start;
        dwRenderBuffer_unmap(8, renderBuffer);
        dwRenderer_setColor(DW_RENDERER_COLOR_YELLOW, renderer);
        dwRenderer_setLineWidth(2, renderer);
        dwRenderer_renderBuffer(renderBuffer, renderer);
    }

    //------------------------------------------------------------------------------
    void drawLaneMarkings(const dwLaneDetection &lanes, float32_t drawScaleX, float32_t drawScaleY,
                          dwLaneDetectorHandle_t laneDetector, dwRenderBufferHandle_t renderBuffer,
                          dwRendererHandle_t renderer)
    {
        dwRect roi{};
        dwLaneDetector_getDetectionROI(&roi,laneDetector);

        drawLaneDetectionROI(drawScaleX, drawScaleY, roi, renderBuffer, renderer);

        for (uint32_t i = 0; i < lanes.numLaneMarkings; ++i) {

            const dwLaneMarking& laneMarking = lanes.laneMarkings[i];

            dwLanePositionType category = laneMarking.positionType;

            if(category==DW_LANEMARK_POSITION_ADJACENT_LEFT)
                dwRenderer_setColor(DW_RENDERER_COLOR_YELLOW, renderer);
            else if(category==DW_LANEMARK_POSITION_EGO_LEFT)
                dwRenderer_setColor(DW_RENDERER_COLOR_RED, renderer);
            else if(category==DW_LANEMARK_POSITION_EGO_RIGHT)
                dwRenderer_setColor(DW_RENDERER_COLOR_GREEN, renderer);
            else if(category==DW_LANEMARK_POSITION_ADJACENT_RIGHT)
                dwRenderer_setColor(DW_RENDERER_COLOR_BLUE, renderer);

            dwRenderer_setLineWidth(6.0f, renderer);

            float32_t* coords = nullptr;
            uint32_t maxVertices = 0;
            uint32_t vertexStride = 0;
            dwRenderBuffer_map(&coords, &maxVertices, &vertexStride, renderBuffer);

            uint32_t n_verts = 0;
            dwVector2f previousP{};
            bool firstPoint = true;

            for (uint32_t j = 0; j < laneMarking.numPoints; ++j) {

                dwVector2f center;
                center.x = laneMarking.imagePoints[j].x*drawScaleX;
                center.y = laneMarking.imagePoints[j].y*drawScaleY;

                if (firstPoint) { // Special case for the first point
                    previousP = center;
                    firstPoint = false;
                }
                else {
                    n_verts += 2;
                    if(n_verts > maxVertices)
                        break;

                    coords[0] = static_cast<float32_t>(previousP.x);
                    coords[1] = static_cast<float32_t>(previousP.y);
                    coords += vertexStride;

                    coords[0] = static_cast<float32_t>(center.x);
                    coords[1] = static_cast<float32_t>(center.y);
                    coords += vertexStride;

                    previousP = center;
                }
            }

            dwRenderBuffer_unmap(n_verts, renderBuffer);
            dwRenderer_renderBuffer(renderBuffer, renderer);
        }
    }

    //------------------------------------------------------------------------------
    void createVideoReplay(dwSensorHandle_t &salSensor,
                           uint32_t &cameraWidth,
                           uint32_t &cameraHeight,
                           uint32_t &cameraSiblings,
                           float32_t &cameraFrameRate,
                           dwImageType &imageType,
                           dwSALHandle_t sal,
                           const std::string &videoFName,
                           const std::string &offscreen)
    {
        std::string arguments = "offscreen=" + offscreen + ",video=" + videoFName;

        dwSensorParams params;
        params.parameters = arguments.c_str();
        params.protocol   = "camera.virtual";
        dwSAL_createSensor(&salSensor, params, sal);

        dwImageProperties cameraImageProperties;
        dwSensorCamera_getImageProperties(&cameraImageProperties,
                                          DW_CAMERA_PROCESSED_IMAGE,
                                          salSensor);
        dwCameraProperties cameraProperties;
        dwSensorCamera_getSensorProperties(&cameraProperties, salSensor);

        cameraWidth = cameraImageProperties.width;
        cameraHeight = cameraImageProperties.height;
        imageType = cameraImageProperties.type;
        cameraFrameRate = cameraProperties.framerate;
        cameraSiblings = cameraProperties.siblings;
    }

    //------------------------------------------------------------------------------
    void setupRenderer(dwRendererHandle_t &renderer, const dwRect &screenRectangle, dwContextHandle_t dwSdk)
    {
        dwRenderer_initialize(&renderer, dwSdk);

        float32_t rasterTransform[9];
        rasterTransform[0] = 1.0f;
        rasterTransform[3] = 0.0f;
        rasterTransform[6] = 0.0f;
        rasterTransform[1] = 0.0f;
        rasterTransform[4] = 1.0f;
        rasterTransform[7] = 0.0f;
        rasterTransform[2] = 0.0f;
        rasterTransform[5] = 0.0f;
        rasterTransform[8] = 1.0f;

        dwRenderer_set2DTransform(rasterTransform, renderer);
        float32_t boxColor[4] = {0.0f,1.0f,0.0f,1.0f};
        dwRenderer_setColor(boxColor, renderer);
        dwRenderer_setLineWidth(2.0f, renderer);
        dwRenderer_setRect(screenRectangle, renderer);
    }

    //------------------------------------------------------------------------------
    void setupLineBuffer(dwRenderBufferHandle_t &lineBuffer, unsigned int maxLines, int32_t windowWidth,
                         int32_t windowHeight, dwContextHandle_t dwSdk)
    {
        dwRenderBufferVertexLayout layout;
        layout.posFormat   = DW_RENDER_FORMAT_R32G32_FLOAT;
        layout.posSemantic = DW_RENDER_SEMANTIC_POS_XY;
        layout.colFormat   = DW_RENDER_FORMAT_NULL;
        layout.colSemantic = DW_RENDER_SEMANTIC_COL_NULL;
        layout.texFormat   = DW_RENDER_FORMAT_NULL;
        layout.texSemantic = DW_RENDER_SEMANTIC_TEX_NULL;
        dwRenderBuffer_initialize(&lineBuffer, layout, DW_RENDER_PRIM_LINELIST, maxLines, dwSdk);
        dwRenderBuffer_set2DCoordNormalizationFactors((float32_t)windowWidth,
                                                      (float32_t)windowHeight, lineBuffer);
    }

    //------------------------------------------------------------------------------
    void renderCameraTexture(dwImageStreamerHandle_t streamer, dwRendererHandle_t renderer)
    {
        dwImageGL *frameGL = nullptr;

        if (dwImageStreamer_receiveGL(&frameGL, 30000, streamer) != DW_SUCCESS) {
            std::cerr << "did not received GL frame within 30ms" << std::endl;
        } else {
            // render received texture
            dwRenderer_renderTexture(frameGL->tex, frameGL->target, renderer);
            dwImageStreamer_returnReceivedGL(frameGL, streamer);
        }
    }

    //------------------------------------------------------------------------------
    void runDetector(dwImageCUDA* frame, float32_t drawScaleLinesX, float32_t drawScaleLinesY,
                     dwLaneDetectorHandle_t laneDetector, dwRenderBufferHandle_t renderBuffer,
                     dwRendererHandle_t renderer)
    {
        // Run inference if the model is valid
        if (laneDetector)
        {
            dwLaneDetection lanes{};
            dwStatus res = dwLaneDetector_processDeviceAsync(frame, laneDetector);
            res = res == DW_SUCCESS ? dwLaneDetector_interpretHost(laneDetector) : res;
            if (res != DW_SUCCESS)
            {
                std::cerr << "runDetector failed with: " << dwGetStatusName(res) << std::endl;
            }

            dwLaneDetector_getLaneDetections(&lanes, laneDetector);
            drawLaneMarkings(lanes, drawScaleLinesX, drawScaleLinesY, laneDetector, renderBuffer, renderer);
        }
    }

#ifdef VIBRANTE
    dwStatus runSingleCameraPipelineNvmedia(dwImageCUDA &frameCUDArgba,
                                            dwImageNvMedia &frameNVMrgba,
                                            float32_t drawScaleLinesX,
                                            float32_t drawScaleLinesY,
                                            dwLaneDetectorHandle_t laneDetector,
                                            dwSensorHandle_t cameraSensor,
                                            dwRenderBufferHandle_t renderBuffer,
                                            dwRendererHandle_t renderer,
                                            dwImageStreamerHandle_t nvm2cudaYUV,
                                            dwImageStreamerHandle_t nvm2glRGBA,
                                            dwImageFormatConverterHandle_t yuv2rgbaNvm,
                                            dwImageFormatConverterHandle_t yuv2rgbaCuda)
    {
        dwStatus result             = DW_FAILURE;
        dwCameraFrameHandle_t frame = nullptr;
        dwImageNvMedia *frameNVMyuv = nullptr;
        dwImageCUDA *imgCUDA        = nullptr;
        dwImageNvMedia *retimg      = nullptr;

        result = dwSensorCamera_readFrame(&frame, 0, 1000000, cameraSensor);
        if (result == DW_END_OF_STREAM)
            return result;

        if (result != DW_SUCCESS && result != DW_END_OF_STREAM) {
            std::cerr << "readFrameNvMedia: " << dwGetStatusName(result) << std::endl;
            return result;
        }

        result = dwSensorCamera_getImageNvMedia(&frameNVMyuv, DW_CAMERA_PROCESSED_IMAGE, frame);
        if (result != DW_SUCCESS && result != DW_END_OF_STREAM) {
            std::cerr << "readFrameNvMedia: " << dwGetStatusName(result) << std::endl;
            return result;
        }

        // (nvmedia) YUV->RGBA
        {
            result = dwImageFormatConverter_copyConvertNvMedia(&frameNVMrgba, frameNVMyuv, yuv2rgbaNvm);
            if (result != DW_SUCCESS) {
                std::cerr << "cannot convert YUV->RGBA (nvmedia)" << dwGetStatusName(result) << std::endl;
                return result;
            }
        }

        // (nvmedia) NVMEDIA->GL (rgba) - for rendering
        {
            result = dwImageStreamer_postNvMedia(&frameNVMrgba, nvm2glRGBA);
            if (result != DW_SUCCESS) {
                std::cerr << "cannot post CUDA RGBA image" << dwGetStatusName(result) << std::endl;
                return result;
            }
            renderCameraTexture(nvm2glRGBA, renderer);
            dwImageStreamer_waitPostedNvMedia(&retimg, 60000, nvm2glRGBA);
        }


        // (nvmedia) NVMEDIA->CUDA (rgba) - for processing
        // since DNN expects pitch linear cuda memory we cannot just post frameNVMrgba through the streamer
        // cause the outcome of the streamer would have block layout, but we need pitch
        // hence we perform one more extra YUV2RGBA conversion using CUDA
        {
            result = dwImageStreamer_postNvMedia(frameNVMyuv, nvm2cudaYUV);
            if (result != DW_SUCCESS) {
                std::cerr << "cannot post NvMedia frame " << dwGetStatusName(result) << std::endl;
                return result;
            }

            result = dwImageStreamer_receiveCUDA(&imgCUDA, 60000, nvm2cudaYUV);
            if (result != DW_SUCCESS || imgCUDA == 0) {
                std::cerr << "did not received CUDA frame within 60ms" << std::endl;
                return result;
            }

            // copy convert into RGBA
            dwImageFormatConverter_copyConvertCUDA(&frameCUDArgba, imgCUDA, yuv2rgbaCuda, 0);

            runDetector(&frameCUDArgba, drawScaleLinesX, drawScaleLinesY, laneDetector,
                        renderBuffer, renderer);

            dwImageStreamer_returnReceivedCUDA(imgCUDA, nvm2cudaYUV);
            dwImageStreamer_waitPostedNvMedia(&retimg, 60000, nvm2cudaYUV);
        }

        // no use for the camera frame anymore, cause we work with a copy now
        dwSensorCamera_returnFrame(&frame);


        return DW_SUCCESS;
    }
#else
    //------------------------------------------------------------------------------
    dwStatus runSingleCameraPipelineCuda(dwImageCUDA &frameCUDArgba,
                                         float32_t drawScaleLinesX,
                                         float32_t drawScaleLinesY,
                                         dwLaneDetectorHandle_t laneDetector,
                                         dwSensorHandle_t cameraSensor,
                                         dwRenderBufferHandle_t renderBuffer,
                                         dwRendererHandle_t renderer,
                                         dwImageStreamerHandle_t cuda2gl,
                                         dwImageFormatConverterHandle_t yuv2rgbaCUDA)
    {
        dwStatus result             = DW_FAILURE;
        dwImageCUDA *frameCUDAyuv   = nullptr;
        dwCameraFrameHandle_t frame = nullptr;
        dwImageCUDA *retimg         = nullptr;

        result = dwSensorCamera_readFrame(&frame, 0, 50000, cameraSensor);
        if (result == DW_END_OF_STREAM)
            return result;

        if (result != DW_SUCCESS) {
            std::cout << "readFrameCUDA: " << dwGetStatusName(result) << std::endl;
            return result;
        }

        result = dwSensorCamera_getImageCUDA(&frameCUDAyuv, DW_CAMERA_PROCESSED_IMAGE, frame);
        if (result != DW_SUCCESS) {
            std::cout << "getImageCUDA: " << dwGetStatusName(result) << std::endl;
            return result;
        }

        // YUV->RGBA (cuda)
        dwImageFormatConverter_copyConvertCUDA(&frameCUDArgba, frameCUDAyuv, yuv2rgbaCUDA, 0);

        // we can return the frame already now, we are working with a copy from now on
        dwSensorCamera_returnFrame(&frame);

        // (nvmedia) CUDA->GL (rgba) - for rendering
        {
            result = dwImageStreamer_postCUDA(&frameCUDArgba, cuda2gl);
            if (result != DW_SUCCESS) {
                std::cerr << "cannot post CUDA RGBA image" << dwGetStatusName(result) << std::endl;
                return result;
            }
            renderCameraTexture(cuda2gl, renderer);
            dwImageStreamer_waitPostedCUDA(&retimg, 60000, cuda2gl);
        }

        runDetector(&frameCUDArgba, drawScaleLinesX, drawScaleLinesY, laneDetector, renderBuffer, renderer);

        return DW_SUCCESS;
    }
#endif
    bool initLaneNet(dwLaneDetectorHandle_t *gLaneDetector, dwImageProperties *gCameraImageProperties, 
                        cudaStream_t gCudaStream, dwContextHandle_t dwSdk, ProgramArguments &gArguments)
    {
        dwStatus res = DW_FAILURE;
        res = dwLaneDetector_initializeLaneNet(gLaneDetector, gCameraImageProperties->width, gCameraImageProperties->height, gCudaStream, dwSdk);
        if (res != DW_SUCCESS)
        {
            std::cerr << "Cannot initialize LaneNetCST: " << dwGetStatusName(res) << std::endl;
            return false;
        }
        float32_t gThreshold = 0.3f;
        std::string inputThreshold = gArguments.get("threshold-lanenet");
        if(inputThreshold != "0.3"){
            try{
                    gThreshold = std::stof(inputThreshold);
            } catch(...){
                std::cerr << "Given threshold can't be parsedCST" << std::endl;
                return false;
            }
        }
        res = dwLaneDetectorLaneNet_setDetectionThreshold(gThreshold, *gLaneDetector);
        if(res != DW_SUCCESS)
        {
            std::cerr << "Cannot set LaneNet thresholdCST: " << dwGetStatusName(res) << std::endl;
            return false;
        }
        //Default to 60 FOV input video setup, FOV is only set when "120" is parsed
        std::string videoFOV = gArguments.get("FOV");
        if (videoFOV.compare("120") == 0){
            res = dwLaneDetectorLaneNet_setVideoFOV(DW_LANEMARK_VIDEO_FOV_120, *gLaneDetector);
            if(res != DW_SUCCESS)
            {
                std::cerr << "Cannot set LaneNet input video FOVCST: " << dwGetStatusName(res) << std::endl;
                return false;
            }
        }
        else if (videoFOV.compare("60") != 0){
            std::cerr << "Given video FOV is not valid" << std::endl;
        }
        //Defalut to 0.25, uncomment this block to cutomize lane temporal smoothing factor
        /*
            uncomment code in ppt No.20
        */
        return true;
    }
    void release(dwLaneDetectorHandle_t *gLaneDetector)
    {
        dwLaneDetector_release(gLaneDetector);
    }
}