#define PI 3.141592653589793f

typedef float2 cfloat;

__attribute__((always_inline))
cfloat cnum(float r, float i)
{
	return (cfloat)(r, i);
}

__attribute__((always_inline))
cfloat cmul(cfloat z1, cfloat z2)
{
	return (cfloat)(
		z1.x*z2.x - z1.y*z2.y,
		z1.x*z2.y + z1.y*z2.x
	);
}

__attribute__((always_inline))
cfloat cexp(cfloat z)
{
	float expx = exp(z.x);
	return (cfloat)(expx * cos(z.y), expx * sin(z.y));
}

__attribute__((always_inline))
cfloat csin(cfloat z)
{
	float x = z.x;
	float y = z.y;
	return (cfloat)(sin(x) * cosh(y), cos(x) * sinh(y));
}

__attribute__((always_inline))
cfloat ccos(cfloat z)
{
	float x = z.x;
	float y = z.y;
	return (cfloat)(cos(x) * cosh(y), -sin(x) * sinh(y));
}

__attribute__((always_inline))
float limit(float val)
{
	return clamp(val, 0.f, 255.f);
}

__attribute__((always_inline))
float amplitude(cfloat val)
{
	return limit(length(val));
}

__attribute__((always_inline))
float phase(cfloat val)
{
	return limit(255.f * (atan(val.y / val.x) + PI/2) / PI);
}

__attribute__((always_inline))
cfloat normalize_phase(cfloat val, float lambda, float z)
{
	cfloat f = cexp(cmul(cnum(2 * PI * z / lambda, 0), cnum(0, 1)));
	cfloat H = cexp(cmul(atan(f.y / f.x), cnum(0, -1)));
	return cmul(val, H);
}

__kernel void a_f32(
	__global cfloat* src, int src_step, int src_offset, int src_h, int src_w,
	__global float* dst, int dst_step, int dst_offset, int dst_h, int dst_w,
	float lambda, float z
)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x >= dst_w || y >= dst_h) return;
	
	dst[y*dst_w + x] = amplitude(src[y*src_w + x]);
}

__kernel void p_f32(
	__global cfloat* src, int src_step, int src_offset, int src_h, int src_w,
	__global float* dst, int dst_step, int dst_offset, int dst_h, int dst_w,
	float lambda, float z
)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x >= dst_w || y >= dst_h) return;
	
	dst[y*dst_w + x] = phase(normalize_phase(src[y*src_w + x], lambda, z));
}

__kernel void amin_8u(
	__global cfloat* src, int src_step, int src_offset, int src_h, int src_w,
	__global uchar* dst, int dst_step, int dst_offset, int dst_h, int dst_w,
	float lambda, float z
)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x >= dst_w || y >= dst_h) return;
	
	uchar a = amplitude(src[y*src_w + x]);
	dst[y*dst_w + x] = min(dst[y*dst_w + x], a);
}

__kernel void pmin_8u(
	__global cfloat* src, int src_step, int src_offset, int src_h, int src_w,
	__global uchar* dst, int dst_step, int dst_offset, int dst_h, int dst_w,
	float lambda, float z
)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x >= dst_w || y >= dst_h) return;
	
	uchar p = phase(normalize_phase(src[y*src_w + x], lambda, z));
	dst[y*dst_w + x] = min(dst[y*dst_w + x], p);
}

__kernel void a_amin_8u(
	__global cfloat* src, int src_step, int src_offset, int src_h, int src_w,
	__global uchar* dst, int dst_step, int dst_offset, int dst_h, int dst_w,
	__global uchar* dst_min,
	float lambda, float z
)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x >= dst_w || y >= dst_h) return;
	
	cfloat val = src[y*src_w + x];
	uchar a = amplitude(val);
	dst[y*dst_w + x] = a;
	dst_min[y*dst_w + x] = min(dst_min[y*dst_w + x], a);
}

__kernel void a_pmin_8u(
	__global cfloat* src, int src_step, int src_offset, int src_h, int src_w,
	__global uchar* dst, int dst_step, int dst_offset, int dst_h, int dst_w,
	__global uchar* dst_min,
	float lambda, float z
)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x >= dst_w || y >= dst_h) return;
	
	cfloat val = src[y*src_w + x];
	uchar a = amplitude(val);
	uchar p = phase(normalize_phase(val, lambda, z));
	dst[y*dst_w + x] = a;
	dst_min[y*dst_w + x] = min(dst_min[y*dst_w + x], p);
}

__kernel void angularspectrum(
	__global cfloat* prop, int step, int offset, int h, int w,
	float2 size,
	float lambda
)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x >= w || y >= h) return;
	
	float u = (float)(x < w/2 ? x : -(w - x)) / size.x;
	float v = (float)(y < h/2 ? y : -(h - y)) / size.y;
	
	// (2*pi*i)/lambda * sqrt(1 - (lambda*u)^2 - (lambda*v)^2)
	float root = sqrt(1 - lambda*lambda * (u*u + v*v));
	prop[y*w + x] = cmul(cnum(2 * PI * root / lambda, 0), cnum(0, 1));
}

__kernel void propagate(
	__global cfloat* src,
	__global cfloat* prop,
	__global cfloat* dst,
	float z
)
{
	// x * e^(z * prop)
	int i = get_global_id(0);
	dst[i] = cmul(src[i], cexp(cmul(prop[i], cnum(z, 0))));
}

__kernel void supergaussian(
	__global cfloat* H, int step, int offset, int h, int w,
	float2 size,
	int type,
	float2 sigma,
	int n
)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x >= w || y >= h) return;
	
	float u = (float)(x < w/2 ? x : -(w - x)) / size.x;
	float v = (float)(y < h/2 ? y : -(h - y)) / size.y;
	
	float filter = exp(-1.0/2.0 * pow(pow(u / sigma.x, 2) + pow(v / sigma.y, 2), n));
	H[y*w + x] = cnum(type == 0 ? filter : 1-filter, 0.0);
}

__kernel void stdfilt_3x3(
	__global float* src,
	__global float* dst, int step, int offset, int h, int w
)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x >= w || y >= h) return;
	
	float sum = 0.0;
	float sqsum = 0.0;
	int n = 0;
	for (int xx = max(x-1, 0); xx <= min(x+1, w-1); xx++) {
		for (int yy = max(y-1, 0); yy <= min(y+1, h-1); yy++) {
			float val = src[yy*w + xx];
			sum += val;
			sqsum += val*val;
			n++;
		}
	}
	float mean = sum / n;
	float var = fabs(sqsum / n - mean*mean);
	dst[y*w + x] = sqrt(var);
}

__kernel void gradient(
	__global float* src,
	__global float* fx,
	__global float* fy,
	int step, int offset, int h, int w
)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x >= w || y >= h) return;
	
	float valx = 0.0;
	float valy = 0.0;
	if (x > 0)
		valx -= 0.5*src[y*w + x-1];
	if (x < w-1)
		valx += 0.5*src[y*w + x+1];
	if (y > 0)
		valy -= 0.5*src[(y-1)*w + x];
	if (y < h-1)
		valy += 0.5*src[(y+1)*w + x];
	
	fx[y*w + x] = valx;
	fy[y*w + x] = valy;
}
