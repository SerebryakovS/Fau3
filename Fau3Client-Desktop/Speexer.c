#include <speex/speex.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define SpeexInFrameSize    160*2
#define SpeexOutFrameSize      38

void EncodePCMFileIntoSpeex(char *);
void DecodeSpeexFileIntoPCM(char *);

int main(int Argc, char **Argv){
    if (strcmp(Argv[1],"enc")){
        EncodePCMFileIntoSpeex(Argv[2]);
    };
    if (strcmp(Argv[1],"dec")){
        DecodeSpeexFileIntoPCM(Argv[2]);
    };
};

void EncodePCMFileIntoSpeex(char *OutFilename){
    SpeexBits SpeexDecoderBits;
    void *SpeexDecoderState;
    speex_bits_init(&SpeexDecoderBits);
    SpeexDecoderState = speex_decoder_init(&speex_nb_mode);
    int Enhancement = 1;
    speex_decoder_ctl(SpeexDecoderState, SPEEX_SET_ENH, &Enhancement);
    FILE *OutFile = fopen(OutFilename, "w");
    while (1){
        char EncodedFrame[SpeexOutFrameSize];
        char OutBuffer[SpeexInFrameSize];
        int ReadCount = fread(EncodedFrame, sizeof(char), SpeexOutFrameSize, stdin);
        if (feof(stdin)){
            break;
        };
        speex_bits_read_from(&SpeexDecoderBits, (char *)EncodedFrame, SpeexOutFrameSize);
        signed short  DecodedFrame[SpeexInFrameSize/sizeof(short)];
        speex_decode_int(SpeexDecoderState, &SpeexDecoderBits, (spx_int16_t *)DecodedFrame);
        memcpy(OutBuffer, DecodedFrame, SpeexInFrameSize);
        fwrite(OutBuffer, sizeof(char), SpeexInFrameSize, OutFile);
    };
    speex_bits_destroy(&SpeexDecoderBits);
    speex_decoder_destroy(SpeexDecoderState);
};

void DecodeSpeexFileIntoPCM(char *OutFilename){
    SpeexBits SpeexEncoderBits;
    void *SpeexEncoderState;
    speex_bits_init(&SpeexEncoderBits);
    SpeexEncoderState = speex_encoder_init(&speex_nb_mode);
    int AudioSampleRate = 8000;
    speex_encoder_ctl(SpeexEncoderState ,SPEEX_SET_SAMPLING_RATE,&AudioSampleRate);
    int SpeexQualityLevel = 8;
    speex_encoder_ctl(SpeexEncoderState, SPEEX_SET_QUALITY, &SpeexQualityLevel);
    FILE *OutFile = fopen(OutFilename, "w");
    while (1){
        signed short FrameToEncode[SpeexInFrameSize/sizeof(short)];
        char InBuffer[SpeexInFrameSize];
        int ReadCount = fread(InBuffer, sizeof(char), SpeexInFrameSize, stdin);
        if (feof(stdin)){
            break;
        };
        memcpy(FrameToEncode, InBuffer, SpeexInFrameSize);
        speex_bits_reset(&SpeexEncoderBits);
        speex_encode_int(SpeexEncoderState, FrameToEncode, &SpeexEncoderBits);
        char EncodedFrame[SpeexOutFrameSize];
        speex_bits_write(&SpeexEncoderBits, EncodedFrame, SpeexOutFrameSize);
        fwrite(EncodedFrame, sizeof(char), SpeexOutFrameSize, OutFile);
    };
    speex_bits_destroy(&SpeexEncoderBits);
    speex_encoder_destroy(SpeexEncoderState);
};


