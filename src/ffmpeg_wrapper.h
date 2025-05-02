#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const uint8_t* data;
    size_t size;
    size_t pos;
} BufferData;

int sandboxed_decode_audio(const char* file_data, const size_t file_size, uint8_t** out_data, size_t* out_size);

#ifdef __cplusplus
}
#endif