## 简介

amrnb实现了AMR-NB语音格式的编解码，其遵循Apache License。

AMR-NB 支持八种速率模式,分别为:模式0(4.75kbit/s)、模式1(5.15kbit/s)、模式2 (5.90kbit/s)、模式3(6.70kbit/s)、模式 4(7.40kbit/s)、模式 5(7.95kbit/s)、模式 6(10.2kbit/s)、模式 7(12.2kbit/s),其以更加智能的方式解决信源和信道编码的速率分配问题,根据无线信道和传输状况来自适应地选择一种编码模式进行传输,使得无线资源的配置与利用更加灵活有效。

## 相关接口

具体接口使用请参见gsmamr_dec.h中的说明。

## 如何使用

### amrnb解码示例

```c
#include <malloc.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "gsmamr_dec.h"
#include <sndfile.h>

// Constants for AMR-NB
enum {
    kInputBufferSize = 64,
    kSamplesPerFrame = 160,
    kBitsPerSample = 16,
    kOutputBufferSize = kSamplesPerFrame * kBitsPerSample/8,
    kSampleRate = 8000,
    kChannels = 1,
    kFileHeaderSize = 6
};
const uint32_t kFrameSizes[] = {12, 13, 15, 17, 19, 20, 26, 31};

int main(int argc, char *argv[]) {

    if(argc != 3) {
        fprintf(stderr, "Usage %s <input file> <output file>\n", argv[0]);
        return 1;
    }

    // Open the input file
    FILE* fpInput = fopen(argv[1], "rb");
    if (!fpInput) {
        fprintf(stderr, "Could not open %s\n", argv[1]);
        return 1;
    }

    // Validate the input AMR file
    char header[kFileHeaderSize];
    int bytesRead = fread(header, 1, kFileHeaderSize, fpInput);
    if (bytesRead != kFileHeaderSize || memcmp(header, "#!AMR\n", kFileHeaderSize)) {
        fprintf(stderr, "Invalid AMR-NB file\n");
        fclose(fpInput);
        return 1;
    }

    // Open the output file
    SF_INFO sfInfo;
    memset(&sfInfo, 0, sizeof(SF_INFO));
    sfInfo.channels = kChannels;
    sfInfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    sfInfo.samplerate = kSampleRate;
    SNDFILE *handle = sf_open(argv[2], SFM_WRITE, &sfInfo);
    if(!handle){
        fprintf(stderr, "Could not create %s\n", argv[2]);
        fclose(fpInput);
        return 1;
    }

    // Create AMR-NB decoder instance
    void* amrHandle;
    int err = GSMInitDecode(&amrHandle, (Word8*)"AMRNBDecoder");
    if(err != 0){
        fprintf(stderr, "Error creating AMR-NB decoder instance\n");
        fclose(fpInput);
        sf_close(handle);
        return 1;
    }

    //Allocate input buffer
    void *inputBuf = malloc(kInputBufferSize);
    assert(inputBuf != NULL);

    //Allocate output buffer
    void *outputBuf = malloc(kOutputBufferSize);
    assert(outputBuf != NULL);
    // Decode loop
    uint32_t retVal = 0;
    while (1) {
        // Read mode
        uint8_t mode;
        bytesRead = fread(&mode, 1, 1, fpInput);
        if (bytesRead != 1) break;

        // Find frame type
        Frame_Type_3GPP frameType = (Frame_Type_3GPP)((mode >> 3) & 0x0f);
        if (frameType >= AMR_SID){
            fprintf(stderr, "Frame type %d not supported\n",frameType);
            retVal = 1;
            break;
        }
        // Find frame type
        int32_t frameSize = kFrameSizes[frameType];
        bytesRead = fread(inputBuf, 1, frameSize, fpInput);
        if (bytesRead != frameSize) break;

        //Decode frame
        int32_t decodeStatus;
        decodeStatus = AMRDecode(amrHandle, frameType, (uint8_t*)inputBuf,
                                 (int16_t*)outputBuf, MIME_IETF);
        if(decodeStatus == -1) {
            fprintf(stderr, "Decoder encountered error\n");
            retVal = 1;
            break;
        }
        //Write output to wav
        sf_writef_short(handle, (int16_t*)outputBuf, kSamplesPerFrame);
    }
    // Close input and output file
    fclose(fpInput);
    sf_close(handle);

    //Free allocated memory
    free(inputBuf);
    free(outputBuf);

    // Close decoder instance
    GSMDecodeFrameExit(&amrHandle);

    return retVal;
}
```

