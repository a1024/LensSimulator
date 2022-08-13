//main.c - Lens Designer, currently compiles only on Windows
//Copyright (C) 2022  Ayman Wagih Mohsen, unless source link provided.
//
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include<stdio.h>
#include<stdlib.h>
#include<stdarg.h>
#define	_USE_MATH_DEFINES
#include<math.h>
#include<Windows.h>
#include<sys/stat.h>

#define			SIZEOF(ST_ARR)	(sizeof(ST_ARR)/sizeof(*(ST_ARR)))
#define			G_BUF_SIZE	2048
char			g_buf[G_BUF_SIZE]={0};
//wchar_t		g_wbuf[G_BUF_SIZE]={0};
HINSTANCE		ghInstance;
HWND			ghWnd;
HDC				ghDC, ghMemDC;
HBITMAP			hBitmap;
const unsigned
	_2dCheckColor=0x00E0E0E0,//0x00D0D0D0	0x00EFEFEF
	_3dGridColor=0x00D0D0D0;
HPEN			hPen=0, hPenLens=0;
HBRUSH			hBrush=0, hBrushHollow=0;
int				w=0, h=0, X0, Y0;
int				mx=0, my=0, kp=0;
char			kb[256]={0};
POINT			centerP, mouseP0;
RECT			R;
int				*rgb=0, rgbn=0;
LARGE_INTEGER	li;
long long		freq, nticks;
void			GUIPrint(HDC hDC, int x, int y, const char* format, ...)
{
	va_list args;
	if(format)
	{
		va_start(args, format);
		int linelen=vsprintf_s(g_buf, G_BUF_SIZE, format, args);
		va_end(args);
		if(linelen>0)
			TextOutA(hDC, x, y, g_buf, linelen);
	}
}
int				GUITPrint(HDC hDC, int x, int y, int tab_origin, const char* format, ...)
{
	va_list args;
	if(format)
	{
		va_start(args, format);
		int linelen=vsprintf_s(g_buf, G_BUF_SIZE, format, args);
		va_end(args);
		if(linelen>0)
			return TabbedTextOutA(hDC, x, y, g_buf, linelen, 0, 0, tab_origin);
	}
	return 0;
}
void			messagebox(const char *title, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vsprintf_s(g_buf, G_BUF_SIZE, format, args);
	va_end(args);
	MessageBoxA(ghWnd, g_buf, title, MB_OK);
}

int				mod(int x, int n)
{
	x%=n;
	x+=n&-(x<0);
	return x;
}
void			memfill(void *dst, const void *src, size_t dstbytes, size_t srcbytes)
{
	unsigned copied;
	char *d=(char*)dst;
	const char *s=(const char*)src;
	if(dstbytes<srcbytes)
	{
		memcpy(dst, src, dstbytes);
		return;
	}
	copied=srcbytes;
	memcpy(d, s, copied);
	while(copied<<1<=dstbytes)
	{
		memcpy(d+copied, d, copied);
		copied<<=1;
	}
	if(copied<dstbytes)
		memcpy(d+copied, d, dstbytes-copied);
}

char			error_msg[G_BUF_SIZE]={0};
int				runtime_error(const char *file, int line, const char *format, ...)
{
	va_list args;
	int printed;

	printed=sprintf_s(error_msg, G_BUF_SIZE, "%s(%d) ERROR\n", file, line);
	if(format)
	{
		va_start(args, format);
		printed+=vsprintf_s(error_msg+printed, G_BUF_SIZE-printed, format, args);
		va_end(args);
		printed+=sprintf_s(error_msg+printed, G_BUF_SIZE-printed, "\n");
	}
	messagebox("Error", error_msg);
	//pause();
	return 0;
}
int				valid(const void *p)
{
	size_t val=(size_t)p;

	if(sizeof(size_t)==4)
	{
		switch(val)
		{
		case 0:
		case 0xCCCCCCCC:
		case 0xFEEEFEEE:
		case 0xEEFEEEFE:
		case 0xCDCDCDCD:
		case 0xFDFDFDFD:
		case 0xBAADF00D:
		case 0xBAAD0000:
			return 0;
		}
	}
	else
	{
		if(val==0xCCCCCCCCCCCCCCCC)
			return 0;
		if(val==0xFEEEFEEEFEEEFEEE)
			return 0;
		if(val==0xEEFEEEFEEEFEEEFE)
			return 0;
		if(val==0xCDCDCDCDCDCDCDCD)
			return 0;
		if(val==0xBAADF00DBAADF00D)
			return 0;
		if(val==0xADF00DBAADF00DBA)
			return 0;
	}
	return 1;
}
#define			LOG_ERROR(FORMAT, ...)		runtime_error(__FILE__, __LINE__, FORMAT, ##__VA_ARGS__)
#define			ASSERT(SUCCESS)				((SUCCESS)!=0||runtime_error(__FILE__, __LINE__, #SUCCESS))
#define			ASSERT_P(POINTER)			(valid(POINTER)||runtime_error(__FILE__, __LINE__, #POINTER " == 0"))


//C array declaration
#if 1
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4200)//no default-constructor for struct with zero-length array
#endif
typedef struct ArrayHeaderStruct
{
	size_t count, esize, cap;//cap is in bytes
	void (*destructor)(void*);
	unsigned char data[];
} ArrayHeader, *ArrayHandle;
typedef const ArrayHeader *ArrayConstHandle;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
ArrayHandle		array_construct(const void *src, size_t esize, size_t count, size_t rep, size_t pad, void (*destructor)(void*));
ArrayHandle		array_copy(ArrayHandle *arr);//shallow
void			array_clear(ArrayHandle *arr);//keeps allocation
void			array_free(ArrayHandle *arr);
void			array_fit(ArrayHandle *arr, size_t pad);

void*			array_insert(ArrayHandle *arr, size_t idx, const void *data, size_t count, size_t rep, size_t pad);//cannot be nullptr
void*			array_erase(ArrayHandle *arr, size_t idx, size_t count);
void*			array_replace(ArrayHandle *arr, size_t idx, size_t rem_count, const void *data, size_t ins_count, size_t rep, size_t pad);

size_t			array_size(ArrayHandle const *arr);
void*			array_at(ArrayHandle *arr, size_t idx);
const void*		array_at_const(ArrayConstHandle *arr, int idx);
void*			array_back(ArrayHandle *arr);
const void*		array_back_const(ArrayHandle const *arr);

#define			ARRAY_ALLOC(ELEM_TYPE, ARR, DATA, COUNT, PAD, DESTRUCTOR)	ARR=array_construct(DATA, sizeof(ELEM_TYPE), COUNT, 1, PAD, DESTRUCTOR)
#define			ARRAY_APPEND(ARR, DATA, COUNT, REP, PAD)					array_insert(&(ARR), (ARR)->count, DATA, COUNT, REP, PAD)
#define			ARRAY_DATA(ARR)			(ARR)->data
#define			ARRAY_I(ARR, IDX)		*(int*)array_at(&ARR, IDX)
#define			ARRAY_U(ARR, IDX)		*(unsigned*)array_at(&ARR, IDX)
#define			ARRAY_F(ARR, IDX)		*(double*)array_at(&ARR, IDX)


//null terminated array
#define			ESTR_ALLOC(TYPE, STR, DATA, LEN)	STR=array_construct(DATA, sizeof(TYPE), LEN, 1, 1, 0)
#define			STR_APPEND(STR, SRC, LEN, REP)		array_insert(&(STR), (STR)->count, SRC, LEN, REP, 1)
#define			STR_FIT(STR)						array_fit(&STR, 1)
#define			ESTR_AT(TYPE, STR, IDX)				*(TYPE*)array_at(&(STR), IDX)

#define			STR_ALLOC(STR, LEN)				ESTR_ALLOC(char, STR, 0, LEN)
#define			STR_COPY(STR, DATA, LEN)		ESTR_ALLOC(char, STR, DATA, LEN)
#define			STR_AT(STR, IDX)				ESTR_AT(char, STR, IDX)

#define			WSTR_ALLOC(STR, LEN)			ESTR_ALLOC(wchar_t, STR, 0, LEN)
#define			WSTR_COPY(STR, DATA, LEN)		ESTR_ALLOC(wchar_t, STR, DATA, LEN)
#define			WSTR_AT(STR, IDX)				ESTR_AT(wchar_t, STR, IDX)
#endif

//C array implementation
#if 1
static void		array_realloc(ArrayHandle *arr, size_t count, size_t pad)//CANNOT be nullptr, array must be initialized with array_alloc()
{
	ArrayHandle p2;
	size_t size, newcap;

	ASSERT_P(*arr);
	size=(count+pad)*arr[0]->esize, newcap=arr[0]->esize;
	for(;newcap<size;newcap<<=1);
	if(newcap>arr[0]->cap)
	{
		p2=(ArrayHandle)realloc(*arr, sizeof(ArrayHeader)+newcap);
		ASSERT_P(p2);
		*arr=p2;
		if(arr[0]->cap<newcap)
			memset(arr[0]->data+arr[0]->cap, 0, newcap-arr[0]->cap);
		arr[0]->cap=newcap;
	}
	arr[0]->count=count;
}

//Array API
ArrayHandle		array_construct(const void *src, size_t esize, size_t count, size_t rep, size_t pad, void (*destructor)(void*))
{
	ArrayHandle arr;
	size_t srcsize, dstsize, cap;
	
	srcsize=count*esize;
	dstsize=rep*srcsize;
	for(cap=esize+pad*esize;cap<dstsize;cap<<=1);
	arr=(ArrayHandle)malloc(sizeof(ArrayHeader)+cap);
	ASSERT_P(arr);
	arr->count=count;
	arr->esize=esize;
	arr->cap=cap;
	arr->destructor=destructor;
	if(src)
	{
		ASSERT_P(src);
		memfill(arr->data, src, dstsize, srcsize);
	}
	else
		memset(arr->data, 0, dstsize);
		
	if(cap-dstsize>0)//zero pad
		memset(arr->data+dstsize, 0, cap-dstsize);
	return arr;
}
ArrayHandle		array_copy(ArrayHandle *arr)
{
	ArrayHandle a2;
	size_t bytesize;

	if(!*arr)
		return 0;
	bytesize=sizeof(ArrayHeader)+arr[0]->cap;
	a2=(ArrayHandle)malloc(bytesize);
	ASSERT_P(a2);
	memcpy(a2, *arr, bytesize);
	return a2;
}
void			array_clear(ArrayHandle *arr)//can be nullptr
{
	if(*arr)
	{
		if(arr[0]->destructor)
		{
			for(size_t k=0;k<arr[0]->count;++k)
				arr[0]->destructor(array_at(arr, k));
		}
		arr[0]->count=0;
	}
}
void			array_free(ArrayHandle *arr)//can be nullptr
{
	if(*arr&&arr[0]->destructor)
	{
		for(size_t k=0;k<arr[0]->count;++k)
			arr[0]->destructor(array_at(arr, k));
	}
	free(*arr);
	*arr=0;
}
void			array_fit(ArrayHandle *arr, size_t pad)//can be nullptr
{
	ArrayHandle p2;
	if(!*arr)
		return;
	arr[0]->cap=(arr[0]->count+pad)*arr[0]->esize;
	p2=(ArrayHandle)realloc(*arr, sizeof(ArrayHeader)+arr[0]->cap);
	ASSERT_P(p2);
	*arr=p2;
}

void*			array_insert(ArrayHandle *arr, size_t idx, const void *data, size_t count, size_t rep, size_t pad)//cannot be nullptr
{
	size_t start, srcsize, dstsize, movesize;
	
	ASSERT_P(*arr);
	start=idx*arr[0]->esize;
	srcsize=count*arr[0]->esize;
	dstsize=rep*srcsize;
	movesize=arr[0]->count*arr[0]->esize-start;
	array_realloc(arr, arr[0]->count+rep*count, pad);
	memmove(arr[0]->data+start+dstsize, arr[0]->data+start, movesize);
	if(data)
		memfill(arr[0]->data+start, data, dstsize, srcsize);
	else
		memset(arr[0]->data+start, 0, dstsize);
	return arr[0]->data+start;
}
void*			array_erase(ArrayHandle *arr, size_t idx, size_t count)
{
	size_t k;

	ASSERT_P(*arr);
	if(arr[0]->count<idx+count)
	{
		LOG_ERROR("array_erase() out of bounds: idx=%lld count=%lld size=%lld", (long long)idx, (long long)count, (long long)arr[0]->count);
		if(arr[0]->count<idx)
			return 0;
		count=arr[0]->count-idx;//erase till end of array if just idx+count is OOB
	}
	if(arr[0]->destructor)
	{
		for(k=0;k<count;++k)
			arr[0]->destructor(array_at(arr, idx+k));
	}
	memmove(arr[0]->data+idx*arr[0]->esize, arr[0]->data+(idx+count)*arr[0]->esize, (arr[0]->count-(idx+count))*arr[0]->esize);
	arr[0]->count-=count;
	return arr[0]->data+idx*arr[0]->esize;
}
void*			array_replace(ArrayHandle *arr, size_t idx, size_t rem_count, const void *data, size_t ins_count, size_t rep, size_t pad)
{
	size_t k, c0, c1, start, srcsize, dstsize;

	ASSERT_P(*arr);
	if(arr[0]->count<idx+rem_count)
	{
		LOG_ERROR("array_replace() out of bounds: idx=%lld rem_count=%lld size=%lld ins_count=%lld", (long long)idx, (long long)rem_count, (long long)arr[0]->count, (long long)ins_count);
		if(arr[0]->count<idx)
			return 0;
		rem_count=arr[0]->count-idx;//erase till end of array if just idx+count is OOB
	}
	if(arr[0]->destructor)
	{
		for(k=0;k<rem_count;++k)//destroy removed objects
			arr[0]->destructor(array_at(arr, idx+k));
	}
	start=idx*arr[0]->esize;
	srcsize=ins_count*arr[0]->esize;
	dstsize=rep*srcsize;
	c0=arr[0]->count;//copy original count
	c1=arr[0]->count+rep*ins_count-rem_count;//calculate new count

	if(ins_count!=rem_count||(c1+pad)*arr[0]->esize>arr[0]->cap)//resize array
		array_realloc(arr, c1, pad);

	if(ins_count!=rem_count)//shift objects
		memmove(arr[0]->data+(idx+ins_count)*arr[0]->esize, arr[0]->data+(idx+rem_count)*arr[0]->esize, (c0-(idx+rem_count))*arr[0]->esize);

	if(dstsize)//initialize inserted range
	{
		if(data)
			memfill(arr[0]->data+start, data, dstsize, srcsize);
		else
			memset(arr[0]->data+start, 0, dstsize);
	}
	return arr[0]->data+start;//return start of inserted range
}

size_t			array_size(ArrayHandle const *arr)//can be nullptr
{
	if(!arr[0])
		return 0;
	return arr[0]->count;
}
void*			array_at(ArrayHandle *arr, size_t idx)
{
	if(!arr[0])
		return 0;
	if(idx>=arr[0]->count)
		return 0;
	return arr[0]->data+idx*arr[0]->esize;
}
const void*		array_at_const(ArrayConstHandle *arr, int idx)
{
	if(!arr[0])
		return 0;
	return arr[0]->data+idx*arr[0]->esize;
}
void*			array_back(ArrayHandle *arr)
{
	if(!*arr||!arr[0]->count)
		return 0;
	return arr[0]->data+(arr[0]->count-1)*arr[0]->esize;
}
const void*		array_back_const(ArrayHandle const *arr)
{
	if(!*arr||!arr[0]->count)
		return 0;
	return arr[0]->data+(arr[0]->count-1)*arr[0]->esize;
}
#endif


//optics simulator
typedef struct PointStruct
{
	double x, y;
} Point;
typedef struct PathStruct
{
	int emerged;
	ArrayHandle points;//array of Point struct
	double r_blur;
	Point em_ray[2];//emergence ray
} Path;
void			free_path(void *data)
{
	Path *path=(Path*)data;
	array_free(&path->points);
}
typedef struct PhotonStruct
{
	double lambda;//nm
	int color;
	ArrayHandle paths;//array of Path struct
	Point ground[2], em_centroid;
} Photon;
void			free_photon(void *data)
{
	Photon *ph=(Photon*)data;
	array_free(&ph->paths);
}

#if 0
typedef struct SurfaceRangeStruct
{
	double r_max;
	int type;
} SurfaceRange;
#endif

typedef enum OpticElemStateEnum
{
	STATE_NONE,
	STATE_MIRROR,
	STATE_ACTIVE,
} OpticElemState;
typedef enum SurfaceTypeEnum
{
	SURF_HOLE,
	SURF_TRANSP,
	SURF_MIRROR,
} SurfaceType;
#if 0
typedef struct SurfaceRangeStruct
{
	SurfaceType type;
	double r_max;
} SurfaceRange;
typedef struct BoundaryStruct
{
	double curv_radius;
	int nranges;//can only be 1 or 2
	SurfaceType type[2];
	double r_max[2];
	//SurfaceRange ranges[2];
} Boundary;
#endif
typedef struct SurfaceStruct
{
	double
		pos,	//the x-distance of surface center from origin
		radius;	//looking LtR: -ve means concave, +ve means convex, zero or +-inf means planar

	//the surface is always divided into {inner circle, outer donut}
	SurfaceType type[2];
	double r_max[2];//only the outer donut can have zero radius difference
} Surface;
typedef struct OpticElemStruct
{
	OpticElemState active;
	ArrayHandle name;	//string
	double n;			//refractive index
	Surface surfaces[2];//{left, right}		right radius is negated when parsing input

#if 0
	ArrayHandle name;//string
	OpticElemState active;
	double n;	//refractive index

	//{left, right}
	double
		pos[2],	//x-distance of surface center from origin
		curv_radius[2];//surface radii
	
	//{left.lo, right.lo, left.hi, right.hi}
	SurfaceType type[4];
	double r_max[4];
#endif

#if 0
	OpticElemState active;
	double
		pos,	//x-distance from origin
		th,		//thickness at center
		n;		//refractive index
	double curv_radius[2];//{left, right} surface radii
	//int nranges[2];//{left, right} range count can only be 1 or 2
	SurfaceType type[4];
	double r_max[4];
	//Boundary profiles[2];
	ArrayHandle name;//string
#endif
	
#if 0
	double
		pos,	//x-distance from origin
		rL,		//left surface radius
		th,		//thickness at center
		rR,		//right surface radius
		n;		//refractive index
		rH;		//element vertical radius (aperture)
	ArrayHandle
		name,//string
		profile_left, profile_right;//array of doubles: negative r[1] means mirror between r[0] and r[1]
#endif
} OpticElem;//no automatic destructor
typedef struct AreaIdxStruct
{
	short e_idx;
	char is_right_not_left, is_outer_not_inner;
} AreaIdx;
#if 0
typedef struct BoundaryStruct
{
	int active, obj_id;
	double
		x_pos,
		r_surface,	//surface curvature radius
		r_in,		//donut inner radius
		r_out,		//donut outer radius
		nL, nR;		//refractive index: if zero then mirror
} Boundary;
#endif


//globals
int				current_elem=0;
ArrayHandle		elements=0, ebackup=0,//arrays of OpticElem
				order=0,//array of AreaIdx
				photons=0;//array of Photon
AreaIdx			initial_target={0};
int				nwavelengths=3, nrays=10;
double			tan_tilt=0,//rays tilt
				total_blur=0,
				spread=0;//main loss value

ArrayHandle		ray_spread_mean=0;//array of Point, an extra last element is centroid
double			*history=0;
int				hist_idx=0, history_enabled=1;


#if 1
typedef struct LambdaColorStruct
{
	float lambda;
	union
	{
		int color;
		char comp[4];
	};
} LambdaColor;
LambdaColor wrgb[]=//https://en.wikipedia.org/wiki/File:Linear_visible_spectrum.svg
{		//0xRRGGBB
	{380, 0x020005},
	{385, 0x020006},
	{390, 0x030008},
	{395, 0x04000A},
	{400, 0x06000D},
	{405, 0x080110},
	{410, 0x0C0117},
	{415, 0x11021F},
	{420, 0x17032A},
	{425, 0x1F053A},
	{430, 0x25084B},
	{435, 0x290A5C},
	{440, 0x2B0E6F},
	{445, 0x291380},
	{450, 0x1F237B},
	{455, 0x132E74},
	{460, 0x09376C},
	{465, 0x0A3E66},
	{470, 0x0C4667},
	{475, 0x0E4F6A},
	{480, 0x10596C},
	{485, 0x11636D},
	{490, 0x146E6F},
	{495, 0x177970},
	{500, 0x178672},
	{505, 0x1A9574},
	{510, 0x1DA375},
	{515, 0x1DB273},
	{520, 0x20C070},
	{525, 0x22CB6B},
	{530, 0x21D662},
	{535, 0x23E054},
	{540, 0x36E842},
	{545, 0x50ED28},
	{550, 0x73EB22},
	{555, 0x8FE722},
	{560, 0xA5E221},
	{565, 0xB9DC22},
	{570, 0xCBD621},
	{575, 0xDCCE20},
	{580, 0xECC420},
	{585, 0xF2B735},
	{590, 0xF5AB42},
	{595, 0xF69F49},
	{600, 0xF7944B},
	{605, 0xF98848},
	{610, 0xFA7B42},
	{615, 0xFB6C39},
	{620, 0xFD5B2E},
	{625, 0xFC471F},
	{630, 0xF7300F},
	{635, 0xEA220D},
	{640, 0xD42215},
	{645, 0xBF2318},
	{650, 0xA92309},
	{655, 0x981F07},
	{660, 0x871B06},
	{665, 0x771805},
	{670, 0x671504},
	{675, 0x591303},
	{680, 0x4D1103},
	{685, 0x420E02},
	{690, 0x370C01},
	{695, 0x2E0A01},
	{700, 0x270801},
	{705, 0x210600},
	{710, 0x1E0400},
	{715, 0x1A0300},
	{720, 0x170200},
	{725, 0x130100},
	{730, 0x100100},
	{735, 0x0C0100},
	{740, 0x080100},
	{745, 0x060100},
	{750, 0x040100},
};
int				wrgb_size=SIZEOF(wrgb);
char			mixchar(char v0, char v1, float x)
{
	return v0+(unsigned char)((v1-v0)*x);
}
int				wavelength2rgb(double lambda)
{
	int L=0, R=wrgb_size-1, middle;
	if(lambda<wrgb[L].lambda||lambda>=wrgb[R].lambda)
		return 0;
	while(L<=R)
	{
		middle=(L+R)>>1;
		if(wrgb[middle].lambda<lambda)
			L=middle+1;
		else if(wrgb[middle].lambda>lambda)
			R=middle-1;
		else
		{
			L=R=middle;
			break;
		}
	}
	
	int ret;
	unsigned char *p=(unsigned char*)&ret;
	p[0]=wrgb[L].comp[2];
	p[1]=wrgb[L].comp[1];
	p[2]=wrgb[L].comp[0];
	p[3]=0;
	return ret;

	//LambdaColor *v0=wrgb+L, *v1=wrgb+L+1;
	//float x=((float)lambda-v0->lambda)/(v1->lambda-v0->lambda);	//strange artifacts at same constant locations
	//int ret;
	//unsigned char *p=(unsigned char*)&ret;
	//p[0]=mixchar(v0->comp[2], v1->comp[2], x);
	//p[1]=mixchar(v0->comp[1], v1->comp[1], x);
	//p[2]=mixchar(v0->comp[0], v1->comp[0], x);
	//p[3]=0;
	//return ret;
}
const double coeff_approx[]={1.87816, 1./8000};
double			r_idx2wavelength(double n)
{
	return (1/(n-1)-coeff_approx[0])/coeff_approx[1];
}
double			lambda2n(double n_base, double lambda)
{
	return n_base-1.512+1+1/(lambda/8000+1.87816);
	//return 1+1/(coeff_approx[1]*lambda+coeff_approx[0]);
	//return 1.60+(1.54-1.60)/(800-300)*(lambda-300);
}
#endif

#if 0
#define			PH_COUNT	3
Photon			lightpaths[PH_COUNT]={{470}, {550}, {640}};//a photon of same color can take many paths, a path may or may not emerge on CCD
//const int		n_count=SIZEOF(lightpaths);
double			total_blur=0;
//double		lambda0=0, lambda=590;//565nm
int				twosides=1;//simulate rays above and below x-axis

int				nrays=5;
double			tan_tilt=0;
double			ap=12, ap0=12;//aperture
double			spread=0;
#endif

ArrayHandle		load_text(const char *filename, int pad)
{
	struct stat info={0};
	FILE *f;
	ArrayHandle str;

	int error=stat(filename, &info);
	if(error)
	{
		strerror_s(g_buf, G_BUF_SIZE, errno);
		LOG_ERROR("Cannot open %s\n%s", filename, g_buf);
		return 0;
	}
	fopen_s(&f, filename, "r");
	//f=fopen(filename, "r");
	//f=fopen(filename, "rb");
	//f=fopen(filename, "r, ccs=UTF-8");//gets converted to UTF-16 on Windows

	str=array_construct(0, 1, info.st_size, 1, pad+1, 0);
	str->count=fread(str->data, 1, info.st_size, f);
	fclose(f);
	memset(str->data+str->count, 0, str->cap-str->count);
	return str;
}
#define			DUPLET(A, B)	((unsigned char)(A)|(unsigned char)(B)<<8)
int				skip_ws(ArrayHandle text, int *idx, int *lineno, int *linestart)
{
	for(;*idx<(int)text->count;)
	{
		if(text->data[*idx]=='/')
		{
			if(text->data[*idx+1]=='/')//line comment
			{
				for(;*idx<(int)text->count&&text->data[*idx]&&text->data[*idx]!='\n';++*idx);
				int inc=*idx<(int)text->count;
				*idx+=inc;//skip newline
				*lineno+=inc;
				*linestart=*idx;
			}
			else if(text->data[*idx+1]=='*')//block comment
			{
				for(unsigned short data=DUPLET(text->data[*idx], text->data[*idx+1]);;)
				{
					if(*idx>=(int)text->count)
					{
						//lex_error(p, len, k, "Expected \'*/\'");
						return 0;
					}
					if(data==DUPLET('*', '/'))
					{
						*idx+=2;
						break;
					}
					if((data&0xFF)=='\n')
					{
						++*idx;
						++lineno;
						*linestart=*idx;
					}
					else
						++*idx;
					data>>=8, data|=(unsigned char)text->data[*idx+1]<<8;
				}
			}
			else
				break;
		}
		else if(isspace(text->data[*idx]))
			++*idx;
		else
			break;
	}
	return 1;
}
int				match_kw(ArrayHandle text, int *idx, const char **kw, int nkw)//returns the index of matched keyword
{
}
const char *keywords[]=
{
	"elem", "ap", "n", "pos", "th", "left", "right", "plane", "light", "rays", "path", "inner", "outer",
};
const char *units[]=
{
	"A", "nm", "mm", "cm", "m", "inch", "ft",
};
int				open_system()
{
	ArrayHandle text;

	g_buf[0]=0;
	OPENFILENAMEA ofn=
	{
		sizeof(OPENFILENAMEA),
		ghWnd, ghInstance,
		"Text Files (.txt)\0*.txt\0",//filter
		0, 0,//custom filter
		1,//filter idx
		g_buf, G_BUF_SIZE,//filename
		0, 0,//filetitle
		0,//initial directory (default)
		0,//dialog title
		OFN_CREATEPROMPT|OFN_PATHMUSTEXIST,//flags
		0,//file offset
		0,//extension offset
		"txt",//default extension
		0, 0,//data & hook
		0,//template name
		0,//reserved
		0,//reserved
		0,//flags ex
	};
	int success=GetOpenFileNameA(&ofn);
	if(!success)
		return 0;

	text=load_text(ofn.lpstrFile, 16);
	if(!text)
		return 0;

	for(int idx=0, lineno=0, linestart=0;;)
	{
	}

	array_free(&text);
	return success;
}

int		sgn_star(double x){return 1-((x<0)<<1);}//assumes zero is positive
int		sgn_int(int n){return (n>0)-(n<0);}
void	normalize(double dx, double dy, Point *ret)
{
	double invhyp=1/sqrt(dx*dx+dy*dy);
	ret->x=dx*invhyp;
	ret->y=dy*invhyp;
}
int		intersect_line_circle(double x1, double y1, double x2, double y2, double cx, double R, Point *ret_points)//https://mathworld.wolfram.com/Circle-LineIntersection.html
{
	double dx=x2-x1, dy=y2-y1, dr=sqrt(dx*dx+dy*dy);
	if(dr)
	{
		x1-=cx, x2-=cx;
		double D=x1*y2-x2*y1;
		double dr2=dr*dr;
		double disc=R*R*dr2-D*D;
		if(disc>=0)
		{
			disc=sqrt(disc), dr2=1/dr2;
			double
				a=cx+D*dy*dr2, b=sgn_star(dy)*dx*disc*dr2,
				c=-D*dx*dr2, d=fabs(dy)*disc*dr2;
			ret_points[0].x=a+b;
			ret_points[0].y=c+d;
			ret_points[1].x=a-b;
			ret_points[1].y=c-d;
			return 1;
		}
	}
	return 0;
}
int		intersect_ray_surface(double x1, double y1, double x2, double y2, double ap_min, double ap_max, double x, double R, Point *ret_point)//returns first future hit (if any), where (x1, y1) is in present
{
	int hit;
	Point sol[2];

	if(R!=R||!R||fabs(R)==_HUGE)//plane at x
	{
		if(x1==x2)
			return 0;
		ret_point->x=x;
		ret_point->y=y1+(y2-y1)/(x2-x1)*(x-x1);
	}
	else
	{
		double xcenter=x+R;
		hit=intersect_line_circle(x1, y1, x2, y2, xcenter, R, sol);
		if(hit)
		{
			int second=(R>0)==(sol[0].x<sol[1].x);//convex from left XNOR sol0 is leftmost
			*ret_point=sol[second];
		}
	}
	if(hit)
	{
		double abs_y=fabs(ret_point->y);
		hit=(x1==ret_point->x||(x1<x2)==(x1<ret_point->x))
			&&abs_y>=ap_min&&abs_y<ap_max;//intersection is in the future and inside aperture
	}
	return hit;
}

int		refract(double n_before, double n_after, double sin_in, double *cos_em, double *sin_em)
{
	double temp=n_before*sin_in;
	int reflection=temp>n_after;
	if(reflection)//total internal reflection
	{
		*sin_em=sin_in;
		*cos_em=-sqrt(1-*sin_em**sin_em);
	}
	else
	{
		*sin_em=temp/n_after;
		*cos_em=sqrt(1-*sin_em**sin_em);
	}
	return reflection;
}
int		refract_ray_surface(double x1, double y1, double x2, double y2, double ap_min, double ap_max, double nL, double nR, double x, double R, Point *ret_line)//(x1, y1) is in present
{
	int hit;
	Point p[2];
	double sin_in, sin_em, cos_em;

	hit=intersect_ray_surface(x1, y1, x2, y2, ap_min, ap_max, x, R, ret_line);
	if(!hit)
		return 0;

	if(!R||fabs(R)==_HUGE)//plane at x
	{
		normalize(x2-x1, y2-y1, p);
		sin_in=p->y;
		hit+=refract(nL, nR, sin_in, &cos_em, &sin_em);
		ret_line[1].x=ret_line[0].x+cos_em;
		ret_line[1].y=ret_line[0].y+sin_em;
	}
	else//+/- spherical at x
	{
		double xcenter=x+R;

		int inc_dir=sgn_star(x2-x1);
		normalize(x2-x1, y2-y1, p);
		normalize(sgn_star(R)*(xcenter-ret_line[0].x), -sgn_star(R)*ret_line[0].y, p+1);
		sin_in=p[0].y*p[1].x-p[0].x*p[1].y;
		hit+=refract(nL, nR, sin_in, &cos_em, &sin_em);
		double
			c2=cos_em*p[1].x-sin_em*p[1].y,
			s2=sin_em*p[1].x+cos_em*p[1].y;
		if(hit!=2&&inc_dir!=sgn_star(c2))
			c2=-c2, s2=-s2;
		ret_line[1].x=ret_line[0].x+c2;
		ret_line[1].y=ret_line[0].y+s2;
	}
	return hit;
}
double	extrapolate_x(double x1, double y1, double x2, double y2, double xCCD){return y1+(y2-y1)/(x2-x1)*(xCCD-x1);}
double	extrapolate_y(double x1, double y1, double x2, double y2, double yground){return x1+(x2-x1)/(y2-y1)*(yground-y1);}
double	distance(Point *p1, Point *p2)
{
	double dx=p2->x-p1->x, dy=p2->y-p1->y;
	return sqrt(dx*dx+dy*dy);
}
int		intersect_lines(Point *l1, Point *l2, Point *ret)//returns perpendicular distance in ret->x when the lines are parallel
{
	double
		*x[]={&l1[0].x, &l1[1].x, &l2[0].x, &l2[1].x},
		*y[]={&l1[0].y, &l1[1].y, &l2[0].y, &l2[1].y};
#define X(IDX)	(*x[IDX-1])
#define Y(IDX)	(*y[IDX-1])
	double dx1=X(1)-X(2), dy1=Y(1)-Y(2), dx2=X(3)-X(4), dy2=Y(3)-Y(4);
	double D=dx1*dy2-dy1*dx2;
	if(D)
	{
		double a=(X(1)*Y(2)-Y(1)*X(2))/D, b=(X(3)*Y(4)-Y(3)*X(4))/D;
		ret->x=a*dx2-b*dx1;
		ret->y=a*dy2-b*dy1;
		return 1;
	}
	ret->x=(-dx1*(Y(1)-Y(3))-(X(1)-X(3))*-dy1)/sqrt(dx1*dx1+dy1*dy1);
	ret->y=0;
	return 0;
#undef X
#undef Y
}
void	project_on_line(Point *line, Point *p, Point *ret)
{
	double
		dx=line[1].x-line[0].x, dy=line[1].y-line[0].y,
		t=(p->x*dx+p->y*dy)/(dx*dx+dy*dy);
	ret->x=t*dx;
	ret->y=t*dy;
}
void	line_make_perpendicular(Point *line, Point *position, Point *ret_line)
{
	ret_line[0]=*position;
	ret_line[1].x=ret_line[0].x+line[1].y-line[0].y;
	ret_line[1].y=ret_line[0].y+line[1].x-line[0].x;
}
void	meanvar(double *arr, int count, int stride, double *ret_mean, double *ret_var)
{
	double mean=0, var=0;
	if(count>0)
	{
		int nvals=count;
		count*=stride;
		for(int k=0;k<count;k+=stride)
			mean+=arr[k];
		mean/=nvals;
		var=0;
		for(int k=0;k<count;k+=stride)
		{
			double val=arr[k]-mean;
			var+=val*val;
		}
		var/=nvals;
	}
	if(ret_mean)
		*ret_mean=mean;
	if(ret_var)
		*ret_var=var;
}

#if 0
typedef struct GlassElemStruct
{
	int active;
	union
	{
		struct{double dist, Rl, th, Rr, n;};//cm;
		double vars[5];
	};
} GlassElem;
GlassElem		elements[]=
{
	//learned parameters (cm)
	//on	dist	Rl		th		Rr		n
	{1,		0,		51.2,	0.8,	10000,	1.512},	//53cm gives 98cm
	{1,		18,		16,		1,		-15,	1.512},
	{0,		50,		10,		1,		-8.5,	1.512},
	{0,		2,		10,		1,		10000,	1.512},

#if 0
	{1,		0,		40,		2,		40},
	{1,		1,		-40,	2,		4000},
	{1,		2,		40,		4,		-40},
	{1,		0.8,	40,		2,		40},

	{1,		2,		40,		5,		40},
	{1,		0.8,	-40,	5,		4000},

	{1,		1,		32,		4,		32},
#endif
};
GlassElem		*ebackup=0;
int				ecount=SIZEOF(elements), current_elem=0;
#endif


void			simulate()//number of rays must be even, always double-sided
{
	if(!elements||!elements->count)
	{
		LOG_ERROR("Elements array is empty");//optical device unknown
		return;
	}
	if(!photons||!photons->count)
	{
		LOG_ERROR("Photon array is empty");//colors unknown
		return;
	}
	if(!order||!order->count)
	{
		LOG_ERROR("Order path array is empty");//device operation unknown
		return;
	}
	
	//naive collision detection
#if 0
	for(int ke=0;ke<elements->count;++ke)
	{
		OpticElem *e1=(OpticElem*)array_at(&elements, ke);
		for(int ke2=0;ke2<elements->count;++ke2)
		{
			OpticElem *e2=(OpticElem*)array_at(&elements, ke2);
			double xmin=e1->left.pos, xmax=e1->right.pos;
			if(xmin>e2->left.pos)
				xmin=e2->left.pos;
			if(xmin<e2->right.pos)
				xmin=e2->right.pos;
			if(e2-e1<e1->right.pos-e1->left.pos+e2->right.pos-e2->left.pos)
			{
				LOG_ERROR("Inconsistent device: element \'%s\' intersects with element \'%s\'", e1->name->data, e2->name->data);
				return;
			}
		}
	}
#endif
	
	//find first element
#if 0
	int e1_idx=0;
	OpticElem *e1=(OpticElem*)array_at(&elements, e1_idx);
	for(int ke=1;ke<elements->count;++ke)
	{
		OpticElem *e2=(OpticElem*)array_at(&elements, ke);
		if(e1->left.pos>e2->left.pos)//naive left surface center comparison
		{
			e1_idx=ke;
			e1=(OpticElem*)array_at(&elements, e1_idx);
		}
	}
#endif
	
	//find rays range (from the target)
	double ymin, ymax;
	OpticElem *e1=(OpticElem*)array_at(&elements, initial_target.e_idx);
	Surface *surface=e1->surfaces+initial_target.is_right_not_left;
	if(initial_target.is_outer_not_inner)//is outer area
	{
		ymin=surface->r_max[0];
		ymax=surface->r_max[1];
	}
	else//is inner area
	{
		ymin=0;
		ymax=surface->r_max[0];
	}

	//find x range of optical device
	double xmin, xmax;
	e1=(OpticElem*)array_at(&elements, 0);
	xmin=e1->surfaces[0].pos;
	xmax=e1->surfaces[1].pos;
	for(int ke=1;ke<(int)elements->count;++ke)
	{
		e1=(OpticElem*)array_at(&elements, ke);
		if(xmin>e1->surfaces[0].pos)
			xmin=e1->surfaces[0].pos;
		if(xmax<e1->surfaces[1].pos)
			xmax=e1->surfaces[1].pos;
	}
	double xlength=xmax-xmin, xstart=xmin-xlength;

	//initialize incident rays
	AreaIdx *aidx=(AreaIdx*)array_at(&order, 0);
	//double r_sign=aidx->is_right_not_left?-1:1;
	e1=(OpticElem*)array_at(&elements, aidx->e_idx);
	surface=e1->surfaces+aidx->is_right_not_left;
	for(int kp=0;kp<(int)photons->count;++kp)
	{
		Photon *ph=(Photon*)array_at(&photons, kp);
		for(int kp2=0;kp2<(int)ph->paths->count;++kp2)//for each ray	path->count must be even
		{
			Path *path=(Path*)array_at(&ph->paths, kp2);
			array_clear(&path->points);
			Point *points=ARRAY_APPEND(path->points, 0, 2, 1, 0);
			points->x=xstart;
			points->y=ymin+(kp2>>1)*(ymax-ymin)/(ph->paths->count>>1)+0.5;
			if(kp2&1)//rays are in pairs mirrored by ground line
				points->y=-points->y;
			intersect_ray_surface(xstart, points->y, xstart+xlength, points->y, ymin, ymax, surface->pos, surface->radius, points+1);//must hit
		}
	}

	//main simulation loop
	for(int ke=0;ke<(int)order->count;++ke)
	{
		e1=(OpticElem*)array_at(&elements, ke);
		for(int kp=0;kp<(int)photons->count;++kp)
		{
			Photon *ph=(Photon*)array_at(&photons, kp);
			for(int kp2=0;kp2<(int)ph->paths->count;++kp2)//for each ray
			{
				Path *path=(Path*)array_at(&ph->paths, kp2);
				//for(int ke=0;ke<(int)elements->count;++ke)//for each boundary
				//{
				//	OpticElem *elem=(OpticElem*)array_at(&elements, ke);
				//	if(elem->active)
				//	{
				//	}
				//}
			}
		}
	}
}
#if 0
double			eval(GlassElem *elements, int ge_count, int nrays, int n_count, double aperture, double xend, double tan_tilt)
{
	Point l1[2], l2[2];
	Point combined_ground[2]={0};
	double combined_slope=0;
	//Point ground[2];
	GlassElem *ge;
	ArrayHandle ray_spread, emerge_count;

	int mirror_ray_count=nrays*twosides;
	//int mirror_ray_count=(nrays+1)*twosides;
	int ray_count=nrays+1+mirror_ray_count;
	for(int kp=0;kp<n_count;++kp)
	{
		Photon *lp=lightpaths+kp;
		int nrays0=array_size(&lp->paths);
		if(nrays0!=ray_count)
		{
			if(lp->paths)
			{
				for(int kp=0;kp<nrays0;++kp)//free array of paths
				{
					Path *p=(Path*)array_at(&lp->paths, kp);
					array_free(&p->points);
				}
				array_free(&lp->paths);
			}
			ARRAY_ALLOC(Path, lp->paths, ray_count, 0, "Path lightpaths[k]::paths[ray_count]");
		}
	}
	ARRAY_ALLOC(Point, ray_spread, 0, ray_count*n_count, "Point ray_spread[ray_count*n_count]");
	ARRAY_ALLOC(int, emerge_count, n_count, 0, "int emerge_count[n_count]");

	int n_wavelengths=0;
	double xpad=100, xstart=0;
	for(int kp=0;kp<n_count;++kp)//for each photon				simulate glass elements
	{
		//double n=n_base+n_deltas[kp];
		Photon *lp=lightpaths+kp;
		int *n_emerged=(int*)array_at(&emerge_count, kp);
		//lp->lambda=r_idx2wavelength(n);
		//lp->color=wavelength2rgb(lp->lambda);
		for(int start=-mirror_ray_count*twosides, kr=start;kr<=nrays;++kr)//for each ray/path
		{
			Path *p=(Path*)array_at(&lp->paths, kr-start);
			ARRAY_ALLOC(Point, p->points, 0, ge_count*2+1, "Point lightpaths[k]::paths[ray_count]::points[ecount*2+1]");
			double x=xstart;
			l1[1].x=x;
			l1[1].y=aperture*0.5*kr/(nrays+1);
			l1[0].x=x-xpad, l1[0].y=l1[1].y-xpad*tan_tilt;
			ARRAY_APPEND(p->points, l1, 1, 1, 0);
			for(int k2=0;k2<ge_count;++k2)//for each glass element
			{
				ge=elements+k2;
				double n=lambda2n(ge->n, lp->lambda);
				x+=ge->dist;
				if(ge->active)
				{
					p->emerged=refract_ray_surface(l1[0].x, l1[0].y, l1[1].x, l1[1].y, aperture, 1, n, x, ge->Rl, l2);
					if(!p->emerged)
					{
						ARRAY_APPEND(p->points, l1+1, 1, 1, 0);
						break;
					}
					ARRAY_APPEND(p->points, l2, 1, 1, 0);
					if(p->emerged==2)
					{
						ARRAY_APPEND(p->points, l2+1, 1, 1, 0);
						break;
					}
				}
				x+=ge->th;
				if(ge->active)
				{
					p->emerged=refract_ray_surface(l2[0].x, l2[0].y, l2[1].x, l2[1].y, aperture, n, 1, x, -ge->Rr, l1);
					if(!p->emerged)
					{
						ARRAY_APPEND(p->points, l2+1, 1, 1, 0);
						break;
					}
					ARRAY_APPEND(p->points, l1, 1, 1, 0);
					if(p->emerged==2)
					{
						ARRAY_APPEND(p->points, l1+1, 1, 1, 0);
						break;
					}
				}
			}
			if(p->emerged==1)
			{
				Point point;
				point.x=xend;
				point.y=extrapolate_x(l1[0].x, l1[0].y, l1[1].x, l1[1].y, xend);
				ARRAY_APPEND(p->points, &point, 1, 1, 0);
				p->em_ray[0].x=l1->x;//save last ray segment as emergence
				p->em_ray[0].y=l1->y;
				p->em_ray[1].x=point.x;
				p->em_ray[1].y=point.y;
			}
			array_fit(&p->points, 0);
		}
		*n_emerged=0;
		memset(lp->ground, 0, 2*sizeof(Point));
		for(int start=-mirror_ray_count*twosides, kr=start;kr<0;++kr)//for each ray/path
		{
			Path *p1=(Path*)array_at(&lp->paths, kr-start), *p2=(Path*)array_at(&lp->paths, -kr-start);
			if(p1->emerged==1&&p2->emerged==1)
			{
				Point point;
				intersect_lines(p1->em_ray, p2->em_ray, &point);
				ARRAY_APPEND(ray_spread, &point, 1, 1, 0);
				if(!*n_emerged)
					lp->ground[0]=lp->ground[1]=point;
				else
				{
					if(lp->ground[0].x>point.x)
						lp->ground[0]=point;
					if(lp->ground[1].x<point.x)
						lp->ground[1]=point;
				}
				++*n_emerged;
			}
		}
		if(*n_emerged)
		{
			combined_ground[0].x+=lp->ground[0].x;
			combined_ground[0].y+=lp->ground[0].y;
			combined_slope+=atan2(lp->ground[1].y-lp->ground[0].y, lp->ground[1].x-lp->ground[0].x);
			++n_wavelengths;
		}
	}
	if(!n_wavelengths)
		return 1e6;
	combined_ground[0].x/=n_wavelengths;
	combined_ground[0].y/=n_wavelengths;
	combined_slope=tan(combined_slope/n_wavelengths);
	combined_ground[1].x=combined_ground[0].x+1;
	combined_ground[1].y=combined_ground[0].y+combined_slope;

	Point variance_total={0};
	Point *mean_total=ray_spread_mean+n_count;
	mean_total->x=mean_total->y=0;
	for(int kp=0, start=0;kp<n_count;++kp)					//find ground cross centroid
	{
		int count=*(int*)array_at(&emerge_count, kp);
		if(count)
		{
			Point var={0}, *points=(Point*)array_at(&ray_spread, start);
			ray_spread_mean[kp].x=0;
			ray_spread_mean[kp].y=0;
			meanvar((double*)points, count, 2, &ray_spread_mean[kp].x, &var.x);
			meanvar((double*)points+1, count, 2, &ray_spread_mean[kp].y, &var.y);
			variance_total.x+=var.x;
			variance_total.y+=var.y;
			mean_total->x+=ray_spread_mean[kp].x;
			mean_total->y+=ray_spread_mean[kp].y;
			start+=count;
		}
	}
	array_free(&ray_spread);
	array_free(&emerge_count);
	mean_total->x/=n_count;
	mean_total->y/=n_count;
	variance_total.x=sqrt(variance_total.x/n_count);//standard deviation
	variance_total.y=sqrt(variance_total.y/n_count);
	double abs_stddev=sqrt(variance_total.x*variance_total.x+variance_total.y*variance_total.y);

	Point iplane[2], iplane_combined[2];
	total_blur=0;
	int total_npaths=0;
	line_make_perpendicular(combined_ground, ray_spread_mean+n_count, iplane_combined);
	for(int kp=0;kp<n_count;++kp)//for each photon (wavelength)			//cast rays on best image plane
	{
		Photon *lp=lightpaths+kp;
		lp->em_centroid=ray_spread_mean[kp];
		line_make_perpendicular(lp->ground, ray_spread_mean+kp, iplane);

		int npaths=array_size(&lp->paths);
		for(int kp=0;kp<npaths;++kp)//for each path
		{
			Path *p=(Path*)array_at(&lp->paths, kp);
			if(p->emerged)
			{
				Point cross;
				int intersect=intersect_lines(iplane, p->em_ray, &cross);
				if(intersect)
				{
					p->r_blur=distance(&lp->em_centroid, &cross);
					if(p->r_blur>1)
						break;
					intersect_lines(iplane_combined, p->em_ray, &cross);
					total_blur+=distance(ray_spread_mean+n_count, &cross);
					++total_npaths;
				}
			}
		}
	}
	if(total_npaths)
		total_blur/=total_npaths;

	if(history_enabled)
	{
		history[hist_idx]=abs_stddev;
		++hist_idx, hist_idx%=w;
	}
	return abs_stddev;
}
#define	EVAL()	spread=eval(elements, ecount, nrays, n_count, ap, 300, tan_tilt)
#endif
double			calc_loss()
{
	return spread;

	//double focal_length=100;
	//focal_length-=ray_spread_mean[n_count>>1];
	//return spread+focal_length*focal_length;
}
int				shift_lambdas(double delta)
{
	if(!photons||!photons->count)
	{
		LOG_ERROR("Photons array is empty");
		return 0;
	}
	Photon *ph=(Photon*)array_at(&photons, 0);
	double min_lambda=ph->lambda, max_lambda=ph->lambda;
	for(int kp=1;kp<(int)photons->count;++kp)
	{
		ph=(Photon*)array_at(&photons, kp);
		if(min_lambda>ph->lambda)
			min_lambda=ph->lambda;
		if(max_lambda<ph->lambda)
			max_lambda=ph->lambda;
	}
	if(min_lambda+delta<380||max_lambda+delta>750)
		return 0;
	for(int kp=0;kp<(int)photons->count;++kp)
	{
		ph=(Photon*)array_at(&photons, kp);
		ph->lambda+=delta;
		ph->color=wavelength2rgb(ph->lambda);
	}
	return 1;
}
void			change_aperture(Surface *surface, double delta)
{
	int same=surface->r_max[0]==surface->r_max[1];
	surface->r_max[1]+=delta;
	if(same)
		surface->r_max[0]=surface->r_max[1];
}
void			change_diopter(double *r, double delta)
{
	if(*r)
		*r=1/(1 / *r+delta);
	else
		*r=1/delta;
}
void			change_n(double *n, double delta)
{
	if(*n+delta<1.2||*n+delta>2)
		return;
	*n+=delta;
}
void			change_var(OpticElem *oe, int idx, double delta)
{
	//idx: {left.pos, left.radius, right.pos, right.radius}
	Surface *s=oe->surfaces+(idx>>1);
	if(idx&1)
		change_diopter(&s->radius, delta);
	else
		s->pos+=delta;
}
void			calc_grad(double *grad, double step, int *var_idx, int nvars, int exclude_elem)
{
	history_enabled=0;
	memset(grad, 0, elements->count*nvars*sizeof(double));
	for(int ke=exclude_elem;ke<(int)elements->count;++ke)
	{
		OpticElem *ge=(OpticElem*)array_at(&elements, ke);
		if(ge->active)
		{
			for(int kv=0;kv<nvars;++kv)
			{
				change_var(ge, var_idx[kv], step);
				simulate();
				change_var(ge, var_idx[kv], -step);
				grad[nvars*ke+kv]=calc_loss();
			}
		}
	}
	simulate();
	double loss=calc_loss();
	for(int ke=exclude_elem;ke<(int)elements->count;++ke)
	{
		OpticElem *oe=(OpticElem*)array_at(&elements, ke);
		if(oe->active)
		{
			for(int kv=0;kv<nvars;++kv)
				grad[nvars*ke+kv]-=loss;
		}
	}
	history_enabled=1;
}
void			update_params(double *grad, int *var_idx, int nvars, int exclude_elem)
{
	for(int ke=exclude_elem;ke<(int)elements->count;++ke)
	{
		OpticElem *ge=(OpticElem*)array_at(&elements, ke);
		if(ge->active)
		{
			for(int kv=0;kv<nvars;++kv)
				change_var(ge, var_idx[kv], grad[nvars*ke+kv]);
		}
	}
}


double			VX=0, VY=0, DX=20, AR_Y=1, Xstep, Ystep;
int				prec;
char			timer=0, drag=0, m_bypass=0, clearScreen=0, _2d_drag_graph_not_window=0;
void			calculate_scale(double DN, int n, double *Nstep)
{
	const double ln10=log(10.);
	double t=100*DN/n, t2=floor(log10(t));
	*Nstep=exp(t2*ln10), prec=t2<0?(int)(-t2):0;
	switch((int)(t/ *Nstep))
	{
	case 1:*Nstep*=1;break;
	case 2:*Nstep*=2;break;
	case 3:*Nstep*=2;break;
	case 4:*Nstep*=2;break;
	case 5:*Nstep*=5;break;
	case 6:*Nstep*=5;break;
	case 7:*Nstep*=5;break;
	case 8:*Nstep*=5;break;
	case 9:*Nstep*=5;break;
	default:*Nstep*=1;break;
	}
}
void			function1()
{
	calculate_scale(DX, w, &Xstep);
	if(AR_Y==1)
		Ystep=Xstep;
	else
		calculate_scale(DX*h/(w*AR_Y), h, &Ystep);
}
typedef struct ScaleConverterStruct
{
	double Xstart, Xr, Yend, Yr;
} ScaleConverter;
#define			real2screenX(XCOMP, SCALE)	(int)floor(((XCOMP)-SCALE->Xstart)*SCALE->Xr)
#define			real2screenY(YCOMP, SCALE)	(int)floor((SCALE->Yend-(YCOMP))*SCALE->Yr)
void			draw_line(double x1, double y1, double x2, double y2, ScaleConverter *scale)
{
	MoveToEx(ghMemDC, real2screenX(x1, scale), real2screenY(y1, scale), 0);
	LineTo(ghMemDC, real2screenX(x2, scale), real2screenY(y2, scale));
}
double			draw_arc(double x, double R, double ap, ScaleConverter *scale, int nsegments)//returns top x
{
	double r2=R*R;
	int x1, y1, x2, y2;
	if(!R)
	{
		x1=real2screenX(x, scale);
		y1=real2screenY(-ap*0.5, scale), y2=real2screenY(ap*0.5, scale);
		MoveToEx(ghMemDC, x1, y1, 0);
		LineTo(ghMemDC, x1, y2);
		return x;
	}
	double ty, tx;
	for(int k=-nsegments;k<=nsegments;++k)
	{
		ty=ap*k/nsegments*0.5, tx=x+R-sgn_star(R)*sqrt(r2-ty*ty);
		x2=real2screenX(tx, scale), y2=real2screenY(ty, scale);
		if(k>-nsegments)
		{
			MoveToEx(ghMemDC, x1, y1, 0);
			LineTo(ghMemDC, x2, y2);
		}
		x1=x2, y1=y2;
	}
	return tx;
}
void			draw_curve(Point *points, int npoints, ScaleConverter *scale)
{
	for(int k=0;k<npoints-1;++k)
	{
		MoveToEx(ghMemDC, real2screenX(points[k].x, scale), real2screenY(points[k].y, scale), 0);
		LineTo(ghMemDC, real2screenX(points[k+1].x, scale), real2screenY(points[k+1].y, scale));
	}
}
void			render()
{
	memset(rgb, 0xFF, rgbn*sizeof(int));
	
	if(h)
	{
		double DY=DX*h/(w*AR_Y);
		double Xr=w/DX, Yr=h/DY;//unity in pixels
		double Ystart=VY-DY/2, Yend=VY+DY/2, Xstart=VX-DX/2, Xend=VX+DX/2;
		ScaleConverter sc={Xstart, Xr, Yend, Yr}, *scale=&sc;
		if(!clearScreen)
		{
			hPen=(HPEN)SelectObject(ghMemDC, hPen), hBrush=(HBRUSH)SelectObject(ghMemDC, hBrush);
			double Xstepx2=Xstep*2, Ystepx2=Ystep*2;
			for(double y=ceil(Yend/Ystepx2)*Ystepx2, yEnd=floor(Ystart/Ystep)*Ystep-Ystepx2;y>yEnd;y-=Ystepx2)
			{
				for(double x=floor(Xstart/Xstepx2)*Xstepx2, xEnd=ceil (Xend/Xstep)*Xstep;x<xEnd;x+=Xstepx2)
				{
					int x1=real2screenX(x, scale), x2=real2screenX(x+Xstep, scale), x3=real2screenX(x+Xstepx2, scale),
						y1=real2screenY(y, scale), y2=real2screenY(y+Ystep, scale), y3=real2screenY(y+Ystepx2, scale);
					Rectangle(ghMemDC, x1, y1, x2, y2);
					Rectangle(ghMemDC, x2, y2, x3, y3);
				}
			}
			hPen=(HPEN)SelectObject(ghMemDC, hPen), hBrush=(HBRUSH)SelectObject(ghMemDC, hBrush);
		}
		for(int kn=0;kn<(int)photons->count;++kn)				//draw light paths
		{
			Photon *lp=(Photon*)array_at(&photons, kn);
			HPEN hPenRay=CreatePen(PS_SOLID, 1, lp->color);
			hPenRay=(HPEN)SelectObject(ghMemDC, hPenRay);
			for(int kp=0, npaths=array_size(&lp->paths);kp<npaths;++kp)
			{
				Path *p=(Path*)array_at(&lp->paths, kp);
				Point *points=(Point*)array_at(&p->points, 0);
				int npoints=array_size(&p->points);
				draw_curve(points, npoints, scale);
			}
			hPenRay=(HPEN)SelectObject(ghMemDC, hPenRay);
			DeleteObject(hPenRay);
		}

		hPenLens=(HPEN)SelectObject(ghMemDC, hPenLens);
		int segments=25;
		double topx1, topx2;
		for(int ke=0;ke<(int)elements->count;++ke)			//draw optical elements
		{
			OpticElem *oe=(OpticElem*)array_at(&elements, ke);
			if(oe->active)
			{
				topx1=draw_arc(oe->surfaces[0].pos, oe->surfaces[0].radius, oe->surfaces[0].r_max[1], scale, segments);
				topx2=draw_arc(oe->surfaces[1].pos, oe->surfaces[1].radius, oe->surfaces[1].r_max[1], scale, segments);
				draw_line(topx1, oe->surfaces[0].r_max[1], topx2, oe->surfaces[1].r_max[1], scale);
				draw_line(topx1, -oe->surfaces[0].r_max[1], topx2, -oe->surfaces[1].r_max[1], scale);
			}
		}
		hPenLens=(HPEN)SelectObject(ghMemDC, hPenLens);
		
		hBrushHollow=(HBRUSH)SelectObject(ghMemDC, hBrushHollow);
		for(int kn=0;kn<(int)photons->count;++kn)				//draw blur circles
		{
			Photon *lp=(Photon*)array_at(&photons, kn);
			int npaths=array_size(&lp->paths);
			int xcenter=real2screenX(lp->em_centroid.x, scale),
				ycenter=real2screenY(lp->em_centroid.y, scale);

			HPEN tempPen=(HPEN)CreatePen(PS_SOLID, 1, lp->color);
			tempPen=(HPEN)SelectObject(ghMemDC, tempPen);
			for(int kp=0;kp<npaths;++kp)
			{
				Path *p=(Path*)array_at(&lp->paths, kp);
				if(p->emerged)
				{
					int rx=(int)(p->r_blur*Xr), ry=(int)(p->r_blur*Yr);
					Ellipse(ghMemDC, xcenter-rx, ycenter-ry, xcenter+rx, ycenter+ry);
				}
			}
			tempPen=(HPEN)SelectObject(ghMemDC, tempPen);
			DeleteObject(tempPen);
		}
		{
			Point *point=(Point*)array_back(&ray_spread_mean);
			int xcenter=real2screenX(point->x, scale),
				ycenter=real2screenY(point->y, scale);
			int rx=(int)(total_blur*Xr), ry=(int)(total_blur*Yr);
			Ellipse(ghMemDC, xcenter-rx, ycenter-ry, xcenter+rx, ycenter+ry);//combined blur circle
		}
		hBrushHollow=(HBRUSH)SelectObject(ghMemDC, hBrushHollow);

		if(!clearScreen)
		{
			double histmax=-1;
			for(int k=0;k<w;++k)
			{
				if(history[k]>=0&&(histmax==-1||histmax<history[k]))
					histmax=history[k];
			}
			if(histmax>0)
			{
				histmax=h/histmax;
				for(int k=1;k<w;++k)
				{
					if(history[k-1]>=0&&history[k]>=0)
					{
						MoveToEx(ghMemDC, k-1, h-(int)(history[k-1]*histmax), 0);
						LineTo(ghMemDC, k, h-(int)(history[k]*histmax));
					}
				}
			}

			int H=(int)(VY-DY/2>0?h:VY+DY/2<0?-1:h*(VY/DY+.5)), HT=H+(H>h-30?-18:2), V=(int)(VX-DX/2>0?-1:VX+DX/2<0?w:w*(-VX+DX/2)/DX), VT=V+(int)(V>w-24-prec*8?-24-prec*8:2);
			int bkMode=GetBkMode(ghMemDC);
			SetBkMode(ghMemDC, TRANSPARENT);
			for(double x=floor((VX-DX/2)/Xstep)*Xstep, xEnd=ceil((VX+DX/2)/Xstep)*Xstep, Xstep_2=Xstep/2;x<xEnd;x+=Xstep)
			{
				if(x>-Xstep_2&&x<Xstep_2)
					continue;
				int linelen=sprintf_s(g_buf, 128, "%g", x);
				double X=(x-(VX-DX/2))*Xr;
				TextOutA(ghMemDC, (int)(X)-(X<0)+2, HT, g_buf, linelen);
			}
			for(double y=ceil((VY+DY/2)/Ystep)*Ystep, yEnd=floor((VY-DY/2)/Ystep)*Ystep, Ystep_2=Ystep/2;y>yEnd;y-=Ystep)
			{
				if(y>-Ystep_2&&y<Ystep_2)
					continue;
				int linelen=sprintf_s(g_buf, 128, "%g", y);
				double Y=((VY+DY/2)-y)*Yr;
				TextOutA(ghMemDC, VT, (int)(Y)-(Y<0)+2, g_buf, linelen);
			}
			MoveToEx(ghMemDC, 0, H, 0), LineTo(ghMemDC, w, H), MoveToEx(ghMemDC, V, 0, 0), LineTo(ghMemDC, V, h);
			SetBkMode(ghMemDC, bkMode);

			for(int k=0;k<(int)photons->count;++k)//draw ray ground
			{
				Photon *lp=(Photon*)array_at(&photons, k);
				MoveToEx(ghMemDC, real2screenX(lp->ground[0].x, scale), real2screenY(lp->ground[0].y, scale), 0);
				LineTo(ghMemDC, real2screenX(lp->ground[1].x, scale), real2screenY(lp->ground[1].y, scale));
			}

			int y=0;
			GUITPrint(ghMemDC, 0, y, 0, "Shift +/-\t\ttilt"), y+=16;
			GUITPrint(ghMemDC, 0, y, 0, "Crtl +/-\t\tchange wavelength"), y+=16;
			GUITPrint(ghMemDC, 0, y, 0, "T\t\tcenter at focus"), y+=16;
			GUITPrint(ghMemDC, 0, y, 0, "tab\t\tselect glass element"), y+=16;
			GUITPrint(ghMemDC, 0, y, 0, "space\t\ttoggle current glass element"), y+=16;
			GUITPrint(ghMemDC, 0, y, 0, "1+left/right\tchange distance"), y+=16;
			GUITPrint(ghMemDC, 0, y, 0, "2+left/right\tchange left radius"), y+=16;
			GUITPrint(ghMemDC, 0, y, 0, "3+left/right\tchange thickness"), y+=16;
			GUITPrint(ghMemDC, 0, y, 0, "4+left/right\tchange right radius"), y+=16;
			GUITPrint(ghMemDC, 0, y, 0, "F\t\tflip glass element"), y+=16;
			GUITPrint(ghMemDC, 0, y, 0, "O\t\toptimize"), y+=48;

			GUITPrint(ghMemDC, 0, y, 0, "\t1 dist\t2 Rl\t3 Th\t4 Rr\t5 n"), y+=32;
			for(int ke=0;ke<(int)elements->count;++ke)
			{
				OpticElem *oe=(OpticElem*)array_at(&elements, ke);
				double th=oe->surfaces[1].pos-oe->surfaces[0].pos;
				GUITPrint(ghMemDC, 0, y, 0, "%s  %c: %c\t%g\t%g\t%g\t%g\t%g%s", current_elem==ke?"->":" ", 'A'+ke, oe->active?'V':'X', oe->surfaces[0].pos, oe->surfaces[0].radius, th, -oe->surfaces[1].radius, oe->n, current_elem==ke?"\t<-":""), y+=16;
			}
			Point *point=(Point*)array_at(&ray_spread_mean, (int)photons->count);
			double focus=sqrt(point->x*point->x+point->y*point->y);
			AreaIdx *aidx=(AreaIdx*)array_at(&order, 0);
			OpticElem *oe=(OpticElem*)array_at(&elements, aidx->e_idx);
			double ap=oe->surfaces[aidx->is_right_not_left].r_max[1];
			y=h-16*6;
			GUITPrint(ghMemDC, 0, y, 0, "D %gcm, F %gcm, f/%g\t Std.Dev %lfmm", ap, focus, focus/ap, 10*spread), y+=16;

			int txtColor0=GetTextColor(ghMemDC);
			for(int kp=0;kp<(int)photons->count;++kp)
			{
				Photon *ph=(Photon*)array_at(&photons, kp);
				SetTextColor(ghMemDC, ph->color);
				GUIPrint(ghMemDC, 0, y, "lambda%d = %gnm", kp+1, ph->lambda), y+=16;
			}
			SetTextColor(ghMemDC, txtColor0);
		}
	}
	
	QueryPerformanceCounter(&li);
	GUIPrint(ghMemDC, 0, h-16, "fps=%.10f, T=%.10fms", freq/(double)(li.QuadPart-nticks), 1000.*(li.QuadPart-nticks)/freq);
	nticks=li.QuadPart;
	BitBlt(ghDC, 0, 0, w, h, ghMemDC, 0, 0, SRCCOPY);
}
void			wnd_resize()
{
	GetClientRect(ghWnd, &R);
	w=R.right-R.left, h=R.bottom-R.top, centerP.x=w/2, centerP.y=h/2;
	ClientToScreen(ghWnd, &centerP);

	history=realloc(history, w*sizeof(double));
	hist_idx%=w;
	for(int k=hist_idx;k<w;++k)
		history[k]=-1;
	//memset(history+hist_idx, 0, (w-hist_idx)*sizeof(double));
}
long __stdcall	WndProc(HWND hWnd, unsigned int message, unsigned int wParam, long lParam)
{
	switch(message)
	{
	case WM_CREATE:
		break;
	case WM_SIZE:
		wnd_resize();
		if(h)
		{
			rgbn=h*w;
			DeleteObject((HBITMAP)SelectObject(ghMemDC, hBitmap));//DIB
			BITMAPINFO bmpInfo={{sizeof(BITMAPINFOHEADER), w, -h, 1, 32, BI_RGB, 0, 0, 0, 0, 0}};
			hBitmap=(HBITMAP)SelectObject(ghMemDC, CreateDIBSection(0, &bmpInfo, DIB_RGB_COLORS, (void**)&rgb, 0, 0));
			SetBkMode(ghMemDC, TRANSPARENT);
			InvalidateRect(ghWnd, 0, 0);
		}
		break;
	case WM_ACTIVATE:
		switch(wParam)
		{
		case WA_ACTIVE:
		case WA_CLICKACTIVE:
			for(int k=0;k<256;++k)
				kb[k]=(GetKeyState(k)&0x8000)!=0;
			kp=kb[VK_UP]+kb[VK_DOWN]+kb[VK_LEFT]+kb[VK_RIGHT]
				+kb[VK_ADD]+kb[VK_SUBTRACT]//numpad
				+kb[VK_OEM_PLUS]+kb[VK_OEM_MINUS]
				+kb[VK_RETURN]+kb[VK_BACK];
			if(kp&&!timer)
				timer=1, SetTimer(ghWnd, 0, 10, 0);
			break;
		case WA_INACTIVE:
			kb[VK_LBUTTON]=0;
			if(drag)
			{
				ReleaseCapture();
				SetCursorPos(mouseP0.x, mouseP0.y);
				ShowCursor(1);
				drag=0;
			}
			kp=0;
			timer=0, KillTimer(ghWnd, 0);
			break;
		}
		break;
	case WM_PAINT:
		render();
		break;
	case WM_TIMER:
		{
			int e=0;
			double dx=DX/w;
			OpticElem *oe=(OpticElem*)array_at(&elements, current_elem);
			if(kb['1']||kb[VK_NUMPAD1])//R2a
			{
				if(kb[VK_LEFT	]&&oe->active)	oe->surfaces[0].pos-=dx, oe->surfaces[1].pos-=dx, e=1;
				if(kb[VK_RIGHT	]&&oe->active)	oe->surfaces[0].pos+=dx, oe->surfaces[1].pos+=dx, e=1;
			}
			else if(kb['2']||kb[VK_NUMPAD2])//R2b
			{
				if(kb[VK_LEFT	]&&oe->active)	change_diopter(&oe->surfaces[0].radius, -0.01*dx), e=1;
				if(kb[VK_RIGHT	]&&oe->active)	change_diopter(&oe->surfaces[0].radius, 0.01*dx), e=1;
			}
			else if(kb['3']||kb[VK_NUMPAD3])//t2
			{
				if(kb[VK_LEFT	]&&oe->active)	oe->surfaces[1].pos-=dx, e=1;
				if(kb[VK_RIGHT	]&&oe->active)	oe->surfaces[1].pos+=dx, e=1;
			}
			else if(kb['4']||kb[VK_NUMPAD4])//separation
			{
				if(kb[VK_LEFT	]&&oe->active)	change_diopter(&oe->surfaces[1].radius, 0.01*dx), e=1;
				if(kb[VK_RIGHT	]&&oe->active)	change_diopter(&oe->surfaces[1].radius, -0.01*dx), e=1;
			}
			else if(kb['5']||kb[VK_NUMPAD5])//refractive index
			{
				if(kb[VK_LEFT	]&&oe->active)	change_n(&oe->n, 0.01*dx), e=1;
				if(kb[VK_RIGHT	]&&oe->active)	change_n(&oe->n, -0.01*dx), e=1;
			}
			else if(_2d_drag_graph_not_window)
			{
				if(kb[VK_LEFT	])	VX+=10*DX/w;
				if(kb[VK_RIGHT	])	VX-=10*DX/w;
				if(kb[VK_UP		])	VY-=10*DX/(w*AR_Y);
				if(kb[VK_DOWN	])	VY+=10*DX/(w*AR_Y);
			}
			else
			{
				if(kb[VK_LEFT	])	VX-=10*DX/w;
				if(kb[VK_RIGHT	])	VX+=10*DX/w;
				if(kb[VK_UP		])	VY+=10*DX/(w*AR_Y);
				if(kb[VK_DOWN	])	VY-=10*DX/(w*AR_Y);
			}
			if(kb[VK_ADD		]||kb[VK_RETURN	]||kb[VK_OEM_PLUS	])
			{
				if(kb[VK_CONTROL])
					e|=shift_lambdas(1);
				//{
				//	if(lambda<750)
				//		lambda+=1, n_base=wavelength2refractive_index(lambda), e=1;
				//}
				else if(kb[VK_SHIFT])
				{
					if(tan_tilt<0.7071)
						tan_tilt+=0.00002*DX, e=1;
				}
				else if(kb['A'])
				{
					change_aperture(oe->surfaces, 0.1*dx);
					change_aperture(oe->surfaces+1, 0.1*dx);
					e=1;

					//if(ap<100)
					//	ap+=0.1, e=1;
				}
				else if(kb['X'])	DX/=1.05, AR_Y/=1.05;
				else if(kb['Y'])	AR_Y*=1.05;
				else				DX/=1.05;
				function1();
			}
			if(kb[VK_SUBTRACT	]||kb[VK_BACK	]||kb[VK_OEM_MINUS	])
			{
				if(kb[VK_CONTROL])
					e|=shift_lambdas(-1);
				//{
				//	if(lambda>380)
				//		lambda-=1, n_base=wavelength2refractive_index(lambda), e=1;
				//}
				else if(kb[VK_SHIFT])
				{
					if(tan_tilt>-0.7071)
						tan_tilt-=0.00002*DX, e=1;
				}
				else if(kb['A'])
				{
					change_aperture(oe->surfaces, -0.1*dx);
					change_aperture(oe->surfaces+1, -0.1*dx);
					e=1;
					//if(ap>0)
					//	ap-=0.1, e=1;
				}
				else if(kb['X'])	DX*=1.05, AR_Y*=1.05;
				else if(kb['Y'])	AR_Y/=1.05;
				else				DX*=1.05;
				function1();
			}
			if(e)
				simulate();
			InvalidateRect(ghWnd, 0, 0);
			if(!kp)
				timer=0, KillTimer(ghWnd, 0);
		}
		break;

	case WM_MOUSEMOVE:
		mx=((short*)&lParam)[0];
		my=((short*)&lParam)[1];
		if(drag)
		{
			if(!m_bypass)
			{
				int dx=mx-w/2, dy=h/2-my;
				if(_2d_drag_graph_not_window)
					dx=-dx, dy=-dy;
				VX+=dx*DX/w, VY+=dy*DX/(w*AR_Y);
				if(!timer)
					InvalidateRect(ghWnd, 0, 0);
				SetCursorPos(centerP.x, centerP.y);//moves mouse
			}
			m_bypass=!m_bypass;
		}
		return 0;
	case WM_LBUTTONDOWN:
		kb[VK_LBUTTON]=1;
		if(!drag)
		{
			ShowCursor(0);
			GetCursorPos(&mouseP0);
			SetCursorPos(centerP.x, centerP.y);
			SetCapture(ghWnd);
			drag=1;
			InvalidateRect(ghWnd, 0, 0);
		}
		return 0;
	case WM_LBUTTONUP:
		kb[VK_LBUTTON]=0;
		if(drag)
		{
			ReleaseCapture();
			SetCursorPos(mouseP0.x, mouseP0.y);
			ShowCursor(1);
			drag=0;
		//	POINT p=mouseP0;
		//	ScreenToClient(ghWnd, &p), ((short*)&oldMouse)[0]=(short)p.x, ((short*)&oldMouse)[1]=(short)p.y;
		}
		return 0;

	case WM_MOUSEWHEEL:
		{
			double dx=(mx-w/2)*DX/w, dy=(h/2-my)*DX/(w*AR_Y);
			int mw_forward=((short*)&wParam)[1]>0;
			if(kb[VK_CONTROL])
			{
				nrays+=mw_forward-!mw_forward;
				if(nrays<1)
					nrays=1;
				simulate();
			}
			else if(kb['X'])
			{
					 if(mw_forward)	DX/=1.1, AR_Y/=1.1, VX=VX+dx-dx/1.1;
				else				DX*=1.1, AR_Y*=1.1, VX=VX+dx-dx*1.1;
			}
			else if(kb['Y'])
			{
					 if(mw_forward)	AR_Y*=1.1, VY=VY+dy-dy/1.1;
				else				AR_Y/=1.1, VY=VY+dy-dy*1.1;
			}
			else
			{
					 if(mw_forward)	DX/=1.1, VX=VX+dx-dx/1.1, VY=VY+dy-dy/1.1;
				else				DX*=1.1, VX=VX+dx-dx*1.1, VY=VY+dy-dy*1.1;
			}
		}
		function1();
		InvalidateRect(ghWnd, 0, 0);
		return 0;
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		switch(wParam)
		{
		case VK_UP:case VK_DOWN:case VK_LEFT:case VK_RIGHT:
		case VK_ADD:case VK_SUBTRACT://numpad
		case VK_OEM_PLUS:case VK_OEM_MINUS:
		case VK_RETURN:case VK_BACK:
			if(!kb[wParam])
				++kp;
			if(!timer)
				SetTimer(ghWnd, 0, 10, 0), timer=1;
			break;
		case VK_TAB:
			current_elem=mod(current_elem+!kb[VK_SHIFT]-kb[VK_SHIFT], (int)elements->count);
			break;
		case VK_SPACE:
			{
				OpticElem *oe=(OpticElem*)array_at(&elements, current_elem);
				oe->active=!oe->active;
				simulate();
			}
			break;
		case VK_F1:
			messagebox("Controls",
				"Up/down/left/right/drag: Navigate\n"
				"+/-/Enter/Backspace/Mousewheel: Zoom\n"
				"X/Y +/-/Enter/Backspace: Change aspect ratio\n"
				"T: Go to focal point\n"
				"R: Reset view\n"
				"E: Reset scale\n"
				"C: Toggle clear screen\n"
				"\n"
				"Tab / Shift Tab: Select glass element\n"
				"Space: Toggle glass element\n"
				"F: Flip glass element\n"
				"\n"
				"1 left/right: Move glass element\n"
				"2 left/right: Change left diopter\n"
				"3 left/right: Change thickness\n"
				"4 left/right: Change right diopter\n"
				"5 left/right: Change refractive index\n"
				"Speed of change depends on zoom level\n"
				"\n"
				"Shift R: Reset all glass elements\n"
				"1/2/3/4/5 R: Reset corresponding property of current glass element\n"
				"\n"
				"A +/-/Enter/Backspace: Change aperture\n"
				"Ctrl +/-/Enter/Backspace: Shift wavelengths\n"
				"Shift +/-/Enter/Backspace: Change ray tilt\n"
				"Ctrl Mousewheel: Change ray count\n"
				"\n"
				"H: Clear history buffer\n"
				"O: Optimize\n"
				"Shift O: Optimize excluding first element\n"
				"\n"
				"Built on: %s %s", __DATE__, __TIME__);
			break;
		case 'O':
			if(kb[VK_CONTROL])//open another system
			{
				if(open_system())
				{
					array_free(&ebackup);
					ebackup=array_copy(&elements);
					simulate();
				}
				else
				{
					array_free(&elements);
					elements=array_copy(&ebackup);
				}
			}
			else//optimize
			{
				static double *grad=0, *velocity=0;
				int var_idx[]={0, 1, 2, 3};
				int nvars=SIZEOF(var_idx);
				if(!grad)
				{
					int bytesize=(int)elements->count*nvars*sizeof(double);
					grad	=(double*)malloc(bytesize);
					velocity=(double*)malloc(bytesize);
					memset(velocity, 0, bytesize);
				}
				int exclude_first=kb[VK_SHIFT]!=0;
				calc_grad(grad, 0.001, var_idx, nvars, exclude_first);
				double gain=0.0000001*DX;
				for(int k=0;k<16;++k)
				{
					if(grad[k])
						velocity[k]=velocity[k]*0.5-gain*grad[k];
					else
						velocity[k]*=0.5;
				}
				update_params(velocity, var_idx, nvars, exclude_first);
				simulate();
			}
			break;
		case 'C':
			clearScreen=!clearScreen;
			break;
		case 'H':
			for(int k=0;k<w;++k)
				history[k]=-1;
			break;
		case 'P':
			{
				OpticElem *oe=(OpticElem*)array_at(&elements, current_elem), *oe2;
				int e=0;
				if(kb['2'])
				{
					if(oe->surfaces[0].radius)
						oe->surfaces[0].radius=0;
					else
					{
						oe2=(OpticElem*)array_at(&ebackup, current_elem);
						oe->surfaces[0].radius=oe2->surfaces[0].radius;
					}
					e=1;
				}
				if(kb['4'])
				{
					if(oe->surfaces[1].radius)
						oe->surfaces[1].radius=0;
					else
					{
						oe2=(OpticElem*)array_at(&ebackup, current_elem);
						oe->surfaces[1].radius=oe2->surfaces[1].radius;
					}
					e=1;
				}
				if(e)
					simulate();
			}
			break;
		case 'T':
			{
				Point *p=(Point*)array_at(&ray_spread_mean, photons->count);
				VX=p->x;
				VY=p->y;
			}
			break;
		//case 'M':
		//	twosides=!twosides;
		//	simulate();
		//	break;
		case 'F'://flip optical element
			{
				OpticElem *oe=(OpticElem*)array_at(&elements, current_elem);
				if(oe->active)
				{
					double temp;

					temp=oe->surfaces[0].radius;
					oe->surfaces[0].radius=-oe->surfaces[1].radius;
					oe->surfaces[1].radius=-temp;

					simulate();
				}
			}
			break;
		case 'R':
			{
				int e=0;
				OpticElem *oe=(OpticElem*)array_at(&elements, current_elem);
				OpticElem *oe0=(OpticElem*)array_at(&ebackup, current_elem);
				if(kb[VK_SHIFT])
				{
					memcpy(elements, ebackup, elements->count*sizeof(OpticElem));
					tan_tilt=0;
					//ap=ap0;
					//lambda=lambda0;
					e=1;
				}
				else if(kb['1'])
					oe->surfaces[0].pos=oe0->surfaces[0].pos, e=1;
				else if(kb['2'])
					oe->surfaces[0].radius=oe0->surfaces[0].radius, e=1;
				else if(kb['3'])
					oe->surfaces[1].pos=oe->surfaces[0].pos+(oe0->surfaces[1].pos-oe0->surfaces[0].pos), e=1;
				else if(kb['4'])
					oe->surfaces[1].radius=oe0->surfaces[1].radius, e=1;
				else if(kb['5'])
					oe->n=oe0->n, e=1;
				if(e)
					simulate();
				else
				{
					if(!kb[VK_CONTROL])
						DX=20, AR_Y=1, function1();
					VX=VY=0;
				}
			}
			break;
		case 'E':
			if(kb[VK_CONTROL])
				DX=20;
			else
				DX/=sqrt(AR_Y);//average zoom
			AR_Y=1, function1();
			break;
		case VK_F8://toggle 2d drag convention
			_2d_drag_graph_not_window=!_2d_drag_graph_not_window;
			break;
		default:
			if(lParam&1<<30)
				return 0;
			break;
		}
		kb[wParam]=1;
		if(!timer)
			InvalidateRect(ghWnd, 0, 0);
		if(message==WM_SYSKEYDOWN)
			break;
		return 0;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		kb[wParam]=0;
		switch(wParam)
		{
		case VK_UP:case VK_DOWN:case VK_LEFT:case VK_RIGHT:
		case VK_ADD:case VK_SUBTRACT://numpad
		case VK_OEM_PLUS:case VK_OEM_MINUS:
		case VK_RETURN:case VK_BACK:
			if(kp)//start+key
				--kp;
			break;
		}
		break;
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	}
	return DefWindowProcA(hWnd, message, wParam, lParam);
}
int __stdcall	WinMain(HINSTANCE hInstance, HINSTANCE hPrev, char *cmdargs, int nCmdShow)
{
	WNDCLASSEXA wndClassEx=
	{
		sizeof(WNDCLASSEXA), CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS,
		WndProc, 0, 0, hInstance,
		LoadIconA(0, (char*)0x00007F00),
		LoadCursorA(0, (char*)0x00007F00),

		0,
	//	(HBRUSH)(COLOR_WINDOW+1),

		0, "New format", 0
	};

	ghInstance=hInstance;
	hPen=CreatePen(PS_SOLID, 1, _2dCheckColor);
	hBrush=CreateSolidBrush(_2dCheckColor);
	hPenLens=CreatePen(PS_SOLID, 1, 0x00FF00FF);
	hBrushHollow=(HBRUSH)GetStockObject(NULL_BRUSH);

	RegisterClassExA(&wndClassEx);
	ghWnd=CreateWindowExA(0, wndClassEx.lpszClassName, "Optics Simulator", WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_CLIPCHILDREN, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 0, 0, hInstance, 0);//20220504
	
		if(!open_system())
			return 1;
		wnd_resize();
		function1();

		shift_lambdas(0);
		ebackup=array_copy(&elements);
		simulate();

	ShowWindow(ghWnd, nCmdShow);
	
		QueryPerformanceFrequency(&li);freq=li.QuadPart;
		QueryPerformanceCounter(&li), srand(li.LowPart);
		nticks=li.QuadPart;

		ghDC=GetDC(ghWnd), ghMemDC=CreateCompatibleDC(ghDC);
		BITMAPINFO bmpInfo={{sizeof(BITMAPINFOHEADER), w, -h, 1, 32, BI_RGB, 0, 0, 0, 0, 0}};
		hBitmap=(HBITMAP)SelectObject(ghMemDC, CreateDIBSection(0, &bmpInfo, DIB_RGB_COLORS, (void**)&rgb, 0, 0));
		SetBkMode(ghMemDC, TRANSPARENT);

	MSG msg;
	for(;GetMessageA(&msg, 0, 0, 0);)TranslateMessage(&msg), DispatchMessageA(&msg);
	
		DeleteObject(SelectObject(ghMemDC, hBitmap)), DeleteDC(ghMemDC);

	return msg.wParam;
}