StructuredBuffer<float> input;
RWStructuredBuffer<float> output;

[shader("compute")]
[numthreads(64, 1, 1)]
void doubleArray(uint3 tid: SV_DispatchThreadID) {
    output[tid.x] = input[tid.x] * 2.0;
}
