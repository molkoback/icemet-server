__kernel void adjust(
	__global uchar* src,
	__global uchar* dst,
	uchar a0, uchar a1,
	uchar b0, uchar b1
)
{
	int i = get_global_id(0);
	float val = clamp(src[i], a0, a1);
	dst[i] = (val-a0) / (a1-a0) * (b1-b0) + b0;
}

__kernel void imghist(__global uchar* src, __global int* dst)
{
	atomic_inc(&dst[src[get_global_id(0)]]);
}
