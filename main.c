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
#include<time.h>
#include<Windows.h>
#include<sys/stat.h>

#include<io.h>//for console
#include<fcntl.h>

//	#define		ENABLE_OPTIMIZER

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
HPEN			hPen=0, hPenLens=0, hPenMirror=0;
HBRUSH			hBrush=0, hBrushHollow=0;
int				w=0, h=0, X0, Y0;
int				mx=0, my=0, kpressed=0;
char			keyboard[256]={0};
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
int				consoleactive=0;
void			console_start()//https://stackoverflow.com/questions/191842/how-do-i-get-console-output-in-c-with-a-windows-program
{
	if(!consoleactive)
	{
		consoleactive=1;
		int hConHandle;
		long lStdHandle;
		CONSOLE_SCREEN_BUFFER_INFO coninfo;
		FILE *fp;

		// allocate a console for this app
		AllocConsole();

		// set the screen buffer to be big enough to let us scroll text
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
		coninfo.dwSize.Y=1000;
		SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

		// redirect unbuffered STDOUT to the console
		lStdHandle=(long)GetStdHandle(STD_OUTPUT_HANDLE);
		hConHandle=_open_osfhandle(lStdHandle, _O_TEXT);
		fp=_fdopen(hConHandle, "w");
		*stdout=*fp;
		setvbuf(stdout, 0, _IONBF, 0);

		// redirect unbuffered STDIN to the console
		lStdHandle=(long)GetStdHandle(STD_INPUT_HANDLE);
		hConHandle=_open_osfhandle(lStdHandle, _O_TEXT);
		fp=_fdopen(hConHandle, "r");
		*stdin=*fp;
		setvbuf(stdin, 0, _IONBF, 0);

		// redirect unbuffered STDERR to the console
		lStdHandle=(long)GetStdHandle(STD_ERROR_HANDLE);
		hConHandle=_open_osfhandle(lStdHandle, _O_TEXT);
		fp=_fdopen(hConHandle, "w");
		*stderr=*fp;
		setvbuf(stderr, 0, _IONBF, 0);

#ifdef __cplusplus
		// make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog
		// point to console as well
		std::ios::sync_with_stdio();
#endif

		printf("\n\tWARNNG: CLOSING THIS WINDOW WILL CLOSE THE PROGRAM\n\n");
	}
}
void			console_end()
{
	if(consoleactive)
	{
		FreeConsole();
		consoleactive=0;
	}
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
void 			memswap(void *p1, void *p2, size_t size, void *temp)
{
	memcpy(temp, p1, size);
	memcpy(p1, p2, size);
	memcpy(p2, temp, size);
}
void			negate(double *x){*x=-*x;}
double			maximum(double a, double b){return a<b?b:a;}
double			minimum(double a, double b){return a<b?a:b;}

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
	//Point ground[2], em_centroid;
} Photon;
void			free_photon(void *data)
{
	Photon *ph=(Photon*)data;
	array_free(&ph->paths);
}

typedef enum SurfaceTypeEnum
{
	SURF_UNINITIALIZED,
	SURF_HOLE,
	SURF_TRANSP,//default
	SURF_MIRROR,
} SurfaceType;
typedef struct BoundaryStruct
{
	double
		pos,	//the x-distance of surface center from origin
		radius;	//looking LtR: -ve means concave, +ve means convex, zero or +-inf means planar

	//the surface is always divided into {inner circle, outer donut}
	SurfaceType type[2];
	double r_max[2];//only the outer donut can have zero radius difference
	double n_next;//refractive index after the boundary
} Boundary;
typedef struct OpticCompStruct
{
	ArrayHandle name;//string
	int nBounds;//1~5 boundaries,	nMedia = nBounds-1 = 0~4 cemented pieces of glass
	//union
	//{
		int active;
	//	char active_media[4];
	//};
	Boundary info[5];
#if 0
	int active;
	ArrayHandle name;	//string
	double n;			//refractive index
	Surface surfaces[2];//{left, right}		right radius is negated when parsing input
#endif
} OpticComp;//no automatic destructor
//void	free_opticElem(void *data)
//{
//	OpticComp *e1=(OpticComp*)data;
//	array_free(&e1->name);
//}
typedef struct AreaIdxStruct
{
	short e_idx;
	char bound_idx, is_outer_not_inner;
} AreaIdx;


//globals
int				current_elem=0;
ArrayHandle		elements=0, ebackup=0,//arrays of OpticComp
				order=0,//array of AreaIdx
				photons=0;//array of Photon
AreaIdx			initial_target={0};
int				nrays=10,
				in_path_count=0,
				out_path_count=0,
				no_focus=0,
				no_system=0;
double			tan_tilt=0,//rays tilt
				x_sensor=0,//x-coordinate of best sensor position
				y_focus=0,
				r_blur=0;//main loss value
int				track_focus=0;
double			saved_VX=0, saved_VY=0;

double			*history=0;
int				hist_idx=0;

double			VX=0, VY=0, DX=20, AR_Y=1, Xstep, Ystep;
int				prec;
char			timer=0, drag=0, m_bypass=0, clearScreen=0, _2d_drag_graph_not_window=0;


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


void			free_elements(ArrayHandle *arr)
{
	if(*arr)
	{
		for(int ke=0;ke<(int)arr[0]->count;++ke)
		{
			OpticComp *oe=(OpticComp*)array_at(arr, ke);
			array_free(&oe->name);
		}
		array_free(arr);
	}
}
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
int				save_text(const char *filename, const char *src, size_t srcSize)
{
	FILE *f;
	size_t written;

	fopen_s(&f, filename, "w");
	//f=fopen(filename, "w");
	if(!f)
	{
		LOG_ERROR("Cannot save \'%s\'", filename);
		return 0;
	}

	written=fwrite(src, 1, srcSize, f);
	fclose(f);
	return 1;
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
						break;
					}
					if(data==DUPLET('*', '/'))
					{
						*idx+=2;
						break;
					}
					if((data&0xFF)=='\n')
					{
						++*idx;
						++*lineno;
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
		{
			if(text->data[*idx]=='\n')
			{
				++*idx;
				++*lineno;
				*linestart=*idx;
			}
			else
				++*idx;
		}
		else
			break;
	}
	return *idx>=(int)text->count;
}
int				match_kw(ArrayHandle text, int *idx, const char **keywords, int nkw)//returns the index of matched keyword
{
	for(int kk=0;kk<nkw;++kk)
	{
		const char *kw=keywords[kk];
		int k=*idx, k2=0;
		for(;k<(int)text->count;++k, ++k2)
		{
			if(!kw[k2])//match
			{
				*idx=k;
				return kk;
			}
			if(text->data[k]!=kw[k2])
				break;
		}
	}
	return -1;
}
int				get_id(ArrayHandle text, int *idx)//returns true if valid identifier
{
	int valid=isalpha(text->data[*idx])||text->data[*idx]=='_';
	if(!valid)
		return 0;
	do
		++*idx;
	while(isalnum(text->data[*idx])||text->data[*idx]=='_');
	return 1;
}
const char
	kw_elem[]="elem",
	kw_comp[]="comp",
	kw_n[]="n",
	kw_pos[]="pos",
	kw_th[]="th",
	kw_ap[]="ap",
	kw_bound[]="bound",
	kw_medium[]="medium",
	kw_inactive[]="inactive",
	kw_left[]="left",
	kw_right[]="right",
	kw_plane[]="plane",
	kw_light[]="light",
	kw_rays[]="rays",
	kw_path[]="path",
	kw_inner[]="inner",
	kw_outer[]="outer",

	kw_mirror[]="mirror",			//surface types
	kw_transp[]="transp",//default
	kw_hole[]="hole",
	
	//units:
	kw_angestrom[]="A",
	kw_nm[]="nm",//default for wavelength

	kw_mm[]="mm",
	kw_cm[]="cm",//default for distance
	kw_m[]="m",
	kw_inch[]="inch",
	kw_ft[]="ft";
const char
	*ksearch_start[]={kw_elem, kw_comp, kw_light, kw_rays, kw_path},
	*ksearch_attr[]={kw_n, kw_pos, kw_th, kw_ap, kw_inactive},
	*ksearch_attr_comp[]={kw_pos, kw_ap, kw_inactive},
	*ksearch_unit_dist[]={kw_mm, kw_cm, kw_m, kw_inch, kw_ft},
	*ksearch_unit_lambda[]={kw_nm, kw_angestrom},
	*ksearch_surface[]={kw_left, kw_right},
	*ksearch_stype[]={kw_hole, kw_transp, kw_mirror},
	*ksearch_area[]={kw_inner, kw_outer},
	*ksearch_medium[]={kw_th, kw_n};
#define			EXPECT(CH)		if(text->data[*idx]!=(CH)){LOG_ERROR("%s(%d): Expected \'%c\'", filename, *lineno, CH); success=0; goto finish;} ++*idx;
int				parse_number(const char *filename, ArrayHandle text, int *idx, int *lineno, int *linestart, int units_type, double *ret_val)//units_type: 0: no units, 1: cm, 2: nm
{
	double val=0;
	int success=0;
	double sign;

	if(text->data[*idx]=='-')
	{
		++*idx;//skip minus
		sign=-1;
		if(skip_ws(text, idx, lineno, linestart))
		{
			LOG_ERROR("%s(%d): Expected a number", filename, *lineno);
			return 0;
		}
	}
	else
		sign=1;
	for(;*idx<(int)text->count;++*idx)
	{
		unsigned char c=text->data[*idx]-'0';
		if(c>=10)
			break;
		val*=10;
		val+=c;
		success=1;
	}
	if(text->data[*idx]=='.')
	{
		++*idx;//skip point
		double p=1;
		for(;*idx<(int)text->count;++*idx)
		{
			unsigned char c=text->data[*idx]-'0';
			if(c>=10)
				break;
			p*=0.1;
			val+=c*p;
			success=1;
		}
	}
	if(!success)
		return -1;
	int match=0;
	switch(units_type)
	{
	case 0://unit-less, eg: refractive index
		break;
	case 1://distance: [mm, ...m], [inch, ft]
		match=match_kw(text, idx, ksearch_unit_dist, SIZEOF(ksearch_unit_dist));
		switch(match)
		{
		case 0://mm -> cm
			val*=0.1;
			break;
		case 1://cm: default, leave it as is
			break;
		case 2://m -> cm
			val*=100;
			break;
		case 3://inch -> cm
			val*=2.54;
			break;
		case 4://ft -> cm
			val*=30.48;
			break;
		}
		break;
	case 2://wavelength
		match=match_kw(text, idx, ksearch_unit_lambda, SIZEOF(ksearch_unit_lambda));
		switch(match)
		{
		case 0://nm: default, leave it as is
			break;
		case 1://Angestrom -> nm
			val*=0.1;
			break;
		}
		break;
	}
	if(ret_val)
		*ret_val=sign*val;
	return success;
}
//int			parse_boundary(const char *filename, ArrayHandle text, int *idx, int *lineno, int *linestart, OpticComp *e1)
//{
//}
//int			parse_medium(const char *filename, ArrayHandle text, int *idx, int *lineno, int *linestart, OpticComp *e1)
//{
//}
int				parse_comp(const char *filename, ArrayHandle text, int *idx, int *lineno, int *linestart, OpticComp *e1)
{
	int fieldmask=0;//{right, left,  ap, th, pos, n}
	int success=1;
	double ap=0;
	
	e1->active=1;//assume all elements initially active unless specified as 'inactive'
	for(;;)
	{
		if(skip_ws(text, idx, lineno, linestart))
		{
			LOG_ERROR("%s(%d): Expected \'n\', \'pos\', \'th\', or \'ap\'", filename, *lineno);
			success=0;
			break;
		}
		int match=match_kw(text, idx, ksearch_attr_comp, SIZEOF(ksearch_attr_comp));
		if(match==-1)
		{
			LOG_ERROR("%s(%d): Expected \'pos\' or \'ap\'", filename, *lineno);
			success=0;
			break;
		}
		if(skip_ws(text, idx, lineno, linestart))
		{
			LOG_ERROR("%s(%d): Expected assignment", filename, *lineno);
			success=0;
			break;
		}
		EXPECT('=')
		if(skip_ws(text, idx, lineno, linestart))
		{
			LOG_ERROR("%s(%d): Expected assignment", filename, *lineno);
			success=0;
			break;
		}
		switch(match)
		{
		case 0://pos
			{
				double offset=0;
				for(int ke=0;ke<(int)elements->count-1;++ke)
				{
					OpticComp *e2=(OpticComp*)array_at(&elements, ke);
					const char *name=e2->name->data;
					match=match_kw(text, idx, &name, 1);
					if(match!=-1)
					{
						offset=e2->info[e2->nBounds-1].pos;
						if(skip_ws(text, idx, lineno, linestart))
						{
							LOG_ERROR("%s(%d): Expected assignment", filename, *lineno);
							success=0;
							goto finish;
						}
						EXPECT('+')//must add number to a previously declared element position
						if(skip_ws(text, idx, lineno, linestart))
						{
							LOG_ERROR("%s(%d): Expected assignment", filename, *lineno);
							success=0;
							goto finish;
						}
						break;
					}
				}
				if(!parse_number(filename, text, idx, lineno, linestart, 1, &e1->info[0].pos))
				{
					success=0;
					goto finish;
				}
				e1->info[0].pos+=offset;
				fieldmask|=2;
			}
			break;
		case 1://ap: the aperture (diameter), if not specified then both left & right specifications must mention r_max[1]
			if(!parse_number(filename, text, idx, lineno, linestart, 1, &ap))//left.pos is to be added
			{
				success=0;
				goto finish;
			}
			fieldmask|=8;
			break;
		case 2://inactive
			e1->active=0;
			break;
		}
		if(text->data[*idx]==';')
		{
			++*idx;
			break;
		}
		EXPECT(',')
	}

	e1->nBounds=0;
	for(;;)//parse bounds & media
	{
		Boundary *bound=e1->info+e1->nBounds;
		if(skip_ws(text, idx, lineno, linestart))
		{
			LOG_ERROR("%s(%d): Expected \'bound\'", filename, *lineno);
			success=0;
			break;
		}
		const char *kw=kw_bound;
		int match=match_kw(text, idx, &kw, 1);
		if(match==-1)
		{
			if(!e1->nBounds)
				LOG_ERROR("%s(%d): Expected \'bound\'", filename, *lineno);
			break;
		}
		if(skip_ws(text, idx, lineno, linestart))
		{
			LOG_ERROR("%s(%d): Expected assignment", filename, *lineno);
			success=0;
			break;
		}
		EXPECT('=')
		if(skip_ws(text, idx, lineno, linestart))
		{
			LOG_ERROR("%s(%d): Expected assignment", filename, *lineno);
			success=0;
			break;
		}

		kw=kw_plane;
		match=match_kw(text, idx, &kw, 1);
		if(match==-1)//parse normal number
		{
			if(!parse_number(filename, text, idx, lineno, linestart, 1, &bound->radius))//negate only the right radius later
			{
				success=0;
				goto finish;
			}
		}
		else//set radius 0 for plane
			bound->radius=0;
		
		if(skip_ws(text, idx, lineno, linestart))
		{
			LOG_ERROR("%s(%d): Expected \':\' or \';\'", filename, *lineno);
			success=0;
			break;
		}
		if(text->data[*idx]==':')
		{
			++*idx;//skip colon
			if(skip_ws(text, idx, lineno, linestart))
			{
				LOG_ERROR("%s(%d): Expected \'mirror\', \'transp\' or \'hole\'", filename, *lineno);
				success=0;
				break;
			}
			match=match_kw(text, idx, ksearch_stype, SIZEOF(ksearch_stype));
			if(match==-1)
			{
				LOG_ERROR("%s(%d): Expected \'mirror\', \'transp\' or \'hole\'", filename, *lineno);
				success=0;
				break;
			}
			switch(match)
			{
			case 0:bound->type[0]=SURF_HOLE;break;
			case 1:bound->type[0]=SURF_TRANSP;break;
			case 2:bound->type[0]=SURF_MIRROR;break;
			}
			if(skip_ws(text, idx, lineno, linestart))
			{
				LOG_ERROR("%s(%d): Expected \';\' at the end of \'bound %s\' declaration", filename, *lineno, e1->nBounds+1);
				success=0;
				break;
			}
			if((fieldmask&8)==8&&text->data[*idx]==';')
			{
				++*idx;
				continue;
			}
			EXPECT(',')
			if(skip_ws(text, idx, lineno, linestart))
			{
				LOG_ERROR("%s(%d): Expected a number", filename, *lineno);
				success=0;
				break;
			}
			if(!parse_number(filename, text, idx, lineno, linestart, 1, &bound->r_max[0]))
			{
				success=0;
				goto finish;
			}
			if(skip_ws(text, idx, lineno, linestart))
			{
				LOG_ERROR("%s(%d): Expected \',\'");
				success=0;
				break;
			}
			if(text->data[*idx]==';')//one area
			{
				++*idx;
				bound->r_max[1]=bound->r_max[0];
				bound->type[1]=bound->type[0];
				continue;
			}
			EXPECT(',')
			if(skip_ws(text, idx, lineno, linestart))
			{
				LOG_ERROR("%s(%d): Expected \'mirror\', \'transp\' or \'hole\'", filename, *lineno);
				success=0;
				break;
			}
			match=match_kw(text, idx, ksearch_stype, SIZEOF(ksearch_stype));
			if(match==-1)
			{
				LOG_ERROR("%s(%d): Expected \'mirror\', \'transp\' or \'hole\'", filename, *lineno);
				success=0;
				break;
			}
			switch(match)
			{
			case 0:bound->type[1]=SURF_HOLE;break;
			case 1:bound->type[1]=SURF_TRANSP;break;
			case 2:bound->type[1]=SURF_MIRROR;break;
			}
			if(skip_ws(text, idx, lineno, linestart))
			{
				LOG_ERROR("%s(%d): Expected a number", filename, *lineno);
				success=0;
				break;
			}
			if(!(fieldmask&8))//if aperture wasn't declared
			{
				EXPECT(',')
				if(skip_ws(text, idx, lineno, linestart))
				{
					LOG_ERROR("%s(%d): Expected \'mirror\', \'transp\' or \'hole\'", filename, *lineno);
					success=0;
					break;
				}
				if(!parse_number(filename, text, idx, lineno, linestart, 1, &bound->r_max[1]))
				{
					success=0;
					goto finish;
				}
				if(skip_ws(text, idx, lineno, linestart))
				{
					LOG_ERROR("%s(%d): Expected \'mirror\', \'transp\' or \'hole\'", filename, *lineno);
					success=0;
					break;
				}
			}
		}
		EXPECT(';')
		
		++e1->nBounds;

		if(skip_ws(text, idx, lineno, linestart))//parse medium (optional)
		{
			LOG_ERROR("%s(%d): Expected \'bound\'", filename, *lineno);
			success=0;
			break;
		}
		kw=kw_medium;
		match=match_kw(text, idx, &kw, 1);
		if(match==-1)//the only correct way to end the 2nd loop
			break;
		if(e1->nBounds==4)
		{
			LOG_ERROR("%s(%d): 4 media with 5 boundaries max", filename, *lineno);
			break;
		}
		for(int mask2=0;mask2!=3;)//{bit1: n, bit0: th}
		{
			if(skip_ws(text, idx, lineno, linestart))
			{
				LOG_ERROR("%s(%d): Expected assignment", filename, *lineno);
				success=0;
				goto finish;
			}
		
			match=match_kw(text, idx, ksearch_medium, SIZEOF(ksearch_medium));
			if(match==-1)
			{
				LOG_ERROR("%s(%d): Expected \'th\' or \'n\'", filename, *lineno);
				success=0;
				goto finish;
			}
			if(skip_ws(text, idx, lineno, linestart))
			{
				LOG_ERROR("%s(%d): Expected assignment", filename, *lineno);
				success=0;
				goto finish;
			}
			EXPECT('=')
			if(skip_ws(text, idx, lineno, linestart))
			{
				LOG_ERROR("%s(%d): Expected assignment", filename, *lineno);
				success=0;
				goto finish;
			}
			switch(match)
			{
			case 0://th
				if(mask2&1)
				{
					LOG_ERROR("%s(%d): \'th\' was already declared", filename, *lineno);
					success=0;
					goto finish;
				}
				mask2|=1;
				if(!parse_number(filename, text, idx, lineno, linestart, 1, &e1->info[e1->nBounds].pos))
				{
					success=0;
					goto finish;
				}
				e1->info[e1->nBounds].pos+=bound->pos;//thickness to absolute position
				break;
			case 1://n
				if(mask2&2)
				{
					LOG_ERROR("%s(%d): \'n\' was already declared", filename, *lineno);
					success=0;
					goto finish;
				}
				mask2|=2;
				if(!parse_number(filename, text, idx, lineno, linestart, 0, &bound->n_next))
				{
					success=0;
					goto finish;
				}
				break;
			}
			if(skip_ws(text, idx, lineno, linestart))
			{
				LOG_ERROR("%s(%d): Expected assignment", filename, *lineno);
				success=0;
				goto finish;
			}
			if(text->data[*idx]==';')
			{
				++*idx;
				if(mask2!=3)
					LOG_ERROR("%s(%d): Missing medium attribute ", filename, *lineno);
				break;
			}
			EXPECT(',');
		}
	}//end of 2nd loop
	if(e1->nBounds>1)
		negate(&e1->info[e1->nBounds-1].radius);//flip right radius
	if(fieldmask&8)//if aperture was declared, it overrides boundary apertures
	{
		ap*=0.5;
		for(int kb=0;kb<e1->nBounds;++kb)
			e1->info[kb].r_max[1]=ap;
	}
	for(int kb=0;kb<e1->nBounds;++kb)//inner radius can't be zero
		if(!e1->info[kb].r_max[0])//if inner radius is zero, then set it as outer radius
			e1->info[kb].r_max[0]=e1->info[kb].r_max[1];
	for(int kb=0;kb<e1->nBounds;++kb)//set transparent by default
	{
		for(int k2=0;k2<2;++k2)
			if(e1->info[kb].type[k2]==SURF_UNINITIALIZED)
				e1->info[kb].type[k2]=SURF_TRANSP;
	}
	e1->info[e1->nBounds-1].n_next=1;
finish:
	return success;
}
int				parse_elem(const char *filename, ArrayHandle text, int *idx, int *lineno, int *linestart, OpticComp *e1)
{
	int fieldmask=0;//{right, left,  ap, th, pos, n}
	int success=1;
	double ap=0;
	
	e1->active=1;//assume all elements initially active unless specified as 'inactive'
#if 1
	for(;;)
	{
		if(skip_ws(text, idx, lineno, linestart))
		{
			LOG_ERROR("%s(%d): Expected \'n\', \'pos\', \'th\', or \'ap\'", filename, *lineno);
			success=0;
			break;
		}
		int match=match_kw(text, idx, ksearch_attr, SIZEOF(ksearch_attr));
		if(match==-1)
		{
			LOG_ERROR("%s(%d): Expected \'n\', \'pos\', \'th\', or \'ap\'", filename, *lineno);
			success=0;
			break;
		}
		if(skip_ws(text, idx, lineno, linestart))
		{
			LOG_ERROR("%s(%d): Expected assignment", filename, *lineno);
			success=0;
			break;
		}
		EXPECT('=')
		if(skip_ws(text, idx, lineno, linestart))
		{
			LOG_ERROR("%s(%d): Expected assignment", filename, *lineno);
			success=0;
			break;
		}
		switch(match)
		{
		case 0://n: the refractive index
			if(!parse_number(filename, text, idx, lineno, linestart, 0, &e1->info[0].n_next))
			{
				success=0;
				goto finish;
			}
			fieldmask|=1;
			break;
		case 1://pos: left surface x-distance
			{
				double offset=0;
				for(int ke=0;ke<(int)elements->count-1;++ke)
				{
					OpticComp *e2=(OpticComp*)array_at(&elements, ke);
					const char *name=e2->name->data;
					match=match_kw(text, idx, &name, 1);
					if(match!=-1)
					{
						offset=e2->info[e2->nBounds-1].pos;
						if(skip_ws(text, idx, lineno, linestart))
						{
							LOG_ERROR("%s(%d): Expected assignment", filename, *lineno);
							success=0;
							goto finish;
						}
						EXPECT('+')//must add number to a previously declared element position
						if(skip_ws(text, idx, lineno, linestart))
						{
							LOG_ERROR("%s(%d): Expected assignment", filename, *lineno);
							success=0;
							goto finish;
						}
						break;
					}
				}
				if(!parse_number(filename, text, idx, lineno, linestart, 1, &e1->info[0].pos))
				{
					success=0;
					goto finish;
				}
				e1->info[0].pos+=offset;
				fieldmask|=2;
			}
			break;
		case 2://th: the element thickness at center
			if(!parse_number(filename, text, idx, lineno, linestart, 1, &e1->info[1].pos))//left.pos is to be added
			{
				success=0;
				goto finish;
			}
			fieldmask|=4;
			break;
		case 3://ap: the aperture (diameter), if not specified then both left & right specifications must mention r_max[1]
			if(!parse_number(filename, text, idx, lineno, linestart, 1, &ap))//left.pos is to be added
			{
				success=0;
				goto finish;
			}
			fieldmask|=8;
			break;
		case 4://inactive
			e1->active=0;
			break;
		}
		if(text->data[*idx]==';')
		{
			++*idx;
			break;
		}
		EXPECT(',')
	}//end of initial loop
#endif
	
	while((fieldmask&0x30)!=0x30)
	{
		if(skip_ws(text, idx, lineno, linestart))
		{
			LOG_ERROR("%s(%d): Expected \'left\' or \'right\'", filename, *lineno);
			success=0;
			break;
		}
		int match=match_kw(text, idx, ksearch_surface, SIZEOF(ksearch_surface));
		if(match==-1)//not an error
			break;
		if(match)//right surface
		{
			if((fieldmask&0x20)==0x20)
			{
				LOG_ERROR("%s(%d): \'right\' was encountered before", filename, *lineno);
				success=0;
				goto finish;
			}
			fieldmask|=0x20;
		}
		else//left surface
		{
			if((fieldmask&0x10)==0x10)
			{
				LOG_ERROR("%s(%d): \'right\' was encountered before", filename, *lineno);
				success=0;
				goto finish;
			}
			fieldmask|=0x10;
		}
		if(skip_ws(text, idx, lineno, linestart))
		{
			LOG_ERROR("%s(%d): Expected assignment", filename, *lineno);
			success=0;
			break;
		}
		EXPECT('=')
		if(skip_ws(text, idx, lineno, linestart))
		{
			LOG_ERROR("%s(%d): Expected assignment", filename, *lineno);
			success=0;
			break;
		}

		const char *kw2=kw_plane;
		int match2=match_kw(text, idx, &kw2, 1);
		if(match2==-1)//parse normal number
		{
			if(!parse_number(filename, text, idx, lineno, linestart, 1, &e1->info[match].radius))//negate only the right radius later
			{
				success=0;
				goto finish;
			}
		}
		else//set radius 0 for plane
			e1->info[match].radius=0;
		
		if(skip_ws(text, idx, lineno, linestart))
		{
			LOG_ERROR("%s(%d): Expected \':\' or \';\'", filename, *lineno);
			success=0;
			break;
		}
		if(text->data[*idx]==':')
		{
			++*idx;//skip colon
			if(skip_ws(text, idx, lineno, linestart))
			{
				LOG_ERROR("%s(%d): Expected \'mirror\', \'transp\' or \'hole\'", filename, *lineno);
				success=0;
				break;
			}
			match2=match_kw(text, idx, ksearch_stype, SIZEOF(ksearch_stype));
			if(match2==-1)
			{
				LOG_ERROR("%s(%d): Expected \'mirror\', \'transp\' or \'hole\'", filename, *lineno);
				success=0;
				break;
			}
			switch(match2)
			{
			case 0:e1->info[match].type[0]=SURF_HOLE;break;
			case 1:e1->info[match].type[0]=SURF_TRANSP;break;
			case 2:e1->info[match].type[0]=SURF_MIRROR;break;
			}
			if(skip_ws(text, idx, lineno, linestart))
			{
				LOG_ERROR("%s(%d): Expected \';\' at the end of \'%s\' declaration", filename, *lineno, match?"right":"left");
				success=0;
				break;
			}
			if((fieldmask&8)==8&&text->data[*idx]==';')
			{
				++*idx;
				continue;
			}
			EXPECT(',')
			if(skip_ws(text, idx, lineno, linestart))
			{
				LOG_ERROR("%s(%d): Expected a number", filename, *lineno);
				success=0;
				break;
			}
			if(!parse_number(filename, text, idx, lineno, linestart, 1, &e1->info[match].r_max[0]))
			{
				success=0;
				goto finish;
			}
			if(skip_ws(text, idx, lineno, linestart))
			{
				LOG_ERROR("%s(%d): Expected \',\'");
				success=0;
				break;
			}
			if(text->data[*idx]==';')//one area
			{
				++*idx;
				e1->info[match].r_max[1]=e1->info[match].r_max[0];
				e1->info[match].type[1]=e1->info[match].type[0];
				continue;
			}
			EXPECT(',')
			if(skip_ws(text, idx, lineno, linestart))
			{
				LOG_ERROR("%s(%d): Expected \'mirror\', \'transp\' or \'hole\'", filename, *lineno);
				success=0;
				break;
			}
			match2=match_kw(text, idx, ksearch_stype, SIZEOF(ksearch_stype));
			if(match2==-1)
			{
				LOG_ERROR("%s(%d): Expected \'mirror\', \'transp\' or \'hole\'", filename, *lineno);
				success=0;
				break;
			}
			switch(match2)
			{
			case 0:e1->info[match].type[1]=SURF_HOLE;break;
			case 1:e1->info[match].type[1]=SURF_TRANSP;break;
			case 2:e1->info[match].type[1]=SURF_MIRROR;break;
			}
			if(skip_ws(text, idx, lineno, linestart))
			{
				LOG_ERROR("%s(%d): Expected a number", filename, *lineno);
				success=0;
				break;
			}
			if(!(fieldmask&8))//if aperture wasn't declared
			{
				EXPECT(',')
				if(skip_ws(text, idx, lineno, linestart))
				{
					LOG_ERROR("%s(%d): Expected \'mirror\', \'transp\' or \'hole\'", filename, *lineno);
					success=0;
					break;
				}
				if(!parse_number(filename, text, idx, lineno, linestart, 1, &e1->info[match].r_max[1]))
				{
					success=0;
					goto finish;
				}
				if(skip_ws(text, idx, lineno, linestart))
				{
					LOG_ERROR("%s(%d): Expected \'mirror\', \'transp\' or \'hole\'", filename, *lineno);
					success=0;
					break;
				}
			}
		}
		EXPECT(';')
	}//end of 2nd loop
	e1->nBounds=2;
	e1->info[1].pos+=e1->info[0].pos;//add left position to thickness
	//if(e1->info[1].radius)
		e1->info[1].radius=-e1->info[1].radius;//flip right radius
	if(fieldmask&8)//if aperture was declared, it overrides left & right declarations
	{
		ap*=0.5;
		e1->info[0].r_max[1]=ap;
		e1->info[1].r_max[1]=ap;
	}
	if(!e1->info[0].r_max[0])//if inner radius is zero, then set it as outer radius
		e1->info[0].r_max[0]=e1->info[0].r_max[1];
	if(!e1->info[1].r_max[0])
		e1->info[1].r_max[0]=e1->info[1].r_max[1];
	for(int kb=0;kb<2;++kb)//set transparent by default
	{
		for(int k2=0;k2<2;++k2)
			if(e1->info[kb].type[k2]==SURF_UNINITIALIZED)
				e1->info[kb].type[k2]=SURF_TRANSP;
	}
	e1->info[e1->nBounds-1].n_next=1;
finish:
	return success;
}
#undef			EXPECT
#define			EXPECT(CH)		if(text->data[idx]!=(CH)){LOG_ERROR("%s(%d): Expected \'%c\'", filename, lineno, CH); success=0; goto finish;} ++idx;
void			init_system()
{
	array_clear(&elements);
	ARRAY_ALLOC(OpticComp, elements, 0, 0, 0, 0);
	
	array_clear(&photons);
	ARRAY_ALLOC(Photon, photons, 0, 0, 0, free_photon);

	array_clear(&order);
	ARRAY_ALLOC(AreaIdx, order, 0, 0, 0, 0);
}
int				open_system(const char *filename)
{
	ArrayHandle text;
	int success=1;
	
	if(!filename)
	{
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
		g_buf[0]=0;
		success=GetOpenFileNameA(&ofn);
		if(!success)
			return 0;
		filename=ofn.lpstrFile;
	}

	text=load_text(filename, 16);
	if(!text)
		return 0;

	init_system();

	for(int idx=0, lineno=0, linestart=0;;)
	{
		if(skip_ws(text, &idx, &lineno, &linestart))
			break;

		int match=match_kw(text, &idx, ksearch_start, SIZEOF(ksearch_start));
		if(match==-1)
		{
			LOG_ERROR("%s(%d): Expected \'elem\', \'light\', \'rays\', or \'path\'", filename, lineno);
			success=0;
			break;
		}
		if(skip_ws(text, &idx, &lineno, &linestart))
		{
			LOG_ERROR("%s(%d): Expected a declaration", filename, lineno);
			success=0;
			break;
		}
		switch(match)
		{
		case 0://elem
		case 1://comp
			{
				OpticComp *e1=(OpticComp*)ARRAY_APPEND(elements, 0, 1, 1, 0);
				int start=idx;
				get_id(text, &idx);
				if(start>=idx)
				{
					LOG_ERROR("%s(%d): Element %d has no name", filename, lineno, (int)elements->count-1);
					success=0;
					goto finish;
				}
				STR_COPY(e1->name, text->data+start, idx-start);
				for(int ke=0;ke<(int)elements->count-1;++ke)//check for name uniqueness
				{
					OpticComp *e2=(OpticComp*)array_at(&elements, ke);
					if(!strcmp(e2->name->data, e1->name->data))
					{
						LOG_ERROR("%s(%d): Element \'%s\' appeared twice at %d and %d", filename, lineno, e1->name->data, ke, (int)elements->count-1);
						success=0;
						goto finish;
					}
				}
				if(skip_ws(text, &idx, &lineno, &linestart))
				{
					success=0;
					goto finish;
				}
				EXPECT(',')
				if(skip_ws(text, &idx, &lineno, &linestart))
				{
					success=0;
					goto finish;
				}
				if(match)
					parse_comp(filename, text, &idx, &lineno, &linestart, e1);
				else
					parse_elem(filename, text, &idx, &lineno, &linestart, e1);
			}
			break;
		case 2://light
			{
				EXPECT(':')
				array_clear(&photons);
				for(;;)
				{
					if(skip_ws(text, &idx, &lineno, &linestart))
					{
						LOG_ERROR("%s(%d): Expected a number", filename, lineno);
						success=0;
						goto finish;
					}
					Photon *ph=(Photon*)ARRAY_APPEND(photons, 0, 1, 1, 0);
					success=parse_number(filename, text, &idx, &lineno, &linestart, 2, &ph->lambda);
					ph->color=wavelength2rgb(ph->lambda);
					if(skip_ws(text, &idx, &lineno, &linestart))
					{
						LOG_ERROR("%s(%d): Expected a number", filename, lineno);
						success=0;
						goto finish;
					}
					if(text->data[idx]==';')
					{
						++idx;
						break;
					}
					EXPECT(',');
				}
			}
			break;
		case 3://rays
			{
				double f_nrays=0;
				ArrayHandle name;
				EXPECT(':')
				if(skip_ws(text, &idx, &lineno, &linestart))
				{
					LOG_ERROR("%s(%d): Expected a number", filename, lineno);
					success=0;
					goto finish;
				}
				success=parse_number(filename, text, &idx, &lineno, &linestart, 0, &f_nrays);
				if(!success)
					goto finish;
				nrays=(int)f_nrays<<1;
				if(skip_ws(text, &idx, &lineno, &linestart))
				{
					LOG_ERROR("%s(%d): Expected a number", filename, lineno);
					success=0;
					goto finish;
				}
				EXPECT(',')
				if(skip_ws(text, &idx, &lineno, &linestart))
				{
					LOG_ERROR("%s(%d): Expected a number", filename, lineno);
					success=0;
					goto finish;
				}
				int start=idx;
				get_id(text, &idx);
				if(start>=idx)
				{
					LOG_ERROR("%s(%d): Expected an element name", filename, lineno);
					success=0;
					goto finish;
				}
				STR_COPY(name, text->data+start, idx-start);
				for(int ke=0;ke<(int)elements->count;++ke)
				{
					OpticComp *e1=(OpticComp*)array_at(&elements, ke);
					if(!strcmp(name->data, e1->name->data))
					{
						initial_target.e_idx=ke;
						break;
					}
				}
				array_free(&name);

				if(skip_ws(text, &idx, &lineno, &linestart))
				{
					LOG_ERROR("%s(%d): Expected \'.\'", filename, lineno);
					success=0;
					goto finish;
				}
				EXPECT('.')
				if(skip_ws(text, &idx, &lineno, &linestart))
				{
					LOG_ERROR("%s(%d): Expected \'left\', \'right\' or a boundary number", filename, lineno);
					success=0;
					goto finish;
				}
				match=match_kw(text, &idx, ksearch_surface, SIZEOF(ksearch_surface));
				if(match!=-1)
					initial_target.bound_idx=match;
				else
				{
					initial_target.bound_idx=(text->data[idx]&0xDF)-'A';
					++idx;
				}
				if(skip_ws(text, &idx, &lineno, &linestart))
				{
					LOG_ERROR("%s(%d): Expected \'.\' or \';\'", filename, lineno);
					success=0;
					goto finish;
				}
				if(text->data[idx]==';')
				{
					initial_target.is_outer_not_inner=0;
					++idx;
					break;
				}
				EXPECT('.')
				if(skip_ws(text, &idx, &lineno, &linestart))
				{
					LOG_ERROR("%s(%d): Expected \'inner\', or \'outer\'", filename, lineno);
					success=0;
					goto finish;
				}
				match=match_kw(text, &idx, ksearch_area, SIZEOF(ksearch_area));
				if(match==-1)
				{
					LOG_ERROR("%s(%d): Expected \'inner\', or \'outer\'", filename, lineno);
					success=0;
					break;
				}
				initial_target.is_outer_not_inner=match;
				if(skip_ws(text, &idx, &lineno, &linestart))
				{
					LOG_ERROR("%s(%d): Expected \';\'", filename, lineno);
					success=0;
					goto finish;
				}
				EXPECT(';')
			}
			break;
		case 4://path
			{
				EXPECT(':')
				for(;;)
				{
					ArrayHandle name;
					if(skip_ws(text, &idx, &lineno, &linestart))
					{
						LOG_ERROR("%s(%d): Expected a number", filename, lineno);
						success=0;
						goto finish;
					}
					int start=idx;
					get_id(text, &idx);
					if(start>=idx)
					{
						LOG_ERROR("%s(%d): Expected an element name", filename, lineno);
						success=0;
						goto finish;
					}
					AreaIdx *aidx=(AreaIdx*)ARRAY_APPEND(order, 0, 1, 1, 0);
					STR_COPY(name, text->data+start, idx-start);
					for(int ke=0;ke<(int)elements->count;++ke)
					{
						OpticComp *e1=(OpticComp*)array_at(&elements, ke);
						if(!strcmp(name->data, e1->name->data))
						{
							aidx->e_idx=ke;
							break;
						}
					}
					array_free(&name);
					if(skip_ws(text, &idx, &lineno, &linestart))
					{
						LOG_ERROR("%s(%d): Expected \'.\'", filename, lineno);
						success=0;
						goto finish;
					}
					EXPECT('.')

					if(skip_ws(text, &idx, &lineno, &linestart))
					{
						LOG_ERROR("%s(%d): Expected \'left\', \'right\' or a boundary number", filename, lineno);
						success=0;
						goto finish;
					}
					match=match_kw(text, &idx, ksearch_surface, SIZEOF(ksearch_surface));
					if(match!=-1)
						aidx->bound_idx=match;
					else
					{
						aidx->bound_idx=(text->data[idx]&0xDF)-'A';
						++idx;
					}

					if(skip_ws(text, &idx, &lineno, &linestart))
					{
						LOG_ERROR("%s(%d): Expected \'.\' or \';\'", filename, lineno);
						success=0;
						goto finish;
					}
					if(text->data[idx]==','||text->data[idx]==';')//default is outer
					{
						int end=text->data[idx]==';';
						++idx;//skip comma/semicolon
						aidx->is_outer_not_inner=0;//default is inner area
						if(end)
							break;
						continue;
					}
					EXPECT('.')

					if(skip_ws(text, &idx, &lineno, &linestart))
					{
						LOG_ERROR("%s(%d): Expected \'inner\', or \'outer\'", filename, lineno);
						success=0;
						goto finish;
					}
					
					match=match_kw(text, &idx, ksearch_area, SIZEOF(ksearch_area));
					if(match==-1)
					{
						LOG_ERROR("%s(%d): Expected \'inner\', or \'outer\'", filename, lineno);
						success=0;
						break;
					}
					aidx->is_outer_not_inner=match;
					
					if(skip_ws(text, &idx, &lineno, &linestart))
					{
						LOG_ERROR("%s(%d): Expected \',\' or \';\'", filename, lineno);
						success=0;
						goto finish;
					}
					if(text->data[idx]==';')
					{
						++idx;//skip semicolon
						break;
					}
					EXPECT(',')
				}
			}
			break;
		}
	}
	if(success)
	{
		free_elements(&ebackup);
		ebackup=array_copy(&elements);
	}
	else
	{
		free_elements(&elements);
		elements=array_copy(&ebackup);
	}
finish:
	array_free(&text);
	SetWindowTextA(ghWnd, filename);
	return success;
}

const char		comment[]=
	"/*\n"
	"2022-08-08Mo\n"
	"KEYWORDS\n"
	"comp <name>:            Glass component declaration\n"
	"elem <name>:            2-sided glass element declaration (deprecated)\n"
	"inactive:               Makes component initially inactive\n"
	"ap:                     Aperture (diameter) (optional)\n"
	"n:                      Refractive index\n"
	"pos:                    x-distance from origin\n"
	"th:                     Thickness\n"
	"left, right:            Surface specifications\n"
	"mirror/transp/hole:     Surface type (transp is default)\n"
	"\n"
	"light:  Wavelength list in nanometers (nm)\n"
	"\n"
	"rays:   Specify the incident ray count\n"
	"        and initial target area (not necessarily the first boundary)\n"
	"\n"
	"path:   Describe the correct light path through the optical device\n"
	"\n"
	"UNITS\n"
	"dimensions:     mm, cm (default), m, inch, ft\n"
	"wavelength:     A (Angestrom), nm (default)\n"
	"*/\n";
void			print2str(ArrayHandle *str, const char *format, ...)
{
	va_list args;
	int printed;
	va_start(args, format);
	printed=vsprintf_s(g_buf, G_BUF_SIZE, format, args);
	va_end(args);
	STR_APPEND(*str, g_buf, printed, 1);
}
#if 0
void			print2str_surface(ArrayHandle *str, Boundary *s, int bound_idx)
{
	const char *name;
	double radius;
	const char *stypes[]={"hole", "transp", "mirror"};

	if(bound_idx)
		name="right", radius=-s->radius;
	else
		name="left", radius=s->radius;
	print2str(str, "\t%s\t=%gcm: %s, %gcm, %s, %gcm;\n", name, radius, stypes[s->type[0]-1], s->r_max[0], stypes[s->type[1]-1], s->r_max[1]);
}
#endif
void			print2str_aidx(ArrayHandle *str, AreaIdx *aidx)
{
	OpticComp *oe=(OpticComp*)array_at(&elements, aidx->e_idx);
	print2str(str, "%s.%c.%s", oe->name->data, 'a'+aidx->bound_idx, aidx->is_outer_not_inner?"outer":"inner");
}
int				save_system(const char *filename)
{
	ArrayHandle fn2, text;
	int success=1;

	if(!filename)
	{
		const char filetitle[]="Untitled.txt";
		OPENFILENAMEA ofn=
		{
			sizeof(OPENFILENAMEA),
			ghWnd, ghInstance,
			"Text Files (.txt)\0*.txt\0",//filter
			0, 0,//custom filter
			1,//filter index: 0 is custom filter, 1 is first, ...
					
			g_buf, G_BUF_SIZE,//file (output)
					
			0, 0,//file title
			0, 0, OFN_ENABLESIZING|OFN_EXPLORER|OFN_NOTESTFILECREATE|OFN_PATHMUSTEXIST|OFN_EXTENSIONDIFFERENT|OFN_OVERWRITEPROMPT,//flags

			0,//nFileOffset
			8,//nFileExtension
			"txt",//default extension
					
			0, 0, 0
		};
		memcpy(g_buf, filetitle, sizeof(filetitle));
		success=GetSaveFileNameA(&ofn);
		if(!success)
			return 0;
		filename=ofn.lpstrFile;
	}

	STR_COPY(fn2, filename, strlen(filename));//filename == g_buf is modified
	STR_COPY(text, comment, sizeof(comment)-1);
	
	time_t t_now=time(0);
#ifdef _MSC_VER
	struct tm t_formatted={0};
	int error=localtime_s(&t_formatted, &t_now);
	struct tm *ts=&t_formatted;
#else
	struct tm *ts=localtime(&t_now);
#endif
	const char *weekdays[]={"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};
	print2str(&text, "\n//%d-%02d-%02d%s %02d%02d%02d%s\n\n",
		1900+ts->tm_year,
		1+ts->tm_mon,
		ts->tm_mday,
		weekdays[ts->tm_wday],
		ts->tm_hour%12,
		ts->tm_min,
		ts->tm_sec,
		ts->tm_hour<12?"AM":"PM");
	
	const char *stypes[]={"hole", "transp", "mirror"};
	for(int ke=0;ke<(int)elements->count;++ke)
	{
		OpticComp *oe=(OpticComp*)array_at(&elements, ke);
		if(oe->nBounds==2)//'elem' is deprecated
		{
			print2str(&text, "elem\t%s, pos=%gcm, th=%gcm, n=%g;\n", (char*)oe->name->data, oe->info[0].pos, oe->info[1].pos-oe->info[0].pos, oe->info[0].n_next);
			for(int kb=0;kb<2;++kb)
			{
				const char *name;
				double radius;
				Boundary const *b=oe->info+kb;
				if(kb)
					name="right", radius=-b->radius;
				else
					name="left", radius=b->radius;
				print2str(&text, "\t%s\t=%gcm: %s, %gcm, %s, %gcm;\n", name, radius, stypes[b->type[0]-1], b->r_max[0], stypes[b->type[1]-1], b->r_max[1]);
			}
			//print2str_surface(&text, oe->info, 0);
			//print2str_surface(&text, oe->info+1, 1);
		}
		else//'comp'
		{
			print2str(&text, "comp\t%s, pos=%gcm;\n", (char*)oe->name->data, oe->info[0].pos);
			for(int kb=0;kb<oe->nBounds;++kb)
			{
				Boundary const *b=oe->info+kb;

				print2str(&text, "\tbound\t=%gcm: %s, %gcm, %s, %gcm;\n",
					kb<oe->nBounds-1?b->radius:-b->radius,
					stypes[b->type[0]-1],
					b->r_max[0],
					stypes[b->type[1]-1],
					b->r_max[1]);

				if(kb+1<oe->nBounds)
				{
					Boundary const *b2=oe->info+kb+1;
					print2str(&text, "\tmedium\tth=%gcm, n=%g;\n", b2->pos-b->pos, b->n_next);
				}
			}
		}
		print2str(&text, "\n");
	}

	print2str(&text, "light:\t");
	for(int kp=0;kp<(int)photons->count;++kp)
	{
		Photon *photon=(Photon*)array_at(&photons, kp);
		print2str(&text, "%gnm", photon->lambda);
		if(kp+1<(int)photons->count)
			print2str(&text, ", ");
	}
	print2str(&text, ";\n");
	
	print2str(&text, "rays:\t%d, ", nrays>>1);
	print2str_aidx(&text, &initial_target);
	print2str(&text, ";\n");
	
	print2str(&text, "\npath:\n");
	for(int ko=0;ko<(int)order->count;++ko)
	{
		AreaIdx *aidx=(AreaIdx*)array_at(&order, ko);
		print2str(&text, "\t");
		print2str_aidx(&text, aidx);
		if(ko+1<(int)order->count)
			print2str(&text, ",\n");
		else
			print2str(&text, ";\n");
	}

	success=save_text(fn2->data, text->data, text->count);
	if(success)
		SetWindowTextA(ghWnd, fn2->data);
	array_free(&fn2);
	array_free(&text);
	return 1;
}

int		sgn_star(double x){return 1-((x<0)<<1);}//assumes zero is positive
int		sgn_int(int n){return (n>0)-(n<0);}
void	normalize(double dx, double dy, Point *ret)
{
	double invhyp=1/sqrt(dx*dx+dy*dy);
	ret->x=dx*invhyp;
	ret->y=dy*invhyp;
}
int		intersect_line_circle(double x1, double y1, double x2, double y2, double cx, double radius, Point *ret_points)//https://mathworld.wolfram.com/Circle-LineIntersection.html
{
	double dx=x2-x1, dy=y2-y1, dr=sqrt(dx*dx+dy*dy);
	if(dr)
	{
		x1-=cx, x2-=cx; //y1-=cy, y2-=cy;//cy==0
		double D=x1*y2-x2*y1;
		double dr2=dr*dr;
		double disc=radius*radius*dr2-D*D;
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

	if(R!=R||!R||fabs(R)==_HUGE)//vertical plane at x
	{
		if(x1==x2)
			return 0;
		hit=1;
		ret_point->x=x;
		ret_point->y=y1+(y2-y1)/(x2-x1)*(x-x1);
	}
	else
	{
		double xcenter=x+R;
		hit=intersect_line_circle(x1, y1, x2, y2, xcenter, R, sol);
		if(hit)
		{
			int second=(R>0)!=(sol[0].x<sol[1].x);//convex from left XNOR sol0 is leftmost
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
int		collinear(Point const *a, Point const *b, Point const *c)
{
	double dx=b->x-a->x, dy=b->y-a->y;
	double disc=dx*dx+dy*dy;
	if(!disc)
		return 1;
	double dist=(dx*(a->y-c->y)-dy*(a->x-c->x))/sqrt(disc);
	return fabs(dist)<1e-5;
}

int		calc_interaction(SurfaceType type, int active, double n_before, double n_after, double cos_in, double sin_in, double *cos_em, double *sin_em)
{
	switch(type)
	{
	case SURF_HOLE://extend
		*cos_em=cos_in;
		*sin_em=sin_in;
		return 1;
	case SURF_TRANSP:
		if(active)//refract
		{
			double temp=n_before*sin_in;
			if(fabs(temp)>n_after)//total internal reflection
			{
				*sin_em=sin_in;
				*cos_em=-cos_in;
				//*cos_em=-sqrt(1-*sin_em**sin_em);
			}
			else
			{
				*sin_em=temp/n_after;
				*cos_em=cos_in;
				//*cos_em=sqrt(1-*sin_em**sin_em);
			}
		}
		else//extend
		{
			*cos_em=cos_in;
			*sin_em=sin_in;
			return 1;
		}
		break;
	case SURF_MIRROR://reflect
		*cos_em=-cos_in;
		*sin_em=sin_in;
		break;
	default://unreachable
		LOG_ERROR("Invalid surface type %d", type);
		*cos_em=cos_in;
		*sin_em=sin_in;
		break;
	}
	return 0;
}
int		hit_ray_surface(double x1, double y1, double x2, double y2, double ap_min, double ap_max, double n_left, double n_right, double x, double R, SurfaceType type, int active, Point *ret_line)//(x1, y1) is in present
{
	int hit;
	Point p[2];
	double sin_in, cos_in, sin_em, cos_em;

	hit=intersect_ray_surface(x1, y1, x2, y2, ap_min, ap_max, x, R, ret_line);
	if(!hit)
		return 0;

	if(!R||fabs(R)==_HUGE)//plane at x
	{
		normalize(x2-x1, y2-y1, p);
		cos_in=p->x;
		sin_in=p->y;
		if(cos_in<0)
			hit+=calc_interaction(type, active, n_right, n_left, cos_in, sin_in, &cos_em, &sin_em);
		else
			hit+=calc_interaction(type, active, n_left, n_right, cos_in, sin_in, &cos_em, &sin_em);
		ret_line[1].x=ret_line[0].x+cos_em;
		ret_line[1].y=ret_line[0].y+sin_em;
	}
	else//+/- spherical at x
	{
		double xcenter=x+R;//left surface convention

		//int inc_dir=sgn_star(x2-x1);
		normalize(x2-x1, y2-y1, p);//p[0] is the incidence unit vector
		normalize(sgn_star(R)*(xcenter-ret_line[0].x), -sgn_star(R)*ret_line[0].y, p+1);//p[1] is the surface normal unit vector = normalize(sgn(radius)*(center-hit))
		cos_in=p[0].x*p[1].x+p[0].y*p[1].y;//cosine of angle between two unit vectors is the dot product
		sin_in=p[0].y*p[1].x-p[0].x*p[1].y;//sine of angle between two unit vectors is the cross product
		if(cos_in<0)
			hit+=calc_interaction(type, active, n_right, n_left, cos_in, sin_in, &cos_em, &sin_em);
		else
			hit+=calc_interaction(type, active, n_left, n_right, cos_in, sin_in, &cos_em, &sin_em);
		double
			c2=cos_em*p[1].x-sin_em*p[1].y,//cos(em+n)=cos(em)*cos(n)-sin(em)*sin(n)
			s2=sin_em*p[1].x+cos_em*p[1].y;//sin(em+n)=sin(em)*cos(n)+cos(em)*sin(n)

		//if(hit!=2&&inc_dir!=sgn_star(c2))//what is this?
		//	c2=-c2, s2=-s2;
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

typedef struct SurfaceParamsStruct
{
	SurfaceType type;
	double
		pos,
		radius,
		ap_min, ap_max,
		nL, nR;
} SurfaceParams;
void	get_params(OpticComp const *e1, AreaIdx const *aidx, double lambda, SurfaceParams *params)
{
	Boundary const *surface=e1->info+aidx->bound_idx;
	params->type=surface->type[aidx->is_outer_not_inner];
	params->pos=surface->pos;
	params->radius=surface->radius;
	params->ap_min=aidx->is_outer_not_inner?surface->r_max[0]:0;
	params->ap_max=surface->r_max[aidx->is_outer_not_inner];

	params->nL=aidx->bound_idx?lambda2n(e1->info[aidx->bound_idx-1].n_next, lambda):1;
	params->nR=aidx->bound_idx<e1->nBounds-1?lambda2n(surface->n_next, lambda):1;
	//double n=lambda2n(surface->n_next, lambda);
	//if(!aidx->bound_idx)
	//	params->nL=1, params->nR=n;
	//else if(aidx->bound_idx==e1->nBounds-1)
	//	params->nL=n, params->nR=1;
}
void	simulate(int user)//number of rays must be even, always double-sided
{
	ArrayHandle intersections;

	if(!elements||!elements->count)
	{
		no_system=1;
		//LOG_ERROR("Elements array is empty");//optical device unknown
		return;
	}
	if(!photons||!photons->count)
	{
		no_system=1;
		//LOG_ERROR("Photon array is empty");//colors unknown
		return;
	}
	if(!order||!order->count)
	{
		no_system=1;
		//LOG_ERROR("Order path array is empty");//device operation unknown
		return;
	}
	no_system=0;
	
	//naive collision detection
#if 0
	for(int ke=0;ke<elements->count;++ke)
	{
		OpticComp *e1=(OpticComp*)array_at(&elements, ke);
		for(int ke2=0;ke2<elements->count;++ke2)
		{
			OpticComp *e2=(OpticComp*)array_at(&elements, ke2);
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
	OpticComp *e1=(OpticComp*)array_at(&elements, e1_idx);
	for(int ke=1;ke<elements->count;++ke)
	{
		OpticComp *e2=(OpticComp*)array_at(&elements, ke);
		if(e1->left.pos>e2->left.pos)//naive left surface center comparison
		{
			e1_idx=ke;
			e1=(OpticComp*)array_at(&elements, e1_idx);
		}
	}
#endif
	
	//find rays range (from the target)
	double ymin, ymax;
	OpticComp *e1=(OpticComp*)array_at(&elements, initial_target.e_idx);
	Boundary *bound=e1->info+initial_target.bound_idx;
	if(initial_target.is_outer_not_inner)//is outer area
	{
		ymin=bound->r_max[0];
		ymax=bound->r_max[1];
	}
	else//is inner area
	{
		ymin=0;
		ymax=bound->r_max[0];
	}

	//find x range of optical device
	double xmin, xmax;
	e1=(OpticComp*)array_at(&elements, 0);
	xmin=e1->info[0].pos;
	xmax=e1->info[e1->nBounds-1].pos;
	for(int ke=1;ke<(int)elements->count;++ke)
	{
		e1=(OpticComp*)array_at(&elements, ke);
		if(xmin>e1->info[0].pos)
			xmin=e1->info[0].pos;
		if(xmax<e1->info[e1->nBounds-1].pos)
			xmax=e1->info[e1->nBounds-1].pos;
	}
	double xlength=xmax-xmin, xstart=xmin-10*xlength;

	//initialize incident rays
	AreaIdx *aidx=(AreaIdx*)array_at(&order, 0);
	e1=(OpticComp*)array_at(&elements, aidx->e_idx);
	bound=e1->info+aidx->bound_idx;
	for(int kp=0;kp<(int)photons->count;++kp)
	{
		Photon *ph=(Photon*)array_at(&photons, kp);
		array_clear(&ph->paths);
		ARRAY_ALLOC(Path, ph->paths, 0, nrays, 0, free_path);
		for(int kp2=0;kp2<(int)ph->paths->count;++kp2)//for each ray	path->count must be even
		{
			Path *path=(Path*)array_at(&ph->paths, kp2);
			ARRAY_ALLOC(Point, path->points, 0, 0, 2, 0);
			Point *points=ARRAY_APPEND(path->points, 0, 2, 1, 0);
			points->x=xstart;
			points->y=ymin+((kp2>>1)+1)*(ymax-ymin)/((ph->paths->count>>1)+1);
			//points->y=ymin+((kp2>>1)+1)*(ymax-ymin)/(ph->paths->count>>1);//X  top ray doesn't emerge
			if(kp2&1)//rays are in pairs mirrored by ground line, odd rays are below the even rays
				points->y=-points->y;
			path->emerged=intersect_ray_surface(xstart, points->y, xstart+xlength, points->y, ymin, ymax, bound->pos, bound->radius, points+1);//must hit
			points[0].y-=tan_tilt*(points[1].x-points[0].x);
		}
	}


	//main simulation loop
	for(int ko=0;ko<(int)order->count;++ko)//for each areaIdx in 'order' array
	{
		aidx=(AreaIdx*)array_at(&order, ko);
		//if(aidx->e_idx==1&&aidx->bound_idx==1)//
		//	aidx=aidx;
		e1=(OpticComp*)array_at(&elements, aidx->e_idx);//fetch next element
		for(int kp=0;kp<(int)photons->count;++kp)
		{
			Photon *ph=(Photon*)array_at(&photons, kp);
			for(int kp2=0;kp2<(int)ph->paths->count;++kp2)//for each ray
			{
				Path *path;
				Point *in_ray;
				Point em_ray[2];
				SurfaceParams params;

				path=(Path*)array_at(&ph->paths, kp2);
				if(!path->emerged)
					continue;
				in_ray=(Point*)array_at(&path->points, path->points->count-2);
				get_params(e1, aidx, ph->lambda, &params);
				int hit=hit_ray_surface(in_ray[0].x, in_ray[0].y, in_ray[1].x, in_ray[1].y, params.ap_min, params.ap_max, params.nL, params.nR, params.pos, params.radius, params.type, e1->active, em_ray);
				if(hit==2)//extend same ray later
					continue;
				if(hit)
				{
					in_ray[1]=em_ray[0];
					ARRAY_APPEND(path->points, em_ray+1, 1, 1, 0);
				}
				else
					path->emerged=0;
			}
		}
	}


	ARRAY_ALLOC(Point, intersections, 0, 0, 0, 0);
	
	no_focus=0;
	if(tan_tilt==0)//update sensor position
	{
		//calculate intersections of emergent ray pairs
		for(int kp=0;kp<(int)photons->count;++kp)
		{
			Photon *ph=(Photon*)array_at(&photons, kp);
			for(int kp2=0;kp2<(int)ph->paths->count;kp2+=2)//for each symmetric ray couple
			{
				Path
					*path1=(Path*)array_at(&ph->paths, kp2),
					*path2=(Path*)array_at(&ph->paths, kp2+1);
				if(!path1->emerged||!path2->emerged)
					continue;
				Point
					*line1=(Point*)array_at(&path1->points, path1->points->count-2),
					*line2=(Point*)array_at(&path2->points, path2->points->count-2);

				double
					dy1=fabs(line1[0].y-line2[0].y),
					dy2=fabs(line1[1].y-line2[1].y);
				if(dy1<dy2)//skip if divergent
					continue;
				Point *cross=(Point*)ARRAY_APPEND(intersections, 0, 1, 1, 0);
				intersect_lines(line1, line2, cross);
			}
		}

		if(!intersections->count)
			x_sensor=100, no_focus=1;
		else
		{
			//calculate mean & standard deviation of symmetric intersections
			meanvar((double*)intersections->data, (int)intersections->count, 2, &x_sensor, 0);
			array_clear(&intersections);
		}
	}

	//extend emerging rays to reach sensor
	for(int kp=0;kp<(int)photons->count;++kp)
	{
		Photon *ph=(Photon*)array_at(&photons, kp);
		for(int kp2=0;kp2<(int)ph->paths->count;++kp2)//for each ray
		{
			Path *path=(Path*)array_at(&ph->paths, kp2);
			if(!path->emerged)
				continue;
			Point *line=(Point*)array_at(&path->points, path->points->count-2);

			double dx=line[1].x-line[0].x;
			if(!dx||(x_sensor-line[0].x<0)!=(dx<0))//don't extend ray to sensor if vertical or the sensor is behind
				continue;
			line[1].x=x_sensor+10*sgn_star(line[1].x-line[0].x);
			line[1].y=line[0].y+(line[1].x-line[0].x)*(line[1].y-line[0].y)/dx;

			//line[1].x=line[0].x+100*(line[1].x-line[0].x);
			//line[1].y=line[0].y+100*(line[1].y-line[0].y);
		}
	}
	
	//calculate intersections of emergent rays with sensor
	Point sensorPlane[2]=
	{
		{x_sensor, -10},
		{x_sensor, 10},
	};
	in_path_count=nrays*(int)photons->count;
	out_path_count=0;
	for(int kp=0;kp<(int)photons->count;++kp)
	{
		Photon *ph=(Photon*)array_at(&photons, kp);
		for(int kp2=0;kp2<(int)ph->paths->count;++kp2)//for each ray
		{
			Path *path=(Path*)array_at(&ph->paths, kp2);
			if(!path->emerged)
				continue;
			Point *line=(Point*)array_at(&path->points, path->points->count-2);

			double dx=line[1].x-line[0].x;
			if(!dx||(dx<0)==(line[0].x<x_sensor))
				continue;
			Point *cross=(Point*)ARRAY_APPEND(intersections, 0, 1, 1, 0);
			intersect_lines(line, sensorPlane, cross);
			++out_path_count;
		}
	}
	meanvar((double*)intersections->data+1, (int)intersections->count, 2, &y_focus, &r_blur);
	r_blur=sqrt(r_blur);

	if(!no_focus&&user)//update h i s t o r y
	{
		history[hist_idx]=r_blur;
		++hist_idx, hist_idx%=w;
	}

	//cleanup
	array_free(&intersections);

	if(track_focus)
		VX=x_sensor, VY=y_focus;
}
double	calc_loss()
{
	return r_blur;

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
void			change_aperture_bound(OpticComp *oe, int kb, double factor)
{
	oe->info[kb].r_max[0]*=factor;
	oe->info[kb].r_max[1]*=factor;
}
void			change_aperture_comp(OpticComp *oc, double factor)
{
	for(int kb=0;kb<oc->nBounds;++kb)
	{
		oc->info[kb].r_max[0]*=factor;
		oc->info[kb].r_max[1]*=factor;
	}
}
void			change_aperture_all(double factor)
{
	for(int ke=0;ke<(int)elements->count;++ke)
	{
		OpticComp *oe=(OpticComp*)array_at(&elements, ke);
		change_aperture_comp(oe, factor);
	}
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
	double n2=*n+delta;
	if(n2>=1.2||n2<=2)
		*n=n2;
}
void			change_pos(OpticComp *oc, double delta)
{
	for(int kb=0;kb<oc->nBounds;++kb)
		oc->info[kb].pos+=delta;
}
void			change_thickness(OpticComp *oc, int kb, double delta)
{
	for(;kb<oc->nBounds;++kb)
	{
		Boundary *b=oc->info+kb;
		b->pos+=delta;
		if(kb>0&&b->pos<oc->info[kb-1].pos)//avoid -ve thickness
			b->pos=oc->info[kb-1].pos;
	}
}

//optimizer
#ifdef ENABLE_OPTIMIZER
int				argmin_quadratic(Point *p, double *ret_x)
{
	//x1^2*a + x1*b + c = f1	solve these for -b/(2a) (argmin of parabola) by Cramer's rule
	//x2^2*a + x2*b + c = f2
	//x3^2*a + x3*b + c = f3
#define		X1	p[0].x
#define		X2	p[1].x
#define		X3	p[2].x
#define		C1	p[0].y
#define		C2	p[1].y
#define		C3	p[2].y
	double sqX1, sqX2, sqX3, d, da, db, minX, minC;
	sqX1=X1*X1, sqX2=X2*X2, sqX3=X3*X3;
#if 1
	d=sqX2*X3-sqX3*X2 - (sqX1*X3-sqX3*X1) + sqX1*X2-sqX2*X1;//not sure this is needed
	if(!d)
		goto upside_down;
#endif
	db=sqX2*C3-sqX3*C2 - (sqX1*C3-sqX3*C1) + sqX1*C2-sqX2*C1;
	if(!db)
		goto upside_down;
	da=C2*X3-C3*X2 - (C1*X3-C3*X1) + C1*X2-C2*X1;
	if((da>0)!=(d>0))
		goto upside_down;
	*ret_x=-db/(2*da);
	return 1;
upside_down:
	minX=X1, minC=C1;
	if(minC>C2)
		minX=X2, minC=C2;
	if(minC>C3)
		minX=X3, minC=C3;
	*ret_x=minX;
	return 0;
#undef		X1
#undef		X2
#undef		X3
#undef		C1
#undef		C2
#undef		C3
}
void			set_var(OpticComp *oe, int kvar, double val)
{
	//kvar: {left.pos, left.radius, right.pos, right.radius}
	Boundary *s=oe->info+(kvar>>1);
	if(kvar&1)
		s->radius=val?1/val:0;
	else
		s->pos=val;
}
double			change_var(OpticComp *oe, int kvar, double delta)
{
	//kvar: {left.pos, left.radius, right.pos, right.radius}
	Boundary *s=oe->info+(kvar>>1);
	if(kvar&1)
	{
		change_diopter(&s->radius, delta);
		if(!s->radius)
			return 0;
		return 1/s->radius;
	}
	s->pos+=delta;
	return s->pos;
}
void			optimize(int ke, int kvar, double dx, double f0)
{
	OpticComp *oe;
	Point p[3];
	double optx;

	oe=(OpticComp*)array_at(&elements, ke);
	p[0].x=change_var(oe, kvar, dx);
	simulate(0);
	p[0].y=calc_loss();

	p[2].x=change_var(oe, kvar, -dx*2);
	simulate(0);
	p[2].y=calc_loss();

	p[1].x=change_var(oe, kvar, dx);
	p[1].y=f0;

	argmin_quadratic(p, &optx);
	set_var(oe, kvar, optx);
}
#endif
#if 0
void			calc_grad(double *grad, double step, int *var_idx, int nvars, int exclude_elem)
{
	history_enabled=0;
	memset(grad, 0, elements->count*nvars*sizeof(double));
	for(int ke=exclude_elem;ke<(int)elements->count;++ke)
	{
		OpticComp *ge=(OpticComp*)array_at(&elements, ke);
		if(ge->active)
		{
			for(int kv=0;kv<nvars;++kv)
			{
				change_var(ge, var_idx[kv], step);
				simulate(0);
				change_var(ge, var_idx[kv], -step);
				grad[nvars*ke+kv]=calc_loss();
			}
		}
	}
	simulate(0);
	double loss=calc_loss();
	for(int ke=exclude_elem;ke<(int)elements->count;++ke)
	{
		OpticComp *oe=(OpticComp*)array_at(&elements, ke);
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
		OpticComp *ge=(OpticComp*)array_at(&elements, ke);
		if(ge->active)
		{
			for(int kv=0;kv<nvars;++kv)
				change_var(ge, var_idx[kv], grad[nvars*ke+kv]);
		}
	}
}
#endif


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
double			draw_arc(double x, double R, double ap_min, double ap_max, ScaleConverter *scale, int nsegments)//returns top x
{
	double r2=R*R;
	int x1, y1, x2, y2,
		y3, y4;
	if(!R)
	{
		x1=real2screenX(x, scale);

		y1=real2screenY(ap_min, scale), y2=real2screenY(ap_max, scale);
		MoveToEx(ghMemDC, x1, y1, 0);
		LineTo(ghMemDC, x1, y2);

		y1=real2screenY(-ap_min, scale), y2=real2screenY(-ap_max, scale);
		MoveToEx(ghMemDC, x1, y1, 0);
		LineTo(ghMemDC, x1, y2);
		return x;
	}
	double ty, tx;
	for(int k=0;k<=nsegments;++k)
	{
		ty=ap_min+(ap_max-ap_min)*k/nsegments, tx=x+R-sgn_star(R)*sqrt(r2-ty*ty);
		x2=real2screenX(tx, scale);
		y2=real2screenY(ty, scale);
		y4=real2screenY(-ty, scale);
		if(k>0)
		{
			MoveToEx(ghMemDC, x1, y1, 0);
			LineTo(ghMemDC, x2, y2);
			MoveToEx(ghMemDC, x1, y3, 0);
			LineTo(ghMemDC, x2, y4);
		}
		x1=x2;
		y1=y2;
		y3=y4;
	}
	return tx;
}
void			draw_curve(Point *points, int npoints, ScaleConverter *scale)
{
	int x1=0, y1=0, x2, y2;
	for(int k=0;k<npoints;++k)
	{
		x2=real2screenX(points[k].x, scale);
		y2=real2screenY(points[k].y, scale);
		if(k)
		{
			MoveToEx(ghMemDC, x1, y1, 0);
			LineTo(ghMemDC, x2, y2);
		}
		//else
		//	Ellipse(ghMemDC, x2-10, y2-10, x2+10, y2+10);
		x1=x2;
		y1=y2;
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
		double topx[2]={0};
		for(int ke=0;ke<(int)elements->count;++ke)			//draw optical components
		{
			OpticComp *oe=(OpticComp*)array_at(&elements, ke);

			for(int kb=0;kb<oe->nBounds;++kb)
			{
				Boundary *b=oe->info+kb;
				topx[0]=topx[1];
				for(int ka=0;ka<2;++ka)
				{
					if(b->type[ka]!=SURF_HOLE&&(oe->active||b->type[ka]==SURF_MIRROR))
					{
						double r1, r2;
						if(ka)
							r1=b->r_max[0], r2=b->r_max[1];
						else
							r1=0, r2=b->r_max[0];
						if(b->type[ka]==SURF_MIRROR)
							hPenMirror=(HPEN)SelectObject(ghMemDC, hPenMirror);
						topx[1]=draw_arc(b->pos, b->radius, r1, r2, scale, segments);
						if(b->type[ka]==SURF_MIRROR)
							hPenMirror=(HPEN)SelectObject(ghMemDC, hPenMirror);
					}
				}
				if(oe->active&&kb)
				{
					draw_line(topx[0], oe->info[kb-1].r_max[1], topx[1], oe->info[kb].r_max[1], scale);
					draw_line(topx[0], -oe->info[kb-1].r_max[1], topx[1], -oe->info[kb].r_max[1], scale);
				}
			}
		}
		hPenLens=(HPEN)SelectObject(ghMemDC, hPenLens);
		
		hBrushHollow=(HBRUSH)SelectObject(ghMemDC, hBrushHollow);
#if 0
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
#endif
		{
			int xcenter=real2screenX(x_sensor, scale),
				ycenter=real2screenY(y_focus, scale);
			int rx=(int)(r_blur*Xr), ry=(int)(r_blur*Yr);
			Ellipse(ghMemDC, xcenter-rx, ycenter-ry, xcenter+rx, ycenter+ry);//combined blur circle
		}

		{//draw sensor
			int xs=real2screenX(x_sensor, scale), y1, y2;
			double y=fabs(y_focus);
			if(y<10)
				y=10;
			y1=real2screenY(y, scale);
			y2=real2screenY(-y, scale);
			MoveToEx(ghMemDC, xs, y1, 0);
			LineTo(ghMemDC, xs, y2);
		}

		if(!clearScreen)
		{
			double histmax=-1;
			for(int k=0;k<w;++k)		//draw history
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
				int idx=hist_idx-1;
				idx+=w&-(idx<0);
				if(history[idx]>=0)
				{
					int tempy=h-(int)(history[idx]*histmax);
					Ellipse(ghMemDC, idx-10, tempy-10, idx+10, tempy+10);
				}
			}

			int H=(int)(VY-DY/2>0?h:VY+DY/2<0?-1:h*(VY/DY+.5)), HT=H+(H>h-30?-18:2),
				V=(int)(VX-DX/2>0?-1:VX+DX/2<0?w:w*(-VX+DX/2)/DX), VT=V+(int)(V>w-24-prec*8?-24-prec*8:2);
			int bkMode=GetBkMode(ghMemDC);
			SetBkMode(ghMemDC, TRANSPARENT);
			for(double x=floor((VX-DX/2)/Xstep)*Xstep, xEnd=ceil((VX+DX/2)/Xstep)*Xstep, Xstep_2=Xstep/2;x<xEnd;x+=Xstep)//label the axes
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
			MoveToEx(ghMemDC, 0, H, 0), LineTo(ghMemDC, w, H);//draw the axes
			MoveToEx(ghMemDC, V, 0, 0), LineTo(ghMemDC, V, h);
			SetBkMode(ghMemDC, bkMode);

#if 0
			for(int k=0;k<(int)photons->count;++k)//draw ray ground
			{
				Photon *lp=(Photon*)array_at(&photons, k);
				MoveToEx(ghMemDC, real2screenX(lp->ground[0].x, scale), real2screenY(lp->ground[0].y, scale), 0);
				LineTo(ghMemDC, real2screenX(lp->ground[1].x, scale), real2screenY(lp->ground[1].y, scale));
			}
#endif

			int y=0;
#if 1
			GUITPrint(ghMemDC, 0, y, 0, "PRESS F1 FOR HELP"), y+=32;
#else
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
#endif

			int max_bounds=0;
			for(int ke=0;ke<(int)elements->count;++ke)
			{
				OpticComp *oe=(OpticComp*)array_at(&elements, ke);
				if(max_bounds<oe->nBounds)
					max_bounds=oe->nBounds;
			}

			int nCols=max_bounds*2-1;
			int printed=sprintf_s(g_buf, G_BUF_SIZE, "\t`");
			for(int k=0;k<nCols;++k)
				printed+=sprintf_s(g_buf+printed, G_BUF_SIZE-printed, "\t%d", k+1);
			GUITPrint(ghMemDC, 0, y, 0, g_buf), y+=16;

			printed=sprintf_s(g_buf, G_BUF_SIZE, "<>\tPos");
			for(int k=0;;++k)
			{
				printed+=sprintf_s(g_buf+printed, G_BUF_SIZE-printed, "\tR");
				++k;
				if(k>=nCols)
					break;
				printed+=sprintf_s(g_buf+printed, G_BUF_SIZE-printed, "\tth");
			}
			GUITPrint(ghMemDC, 0, y, 0, g_buf), y+=16;

			printed=sprintf_s(g_buf, G_BUF_SIZE, "^v\tAp");
			for(int k=0;;++k)
			{
				printed+=sprintf_s(g_buf+printed, G_BUF_SIZE-printed, "\tap");
				++k;
				if(k>=nCols)
					break;
				printed+=sprintf_s(g_buf+printed, G_BUF_SIZE-printed, "\tn");
			}
			GUITPrint(ghMemDC, 0, y, 0, g_buf), y+=32;
#if 0
			int printed=sprintf_s(g_buf, G_BUF_SIZE, "\t`>pos\t^ap");
			for(int k=0;;++k)
			{
				printed+=sprintf_s(g_buf+printed, G_BUF_SIZE-printed, "\t%d>R\t^ap", k+1);
				++k;
				if(k>=max_bounds)
					break;
				printed+=sprintf_s(g_buf+printed, G_BUF_SIZE-printed, "\t%d>th\t^n", k+1);
			}
			GUITPrint(ghMemDC, 0, y, 0, g_buf), y+=32;
#endif
			//GUITPrint(ghMemDC, 0, y, 0, "\t1 pos\t2 Rl\t3 Th\t4 Rr\t5 n\t6 ap"), y+=32;
			
			double max_ap;
			for(int ke=0;ke<(int)elements->count;++ke)
			{
				OpticComp *oe=(OpticComp*)array_at(&elements, ke);

				printed=sprintf_s(g_buf, G_BUF_SIZE, "%s %c\t%g", current_elem==ke?"->":" ", oe->active?'V':'X', oe->info[0].pos);
				for(int k=0;k<oe->nBounds;++k)
				{
					double r=oe->info[k].radius;
					if(k==(int)elements->count-1)
						r=-r;
					printed+=sprintf_s(g_buf+printed, G_BUF_SIZE-printed, "\t%g", r);
					if(k+1>=oe->nBounds)
						break;
					printed+=sprintf_s(g_buf+printed, G_BUF_SIZE-printed, "\t%g", oe->info[k+1].pos-oe->info[k].pos);
				}
				printed+=sprintf_s(g_buf+printed, G_BUF_SIZE-printed, "\t%s%s", (char*)oe->name->data, current_elem==ke?"\t<-":"");
				GUITPrint(ghMemDC, 0, y, 0, g_buf), y+=16;
				
				max_ap=0;
				for(int k=0;k<oe->nBounds;++k)
				{
					if(max_ap<oe->info[k].r_max[1])
						max_ap=oe->info[k].r_max[1];
				}
				printed=sprintf_s(g_buf, G_BUF_SIZE, "%s %c\t%g", current_elem==ke?"->":" ", oe->active?'V':'X', max_ap*2);
				for(int k=0;k<oe->nBounds;++k)
				{
					printed+=sprintf_s(g_buf+printed, G_BUF_SIZE-printed, "\t%g", oe->info[k].r_max[1]*2);
					if(k+1>=oe->nBounds)
						break;
					printed+=sprintf_s(g_buf+printed, G_BUF_SIZE-printed, "\t%g", oe->info[k].n_next);
				}
				printed+=sprintf_s(g_buf+printed, G_BUF_SIZE-printed, "%s", current_elem==ke?"\t\t<-":"");
				GUITPrint(ghMemDC, 0, y, 0, g_buf), y+=32;

#if 0
				double th=oe->info[1].pos-oe->info[0].pos;
				GUITPrint(ghMemDC, 0, y, 0, "  %s %c\t%g\t%g\t%g\t%g\t%g\t%g  %s%s",
					current_elem==ke?"->":" ",
					oe->active?'V':'X',
					oe->info[0].pos,
					oe->info[0].radius,
					th,
					-oe->info[1].radius,
					oe->n,
					maximum(oe->info[0].r_max[1], oe->info[1].r_max[1])*2,//aperture = diameter
					(char*)oe->name->data,
					current_elem==ke?"\t<-":"");
				y+=16;
#endif
			}
			y+=16;
			GUITPrint(ghMemDC, 0, y, 0, "\tSensor: %d/%d rays  Std.Dev %lfmm  Dblur=%lfmm%s",
				out_path_count,
				in_path_count,
				10*r_blur,
				20*r_blur,
				no_system?"\t\tNO SYSTEM":(no_focus?"\t\tNO FOCUS":"")
			);
			y+=16;
			if(initial_target.e_idx<(int)elements->count)
			{
				OpticComp *oe=(OpticComp*)array_at(&elements, initial_target.e_idx);
				double ymin, ymax;
				Boundary *surface=oe->info+initial_target.bound_idx;
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
				double ap=sqrt(ymax*ymax-ymin*ymin);
				int printed=0;
				if(!no_focus)
				{
					for(int kp=0;kp<(int)photons->count;++kp)
					{
						Photon *ph=(Photon*)array_at(&photons, 0);//any color
						Path *path=(Path*)array_at(&ph->paths, ph->paths->count-2);//even rays are above (mirror of) odd rays
						if(path->emerged)
						{
							Point *line=(Point*)array_at(&path->points, path->points->count-2);//
							if(line[1].y<line[0].y)//descending ray
							{
								//double
								//	slope=(line[0].x-line[1].x)/(line[0].y-line[1].y),
								//	x0=line[1].x+(0	-line[1].y)*slope,
								//	xa=line[1].x+(ymax-line[1].y)*slope,
								//	eq_focal_length=fabs(xa-x0);
								double eq_focal_length=fabs(ymax*(line[0].x-line[1].x)/(line[0].y-line[1].y));
								ymax*=2;
								GUITPrint(ghMemDC, 0, y, 0, "\tD=%g, Deq=%g, Feq=%gcm, f/%g, Dblur/Deq=%.8lf", ymax, ap*2, eq_focal_length, eq_focal_length/ymax, r_blur/ap);
								printed=1;
							}
							break;
						}
					}
				}
				if(!printed)
					GUITPrint(ghMemDC, 0, y, 0, "\tD=%g, Deq=%g, Dblur/Deq=%.8lf", ymax, ap, r_blur/ap);
				y+=16;
			}
			if(tan_tilt)
			{
				double
					tilt=atan(tan_tilt)*(180/M_PI),
					abs_tilt=fabs(tilt),

					degrees=floor(abs_tilt),
					tail=(abs_tilt-degrees)*60,
					minutes=floor(tail),
					seconds=(tail-minutes)*60;
				const char *a=0;
				abs_tilt*=2;
					 if(abs_tilt<0.000333334)	a="\tGanymede-";
				else if(abs_tilt<0.0005)		a="\tGanymede+";
				else if(abs_tilt<0.000611112)	a="\tNeptune-";
				else if(abs_tilt<0.000666667)	a="\tNeptune+";
				else if(abs_tilt<0.000916667)	a="\tUranus-";
				else if(abs_tilt<0.000972222)	a="\tMars-";
				else if(abs_tilt<0.001138888)	a="\tUranus+";
				else if(abs_tilt<0.00125)		a="\tMercury-";
				else if(abs_tilt<0.00269444)	a="\tVenus-";
				else if(abs_tilt<0.00361111)	a="\tMercury+";
				else if(abs_tilt<0.00402776)	a="\tSaturn-";
				else if(abs_tilt<0.00558334)	a="\tSaturn+";
				else if(abs_tilt<0.00697222)	a="\tMars+";
				else if(abs_tilt<0.00827778)	a="\tJupiter-";
				else if(abs_tilt<0.01391667)	a="\tJupiter+";
				else if(abs_tilt<0.01833334)	a="\tVenus+";
				else if(abs_tilt<0.488334)		a="\tMoon-";
				else if(abs_tilt<0.458612)		a="\tSun-";
				else if(abs_tilt<0.542222)		a="\tSun+";
				else if(abs_tilt<0.568334)		a="\tMoon+";
				else if(abs_tilt<1)				a="\tAndromeda-";
				else if(abs_tilt<3.01667)		a="\tAndromeda+";
				else							a="";
				GUITPrint(ghMemDC, 0, y, 0, "\tTilt = %gd %g\' %g\"%s", tilt, minutes, seconds, a);
				y+=16;
			}
			y+=16;
#if 0
			Point *point=(Point*)array_at(&ray_spread_mean, (int)photons->count);
			if(point)
			{
				double focus=sqrt(point->x*point->x+point->y*point->y);
				AreaIdx *aidx=(AreaIdx*)array_at(&order, 0);
				OpticComp *oe=(OpticComp*)array_at(&elements, aidx->e_idx);
				double ap=oe->info[aidx->bound_idx].r_max[1];
				y=h-16*6;
				GUITPrint(ghMemDC, 0, y, 0, "D %gcm, F %gcm, f/%g\t Std.Dev %lfmm", ap, focus, focus/ap, r_blur), y+=16;
			}
#endif

			int txtColor0=GetTextColor(ghMemDC);
			for(int kp=0;kp<(int)photons->count;++kp)//print lambdas
			{
				Photon *ph=(Photon*)array_at(&photons, kp);
				SetTextColor(ghMemDC, ph->color);
				GUITPrint(ghMemDC, 0, y, 0, "\t%gnm", ph->lambda), y+=16;
				//GUIPrint(ghMemDC, 0, y, "lambda%d = %gnm", kp+1, ph->lambda), y+=16;
			}
			SetTextColor(ghMemDC, txtColor0);
		}
		hBrushHollow=(HBRUSH)SelectObject(ghMemDC, hBrushHollow);
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

	if(w)
	{
		history=realloc(history, w*sizeof(double));
		hist_idx%=w;
		for(int k=hist_idx;k<w;++k)
			history[k]=-1;
		//memset(history+hist_idx, 0, (w-hist_idx)*sizeof(double));
	}
}
void			set_attribute(int attrNo)//attrNo: attrNo>>1 is kx, LSB is ky: {`/0: pos/Ap, 1: R/ap, 2: th/n, 3: R/ap, ...}
{
	double val;
	OpticComp *oe=(OpticComp*)array_at(&elements, current_elem);
	console_start();

	int kx=attrNo>>1, ky=attrNo&1;
	printf("Set %s ", (char*)oe->name->data);
	if(!kx)
	{
		if(ky)
			printf("global aperture");
		else
			printf("pos");
	}
	else if(kx&1)
	{
		if(ky)
			printf("aperture of boundary #%d", (kx>>1)+1);
		else
			printf("radius of boundary #%d", (kx>>1)+1);
	}
	else//kx is even & can't be zero
	{
		if(ky)
			printf("refractive index (n) of medium #%d", kx>>1);
		else
			printf("thickness of medium #%d", kx>>1);
	}
	printf(": ");

	val=0;
	scanf_s("%lf", &val);
	
	if(!kx)
	{
		if(ky)//set global Ap
		{
			for(int kb=0;kb<oe->nBounds;++kb)
			{
				oe->info[kb].r_max[1]=val;
				if(oe->info[kb].r_max[0]>val)
					oe->info[kb].r_max[0]=val;
			}
		}
		else//set element position
		{
			double delta=val-oe->info[0].pos;
			for(int kb=0;kb<oe->nBounds;++kb)
				oe->info[kb].pos+=delta;
		}
	}
	else if(kx&1)
	{
		if(ky)//set aperture
		{
			oe->info[kx>>1].r_max[1]=val;//FIXME: what if I need to set r_max[0]?
			if(oe->info[kx>>1].r_max[0]>val)
				oe->info[kx>>1].r_max[0]=val;
		}
		else//set curvature radius
			oe->info[kx>>1].radius=val;
	}
	else//kx is even & can't be zero
	{
		if(ky)//set refractive index
			oe->info[(kx>>1)-1].n_next=val;
		else//set thickness
		{
			double delta=val-(oe->info[kx>>1].pos-oe->info[(kx>>1)-1].pos);//new thickness - prev thickness
			for(int k=kx>>1;k<oe->nBounds;++k)
				oe->info[k].pos+=delta;
		}
	}
#if 0
	const char *a=0;
	switch(attrNo)
	{
	case 1:a="position";break;
	case 2:a="left radius";break;
	case 3:a="thickness";break;
	case 4:a="right radius";break;
	case 5:a="refractive index (n)";break;
	default:a="UNRECOGNIZED ATTRIBUTE";break;
	}
	printf("Set %s %s: ", (char*)oe->name->data, a);
	double val=0;
	scanf_s("%lf", &val);
	switch(attrNo)
	{
	case 1://set position
		oe->info[1].pos=val+oe->info[1].pos-oe->info[0].pos;
		oe->info[0].pos=val;
		break;
	case 2://set left radius
		oe->info[0].radius=val;
		break;
	case 3://set thickness
		oe->info[1].pos=oe->info[0].pos+val;
		break;
	case 4://set right radius
		oe->info[1].radius=-val;
		break;
	case 5:
		oe->n=val;
		break;
	}
#endif
	simulate(1);
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
			function1();
		}
		break;
	case WM_ACTIVATE:
		switch(wParam)
		{
		case WA_ACTIVE:
		case WA_CLICKACTIVE:
			for(int k=0;k<256;++k)
				keyboard[k]=(GetKeyState(k)&0x8000)!=0;
			kpressed=keyboard[VK_UP]+keyboard[VK_DOWN]+keyboard[VK_LEFT]+keyboard[VK_RIGHT]
				+keyboard[VK_ADD]+keyboard[VK_SUBTRACT]//numpad
				+keyboard[VK_OEM_PLUS]+keyboard[VK_OEM_MINUS]
				+keyboard[VK_RETURN]+keyboard[VK_BACK];
			if(kpressed&&!timer)
				timer=1, SetTimer(ghWnd, 0, 10, 0);
			break;
		case WA_INACTIVE:
			keyboard[VK_LBUTTON]=0;
			if(drag)
			{
				ReleaseCapture();
				SetCursorPos(mouseP0.x, mouseP0.y);
				ShowCursor(1);
				drag=0;
			}
			kpressed=0;
			timer=0, KillTimer(ghWnd, 0);
			break;
		}
		break;
	case WM_PAINT:
		render();
		break;
	case WM_TIMER:
		{
			OpticComp *oe=(OpticComp*)array_at(&elements, current_elem);
			double dx=DX/w, delta;//horizontal pixel size in real units: the more you zoom in, the finer is the change
			int e=0;

			if(keyboard[VK_SHIFT])
				delta=10*dx;
			else if(keyboard[VK_CONTROL])
				delta=0.1*dx;
			else
				delta=dx;

			if(keyboard[VK_OEM_3])//<> pos / ^v Ap
			{
				if(keyboard[VK_LEFT])	change_pos(oe, -delta), e=1;
				if(keyboard[VK_RIGHT])	change_pos(oe, delta), e=1;
				if(keyboard[VK_UP])		change_aperture_comp(oe, 1+0.1*dx), e=1;
				if(keyboard[VK_DOWN])	change_aperture_comp(oe, 1/(1+0.1*dx)), e=1;
			}
			delta*=0.01;
			for(int kb=0;kb<oe->nBounds;++kb)
			{
				int idx=kb<<1;
				if(keyboard['1'+idx]||keyboard[VK_NUMPAD1+idx])//R/ap
				{
					int allow_mod=oe->active||oe->info[kb].type[0]==SURF_MIRROR||oe->info[kb].type[1]==SURF_MIRROR;
					if(!allow_mod)
						continue;
					if(keyboard[VK_LEFT	])	change_diopter(&oe->info[kb].radius, -delta), e=1;//can be modified when inactive because of mirrors
					if(keyboard[VK_RIGHT])	change_diopter(&oe->info[kb].radius, delta), e=1;
					if(keyboard[VK_UP	])	change_aperture_bound(oe, kb, 1/(1+0.1*dx)), e=1;
					if(keyboard[VK_DOWN	])	change_aperture_bound(oe, kb, 1+0.1*dx), e=1;
				}
			}
			delta*=100;
			for(int kb=1;kb<oe->nBounds;++kb)
			{
				int idx=(kb-1)<<1;
				if(keyboard['2'+idx]||keyboard[VK_NUMPAD2+idx])//th/n
				{
					if(keyboard[VK_LEFT	]&&oe->active)	change_thickness(oe, kb, -delta), e=1;
					if(keyboard[VK_RIGHT]&&oe->active)	change_thickness(oe, kb, delta), e=1;
					if(keyboard[VK_UP	]&&oe->active)	change_n(&oe->info[kb-1].n_next, 0.01*dx), e=1;//last n_next is always 1
					if(keyboard[VK_DOWN	]&&oe->active)	change_n(&oe->info[kb-1].n_next, -0.01*dx), e=1;
				}
			}
			if(!e)
			{
				if(_2d_drag_graph_not_window)
				{
					if(keyboard[VK_LEFT	])	track_focus=0, VX+=10*dx;
					if(keyboard[VK_RIGHT])	track_focus=0, VX-=10*dx;
					if(keyboard[VK_UP	])	track_focus=0, VY-=10*dx/AR_Y;
					if(keyboard[VK_DOWN	])	track_focus=0, VY+=10*dx/AR_Y;
				}
				else
				{
					if(keyboard[VK_LEFT	])	track_focus=0, VX-=10*dx;
					if(keyboard[VK_RIGHT])	track_focus=0, VX+=10*dx;
					if(keyboard[VK_UP	])	track_focus=0, VY+=10*dx/AR_Y;
					if(keyboard[VK_DOWN	])	track_focus=0, VY-=10*dx/AR_Y;
				}
			}
			if(keyboard[VK_ADD		]||keyboard[VK_RETURN	]||keyboard[VK_OEM_PLUS	])
			{
				if(keyboard[VK_CONTROL])
					e|=shift_lambdas(1);
				else if(keyboard[VK_SHIFT])
				{
					if(tan_tilt<0.7071)
						tan_tilt+=0.00002*DX, e=1;
				}
				else if(keyboard['A'])
				{
					change_aperture_all(1+0.1*dx);
					e=1;
				}
				else
				{
						 if(keyboard['X'])	DX/=1.05, AR_Y/=1.05;
					else if(keyboard['Y'])	AR_Y*=1.05;
					else					DX/=1.05;
					function1();
				}
			}
			if(keyboard[VK_SUBTRACT	]||keyboard[VK_BACK	]||keyboard[VK_OEM_MINUS	])
			{
				if(keyboard[VK_CONTROL])
					e|=shift_lambdas(-1);
				else if(keyboard[VK_SHIFT])
				{
					if(tan_tilt>-0.7071)
						tan_tilt-=0.00002*DX, e=1;
				}
				else if(keyboard['A'])
				{
					change_aperture_all(1/(1+0.1*dx));
					e=1;
				}
				else
				{
						 if(keyboard['X'])	DX*=1.05, AR_Y*=1.05;
					else if(keyboard['Y'])	AR_Y/=1.05;
					else					DX*=1.05;
					function1();
				}
			}
			if(e)
				simulate(1);
			InvalidateRect(ghWnd, 0, 0);
			if(!kpressed)
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
				track_focus=0;
				if(!timer)
					InvalidateRect(ghWnd, 0, 0);
				SetCursorPos(centerP.x, centerP.y);//moves mouse
			}
			m_bypass=!m_bypass;
		}
		return 0;
	case WM_LBUTTONDOWN:
		keyboard[VK_LBUTTON]=1;
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
		keyboard[VK_LBUTTON]=0;
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
			if(keyboard[VK_CONTROL])//change nrays
			{
				nrays+=mw_forward-!mw_forward;
				if(nrays<1)
					nrays=1;
				simulate(1);
			}
			else if(keyboard['X'])//scale x-axis
			{
					 if(mw_forward)	DX/=1.1, AR_Y/=1.1, VX=VX+dx-dx/1.1;
				else				DX*=1.1, AR_Y*=1.1, VX=VX+dx-dx*1.1;
				track_focus=0;
			}
			else if(keyboard['Y'])//scale y-axis
			{
					 if(mw_forward)	AR_Y*=1.1, VY=VY+dy-dy/1.1;
				else				AR_Y/=1.1, VY=VY+dy-dy*1.1;
				track_focus=0;
			}
			else//zoom
			{
					 if(mw_forward)	DX/=1.1, VX=VX+dx-dx/1.1, VY=VY+dy-dy/1.1;
				else				DX*=1.1, VX=VX+dx-dx*1.1, VY=VY+dy-dy*1.1;
				track_focus=0;
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
			if(!keyboard[wParam])
				++kpressed;
			if(!timer)
				SetTimer(ghWnd, 0, 10, 0), timer=1;
			break;
		case VK_TAB:
			current_elem=mod(current_elem+!keyboard[VK_SHIFT]-keyboard[VK_SHIFT], (int)elements->count);
			break;
		case VK_SPACE:
			{
				OpticComp *oe=(OpticComp*)array_at(&elements, current_elem);
				oe->active=!oe->active;
				simulate(1);
			}
			break;
		case VK_F1:
			messagebox("Controls",
				"Up/down/left/right/drag:\t\tNavigate\n"
				"+/-/Enter/Backspace/Mousewheel:\tZoom\n"
				"X/Y +/-/Enter/Backspace:\t\tChange aspect ratio\n"
				"T:\tTrack focal point\n"
				"R:\tReset view\n"
				"E:\tReset scale\n"
				"C:\tToggle clear screen\n"
				"\n"
				"Ctrl O:\t\tOpen a preset\n"
				"Tab / Shift Tab:\tSelect glass element\n"
				"Space:\t\tToggle glass element\n"
				"F:\t\tFlip glass element\n"
				"\n"
				"`(grave accent)/1/...9 S:\tSet top row parameter\n"
				"`(grave accent)/1/...9 Z:\tSet bottom row parameter\n"
				"`(grave accent) left/right:\tMove glass component\n"
				"`(grave accent) up/down:\tChange component aperture (Ap)\n"
				"1/3/5/7/9 left/right:\tChange corresponding boundary radius (R)\n"
				"1/3/5/7/9 up/down:\tChange corresponding boundary aperture (ap)\n"
				"2/4/6/8 left/right:\tChange corresponding medium thickness (th)\n"
				"2/4/6/8 up/down:\tChange corresponding medium refractive index (n)\n"
				"Speed of change depends on zoom level\n"
				"Hold Shift for fast motion, Ctrl for slow motion\n"
#if 0//don't uncomment this
				"1 left/right: Move glass element\n"
				"2 left/right: Change left diopter\n"
				"3 left/right: Change thickness\n"
				"4 left/right: Change right diopter\n"
				"5 left/right: Change refractive index (n)\n"
				"6 left/right: Change element aperture\n"
				"Speed of change depends on zoom level\n"
				"1~6 S: Set value of corresponding property\n"
#endif
				"\n"
				"Shift R:\tReset all glass elements\n"
			//	"1~6 R: Reset corresponding property of current glass element\n"
				"Alt R:\tReset ray tilt\n"
				"\n"
				"A +/-/Enter/Backspace:\tChange all apertures\n"
				"Ctrl +/-/Enter/Backspace:\tShift wavelengths\n"
				"Shift +/-/Enter/Backspace:\tChange ray tilt\n"
				"Ctrl Mousewheel:\t\tChange ray count\n"
				"\n"
				"H:\tClear loss function history\n"
#ifdef ENABLE_OPTIMIZER
				"O: Optimize current glass element\n"
				"Shift O: Optimize all glass elements\n"
#endif
				"\n"
				"Built on: %s %s", __DATE__, __TIME__);
			break;
		case 'O':
			if(keyboard[VK_CONTROL])//open another system
			{
				if(open_system(0))
				{
					tan_tilt=0;
					simulate(1);
				}
			}
#ifdef ENABLE_OPTIMIZER
			else
			{
				double dx=DX/w;
				double loss=calc_loss();
				if(keyboard[VK_SHIFT])//optimize all elements
				{
					for(int ke=0;ke<(int)elements->count;++ke)//X
						for(int kv=0;kv<4;++kv)
							optimize(ke, kv, dx, loss);
				}
				else//optimize current element
				{
					for(int kv=0;kv<4;++kv)//X  joint optimization
						optimize(current_elem, kv, dx, loss);
				}
				simulate(1);
			}
#endif
#if 0
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
				int exclude_first=keyboard[VK_SHIFT]!=0;
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
				simulate(1);
			}
#endif
			break;
		case 'S':
			if(keyboard['1'])
				set_attribute(1);
			else if(keyboard['2'])
				set_attribute(2);
			else if(keyboard['3'])
				set_attribute(3);
			else if(keyboard['4'])
				set_attribute(4);
			else if(keyboard['5'])
				set_attribute(5);
			else//save preset
			{
				save_system(0);
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
				OpticComp *oe=(OpticComp*)array_at(&elements, current_elem), *oe2;
				int e=0;
				if(keyboard['2'])
				{
					if(oe->info[0].radius)
						oe->info[0].radius=0;
					else
					{
						oe2=(OpticComp*)array_at(&ebackup, current_elem);
						oe->info[0].radius=oe2->info[0].radius;
					}
					e=1;
				}
				if(keyboard['4'])
				{
					if(oe->info[1].radius)
						oe->info[1].radius=0;
					else
					{
						oe2=(OpticComp*)array_at(&ebackup, current_elem);
						oe->info[1].radius=oe2->info[1].radius;
					}
					e=1;
				}
				if(e)
					simulate(1);
			}
			break;
		case 'T':
			track_focus=!track_focus;
			if(track_focus)
			{
				saved_VX=VX;
				saved_VY=VY;
				VX=x_sensor;
				VY=y_focus;
			}
			else
			{
				VX=saved_VX;
				VY=saved_VY;
			}
			break;
		//case 'M':
		//	twosides=!twosides;
		//	simulate(1);
		//	break;
		case 'F'://flip optical element
			{
				OpticComp *oe=(OpticComp*)array_at(&elements, current_elem);
				if(oe->active)
				{
					//Boundary temp;//BREAKS PATH
					//memswap(oe->info, oe->info+1, sizeof(Boundary), &temp);
					//memswap(&oe->info[0].pos, &oe->info[1].pos, sizeof(double), &temp);
					//negate(&oe->info[0].radius);
					//negate(&oe->info[1].radius);

					double temp=oe->info[0].radius;
					oe->info[0].radius=-oe->info[1].radius;
					oe->info[1].radius=-temp;

					simulate(1);
				}
			}
			break;
		case 'R':
			{
				int e=0;
				OpticComp *oe=(OpticComp*)array_at(&elements, current_elem);
				OpticComp *oe0=(OpticComp*)array_at(&ebackup, current_elem);
				if(keyboard[VK_MENU])
				{
					tan_tilt=0;
					e=1;
				}
				else if(keyboard[VK_SHIFT])
				{
					memcpy(elements->data, ebackup->data, elements->count*sizeof(OpticComp));
					tan_tilt=0;
					//ap=ap0;
					//lambda=lambda0;
					e=1;
				}
#if 0
				else if(keyboard['1'])
					oe->info[0].pos=oe0->info[0].pos, e=1;
				else if(keyboard['2'])
					oe->info[0].radius=oe0->info[0].radius, e=1;
				else if(keyboard['3'])
					oe->info[1].pos=oe->info[0].pos+(oe0->info[1].pos-oe0->info[0].pos), e=1;
				else if(keyboard['4'])
					oe->info[1].radius=oe0->info[1].radius, e=1;
				else if(keyboard['5'])
					oe->n=oe0->n, e=1;
#endif
				if(e)
					simulate(1);
				else
				{
					if(!keyboard[VK_CONTROL])
						DX=20, AR_Y=1, function1();
					VX=VY=0;
				}
			}
			break;
		case 'E':
			if(keyboard[VK_CONTROL])
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
		keyboard[wParam]=1;
		if(!timer)
			InvalidateRect(ghWnd, 0, 0);
		if(message==WM_SYSKEYDOWN)
			break;
		return 0;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		keyboard[wParam]=0;
		switch(wParam)
		{
		case VK_UP:case VK_DOWN:case VK_LEFT:case VK_RIGHT:
		case VK_ADD:case VK_SUBTRACT://numpad
		case VK_OEM_PLUS:case VK_OEM_MINUS:
		case VK_RETURN:case VK_BACK:
			if(kpressed)//start+key
				--kpressed;
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
	hPenLens=CreatePen(PS_SOLID, 1, 0x00808080);
	hPenMirror=CreatePen(PS_SOLID, 1, 0x00FF00FF);
	hBrushHollow=(HBRUSH)GetStockObject(NULL_BRUSH);

	RegisterClassExA(&wndClassEx);
	ghWnd=CreateWindowExA(0, wndClassEx.lpszClassName, "Optics Simulator", WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_CLIPCHILDREN, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 0, 0, hInstance, 0);//20220504
	
		if(__argc!=2||!open_system(__argv[1]))
			init_system();
		//if(!open_system())
		//	return 1;
		wnd_resize();
		function1();

		//shift_lambdas(0);
		ebackup=array_copy(&elements);
		simulate(1);

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