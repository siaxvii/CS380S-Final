#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const uint8_t* data;
    size_t size;
    size_t pos;
} BufferData;

// 🆕 Struct to hold the decoded audio
typedef struct {
    uint8_t* data;     // Interleaved S16 samples
    int data_size;     // Size in bytes
    int sample_rate;   // Hz
    int channels;      // 2 for stereo
    int nb_samples;    // Total samples per channel
} DecodedAudio;

int sandboxed_decode_audio(const char* file_data, const int file_size, DecodedAudio* out_audio);

#ifdef __cplusplus
}
#endif