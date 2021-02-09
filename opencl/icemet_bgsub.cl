#define STACK_LEN_MAX 25

__kernel void push(
	__global uchar* img,
	__global uchar* stack,
	int size,
	int idx
)
{
	const int gid = get_global_id(0);
	stack[idx*size + gid] = img[gid];
}

__attribute__((always_inline))
void swap(uchar* a, uchar* b)
{
	uchar tmp = *a;
	*a = *b;
	*b = tmp;
}

__attribute__((always_inline))
uchar median(uchar* A, int n)
{
	int medi = n / 2;
	int low = 0;
	int high = n-1;
	while (high > low+1) {
		int middle = (low + high) / 2;
		swap(&A[middle], &A[low+1]);
		
		if (A[low] > A[high])
			swap(&A[low], &A[high]);
		if (A[low+1] > A[high])
			swap(&A[low+1], &A[high]);
		if (A[low] > A[low+1])
			swap(&A[low], &A[low+1]);
		
		uchar val = A[low+1];
		int ll = low+1;
		int hh = high;
		while (true) {
			do ll++; while(A[ll] < val);
			do hh--; while(A[hh] > val);
			if (hh < ll) break;
			swap(&A[ll], &A[hh]);
		}
		
		A[low+1] = A[hh];
		A[hh] = val;
		if (hh >= medi) high = hh-1;
		if (hh <= medi) low = ll;
	}
	
	if (high == low+1 && A[high] < A[low])
		swap(&A[low], &A[high]);
	return A[medi];
}

__kernel void meddiv(
	__global uchar* stack,
	__global float* means,
	__global uchar* dst,
	int size,
	int len,
	int idx
)
{
	int gid = get_global_id(0);
	
	// Fill median array
	uchar A[STACK_LEN_MAX];
	for (int i = 0; i < len; i++)
		A[i] = (float)stack[i*size + gid] / means[i] * means[idx];
	
	// Division
	dst[gid] = (float)A[idx] / (float)median(A, len) * means[idx];
}
