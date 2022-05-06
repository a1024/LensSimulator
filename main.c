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

//	#define		WAVELENGTH_IS_VARIABLE

#define			SIZEOF(ST_ARR)	(sizeof(ST_ARR)/sizeof(*(ST_ARR)))
#define			G_BUF_SIZE	2048
char			g_buf[G_BUF_SIZE]={0};
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
int				mod(int x, int n)
{
	x%=n;
	x+=n&-(x<0);
	return x;
}

typedef struct PointStruct
{
	double x, y;
} Point;
typedef struct LineStruct
{
	double x1, y1, x2, y2;
} Line;
int		nlines=0;
//size_t	lines_bytes=0;
Line	*lines=0;

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
	LambdaColor *v0=wrgb+L, *v1=wrgb+L+1;
	float x=((float)lambda-v0->lambda)/(v1->lambda-v0->lambda);
	int ret;
	unsigned char *p=(unsigned char*)&ret;
	p[0]=mixchar(v0->comp[2], v1->comp[2], x);
	p[1]=mixchar(v0->comp[1], v1->comp[1], x);
	p[2]=mixchar(v0->comp[0], v1->comp[0], x);
	p[3]=0;
	return ret;
}
const double coeff_approx[]={1.87816, 1./8000};
double			r_idx2wavelength(double n)
{
	return (1/(n-1)-coeff_approx[0])/coeff_approx[1];
}
double			wavelength2refractive_index(double lambda)
{
	return 1+1/(coeff_approx[1]*lambda+coeff_approx[0]);
	//return 1.60+(1.54-1.60)/(800-300)*(lambda-300);
	//return 1.5+0.008*20*log(1+exp(1/20*-0.1*(lambda-450)));//approximated with softplus
}
#endif
//#ifdef WAVELENGTH_IS_VARIABLE
double			lambda=565;//nm
//#else
#define			R_IDX_COUNT	3
double			r_idx[R_IDX_COUNT]={1.516, 1.513, 1.510};//B->R
int				ray_idx[R_IDX_COUNT+1]={0};
const int		ridx_count=SIZEOF(r_idx);
//#endif

double	*ray_spread=0, ray_spread_mean=0;
int		nhit=0;
double	*history=0;
int		hist_idx=0, history_enabled=1;

#ifdef WAVELENGTH_IS_VARIABLE
int		nrays=25;
#else
int		nrays=9;
#endif
int		nsurfaces=4;
int		correctorOn=1;

int		line_buffer_size=0;
void	lines_alloc(int count)
{
	//if(!lines_bytes)
	//	lines_bytes=1;
	//for(;count*sizeof(Line)>=lines_bytes;lines_bytes<<=1);
	//lines=(Line*)realloc(lines, lines_bytes);

	if(count!=line_buffer_size)
	{
		lines=(Line*)realloc(lines, count*sizeof(Line));
		line_buffer_size=count;
	}
}
void	lines_init()
{
	ray_spread=(double*)realloc(ray_spread, nrays*sizeof(double));
	lines_alloc((nsurfaces+2)*nrays);
	nlines=0;
}
void	lines_insert(double x1, double y1, double x2, double y2)
{
//	lines_alloc((nsurfaces+2)*nrays);
	Line *line=lines+nlines;
	line->x1=x1;
	line->y1=y1;
	line->x2=x2;
	line->y2=y2;
	++nlines;
}

int		sgn_star(double x){return 1-((x<0)<<1);}
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
void	normalize(double dx, double dy, Point *ret)
{
	double invhyp=1/sqrt(dx*dx+dy*dy);
	ret->x=dx*invhyp;
	ret->y=dy*invhyp;
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
int		refract_ray_surface(double x1, double y1, double x2, double y2, double aperture, double nL, double nR, double x, double R, Point *ret_line)
{
	int hit=0;
	Point sol[2];
	double sin_in, sin_em, cos_em;
	if(x2==x1)
		return 0;
	if(x2<x1)
	{
		double temp;
		temp=x1, x1=x2, x2=temp;
		temp=y1, y1=y2, y2=temp;
	}
	if(!R||fabs(R)==_HUGE)//plane at x
	{
		ret_line[0].x=x;
		ret_line[0].y=y1+(y2-y1)/(x2-x1)*(x-x1);
		hit=fabs(ret_line[0].y)<aperture;
		if(hit)
		{
			normalize(x2-x1, y2-y1, sol);
			sin_in=sol->y;
			hit+=refract(nL, nR, sin_in, &cos_em, &sin_em);
			ret_line[1].x=ret_line[0].x+cos_em;
			ret_line[1].y=ret_line[0].y+sin_em;
		}
	}
	else//+/- spherical at x
	{
		double xcenter=x+R;
		hit=intersect_line_circle(x1, y1, x2, y2, xcenter, R, sol);
		if(hit)
		{
			if((R>0)==(sol[0].x<sol[1].x))//convex from left XNOR sol0 is leftmost
			{
				ret_line[0].x=sol[0].x;
				ret_line[0].y=sol[0].y;
			}
			else//convex from left XOR sol0 is leftmost
			{
				ret_line[0].x=sol[1].x;
				ret_line[0].y=sol[1].y;
			}
			hit=fabs(ret_line[0].y)<aperture*0.5;
			if(hit)
			{
				int inc_dir=sgn_star(x2-x1);
				normalize(x2-x1, y2-y1, sol);
				normalize(sgn_star(R)*(xcenter-ret_line[0].x), -sgn_star(R)*ret_line[0].y, sol+1);
				sin_in=sol[0].y*sol[1].x-sol[0].x*sol[1].y;
				hit+=refract(nL, nR, sin_in, &cos_em, &sin_em);
				double
					c2=cos_em*sol[1].x-sin_em*sol[1].y,
					s2=sin_em*sol[1].x+cos_em*sol[1].y;
				if(hit!=2&&inc_dir!=sgn_star(c2))
					c2=-c2, s2=-s2;
				ret_line[1].x=ret_line[0].x+c2;
				ret_line[1].y=ret_line[0].y+s2;
			}
		}
	}
	return hit;
}
double	extrapolate_x(double x1, double y1, double x2, double y2, double xCCD){return y1+(y2-y1)/(x2-x1)*(xCCD-x1);}
double	extrapolate_y(double x1, double y1, double x2, double y2, double yground){return x1+(x2-x1)/(y2-y1)*(yground-y1);}
void	meanvar(double *arr, int size, double *ret_mean, double *ret_var)
{
	double mean=0, var=0;
	if(size>0)
	{
		for(int k=0;k<size;++k)
			mean+=arr[k];
		mean/=size;
		var=0;
		for(int k=0;k<size;++k)
		{
			double val=arr[k]-mean;
			var+=val*val;
		}
		var/=size;
	}
	if(ret_mean)
		*ret_mean=mean;
	if(ret_var)
		*ret_var=var;
}
#if 0
double	eval(double aperture, int nrays, double n, double R1, double t1, double xdist, double R2a, double t2, double R2b, double xCCD, double xend)//returns standard deviation of rays
{
	Point ln1[2], ln2[2];
	int intersect;
	lines_init();
	nhit=0;
	for(int k=0;k<nrays;++k)
	{
		double ystart=aperture*(k+1)/nrays*0.5, x=0;
		intersect=refract_ray_surface(-10, ystart, x, ystart, aperture, 1, n, x, R1, ln1);
		if(!intersect)
		{
			lines_insert(-10, ystart, 0, ystart);
			continue;
		}
		lines_insert(-10, ystart, ln1[0].x, ln1[0].y);

		x+=t1;

		intersect=refract_ray_surface(ln1[0].x, ln1[0].y, ln1[1].x, ln1[1].y, aperture, n, 1, x, 0, ln2);
		if(!intersect)
		{
			lines_insert(ln1[0].x, ln1[0].y, ln1[1].x, ln1[1].y);
			continue;
		}
		lines_insert(ln1[0].x, ln1[0].y, ln2[0].x, ln2[0].y);

		x+=xdist;

		if(correctorOn)
		{
			intersect=refract_ray_surface(ln2[0].x, ln2[0].y, ln2[1].x, ln2[1].y, aperture, 1, n, x, R2a, ln1);
			if(!intersect)
			{
				lines_insert(ln2[0].x, ln2[0].y, ln2[1].x, ln2[1].y);
				continue;
			}
		}
		lines_insert(ln2[0].x, ln2[0].y, ln1[0].x, ln1[0].y);

		x+=t2;
		
		if(correctorOn)
		{
			intersect=refract_ray_surface(ln1[0].x, ln1[0].y, ln1[1].x, ln1[1].y, aperture, n, 1, x, -R2b, ln2);
			if(!intersect)
			{
				lines_insert(ln1[0].x, ln1[0].y, ln1[1].x, ln1[1].y);
				continue;
			}
		}
		lines_insert(ln1[0].x, ln1[0].y, ln2[0].x, ln2[0].y);

		double y=extrapolate_x(ln2[0].x, ln2[0].y, ln2[1].x, ln2[1].y, xend);
		lines_insert(ln2[0].x, ln2[0].y, xend, y);
		ray_spread[nhit]=extrapolate_y(ln2[0].x, ln2[0].y, ln2[1].x, ln2[1].y, 0);
		//ray_spread[nhit]=extrapolate_x(ln2[0].x, ln2[0].y, ln2[1].x, ln2[1].y, xCCD);

		++nhit;
	}
	ray_spread_mean=0;
	double variance=-1;
	meanvar(ray_spread, nhit, &ray_spread_mean, &variance);
	for(int k=0;k<nhit;++k)
		ray_spread[k]-=ray_spread_mean;
	//free(ray_spread);
	variance=sqrt(variance);
	history[hist_idx]=variance;
	++hist_idx, hist_idx%=w;
	return variance;
}
#endif
double
	n_float=1.512, ap=12,//pre-determined

	//DEPRECATED
	t1=0.8, R1=51.2, xdist=16, R2a=30, t2=1, R2b=-70,//std dev = 0.445444mm
//	t1=0.8, R1=51.2, xdist=16, R2a=30, t2=1, R2b=-70,
//	xdist=5, R2a=50, t2=2, R2b=-50,
//	xdist=4, R2a=100, t2=2, R2b=100,

	f1=100,//unused
	xCCD=99.07, xCut=200;
double spread=0;

typedef struct GlassElemStruct
{
	int active;
	union
	{
		struct{double dist, Rl, th, Rr;};//cm;
		double vars[4];
	};
} GlassElem;
GlassElem		elements[]=
{
	//learned parameters (cm)
	{1,		0,		51.2,	0.8,	1000},	//53cm gives 98cm
	{1,		15.1,	16,		1,		-15},
	{0,		2,		14,		1,		-10},
	{0,		2,		30,		1,		-70},
};
int				ecount=SIZEOF(elements), current_elem=0;
void			eval2(GlassElem *elements, int count, double n, double aperture, double xend)
{
	Point l1[2], l2[2];
	int hit;
	GlassElem *ge;

	ecount=count;
	ray_spread=(double*)realloc(ray_spread, nrays*sizeof(double));
	lines=(Line*)realloc(lines, ((count*2+2)*nrays)*sizeof(Line));
//	lines_alloc((count*2+2)*nrays);
	nlines=0;
	nhit=0;
	//for(int k=nrays-1;k>=0;--k)
	for(int k=0;k<nrays;++k)
	{
		l1[0].x=-10, l1[0].y=aperture*(k+1)/(nrays+1)*0.5, l1[1].x=0, l1[1].y=l1[0].y;
	//	lines_insert(l1[0].x, l1[0].y, l1[1].x, l1[1].y);
		double x=0;
		for(int k2=0;k2<count;++k2)
		{
			ge=elements+k2;
			x+=ge->dist;
			if(ge->active)
			{
				hit=refract_ray_surface(l1[0].x, l1[0].y, l1[1].x, l1[1].y, aperture, 1, n, x, ge->Rl, l2);
				if(!hit)
				{
					lines_insert(l1[0].x, l1[0].y, l1[1].x, l1[1].y);
					break;
				}
				lines_insert(l1[0].x, l1[0].y, l2[0].x, l2[0].y);
				if(hit==2)
				{
					lines_insert(l2[0].x, l2[0].y, l2[1].x, l2[1].y);
					break;
				}
			}
			x+=ge->th;
			if(ge->active)
			{
				hit=refract_ray_surface(l2[0].x, l2[0].y, l2[1].x, l2[1].y, aperture, n, 1, x, -ge->Rr, l1);
				if(!hit)
				{
					lines_insert(l2[0].x, l2[0].y, l2[1].x, l2[1].y);
					break;
				}
				lines_insert(l2[0].x, l2[0].y, l1[0].x, l1[0].y);
				if(hit==2)
				{
					lines_insert(l1[0].x, l1[0].y, l1[1].x, l1[1].y);
					break;
				}
			}
		}
		if(hit==1)
		{
			ray_spread[nhit]=extrapolate_y(l1[0].x, l1[0].y, l1[1].x, l1[1].y, 0);
			if(ray_spread[nhit]>xend||l1[0].y<=l1[1].y)//if ray focuses beyond xend, or doesn't focus
			{
				double y=extrapolate_x(l1[0].x, l1[0].y, l1[1].x, l1[1].y, xend);
				lines_insert(l1[0].x, l1[0].y, xend, y);
			}
			else
				lines_insert(l1[0].x, l1[0].y, ray_spread[nhit], 0);
			++nhit;
		}
	}
	ray_spread_mean=0;
	double variance=-1;
	meanvar(ray_spread, nhit, &ray_spread_mean, &variance);
	for(int k=0;k<nhit;++k)
		ray_spread[k]-=ray_spread_mean;
	//free(ray_spread);
	variance=sqrt(variance);
	if(history_enabled)
	{
		history[hist_idx]=variance;
		++hist_idx, hist_idx%=w;
	}
	spread=variance;
}
void			eval3(GlassElem *elements, int count, double *ridx, int *ray_idx, int ridx_count, double aperture, double xend)
{
	Point l1[2], l2[2];
	int hit;
	GlassElem *ge;

	ecount=count;
	ray_spread=(double*)realloc(ray_spread, nrays*ridx_count*sizeof(double));
	lines=(Line*)realloc(lines, ((count*2+2)*nrays*ridx_count)*sizeof(Line));
	nlines=0;
	nhit=0;
	for(int kn=0;kn<ridx_count;++kn)
	{
		ray_idx[kn]=nlines;
		//for(int kr=nrays-1;kr>=0;--kr)
		for(int kr=0;kr<nrays;++kr)
		{
			l1[0].x=-10, l1[0].y=aperture*(kr+1)/(nrays+1)*0.5, l1[1].x=0, l1[1].y=l1[0].y;
		//	lines_insert(l1[0].x, l1[0].y, l1[1].x, l1[1].y);
			double x=0;
			for(int k2=0;k2<count;++k2)
			{
				ge=elements+k2;
				x+=ge->dist;
				if(ge->active)
				{
					hit=refract_ray_surface(l1[0].x, l1[0].y, l1[1].x, l1[1].y, aperture, 1, ridx[kn], x, ge->Rl, l2);
					if(!hit)
					{
						lines_insert(l1[0].x, l1[0].y, l1[1].x, l1[1].y);
						break;
					}
					lines_insert(l1[0].x, l1[0].y, l2[0].x, l2[0].y);
					if(hit==2)
					{
						lines_insert(l2[0].x, l2[0].y, l2[1].x, l2[1].y);
						break;
					}
				}
				x+=ge->th;
				if(ge->active)
				{
					hit=refract_ray_surface(l2[0].x, l2[0].y, l2[1].x, l2[1].y, aperture, ridx[kn], 1, x, -ge->Rr, l1);
					if(!hit)
					{
						lines_insert(l2[0].x, l2[0].y, l2[1].x, l2[1].y);
						break;
					}
					lines_insert(l2[0].x, l2[0].y, l1[0].x, l1[0].y);
					if(hit==2)
					{
						lines_insert(l1[0].x, l1[0].y, l1[1].x, l1[1].y);
						break;
					}
				}
			}
			if(hit==1)
			{
				ray_spread[nhit]=extrapolate_y(l1[0].x, l1[0].y, l1[1].x, l1[1].y, 0);
				if(ray_spread[nhit]>xend||l1[0].y<=l1[1].y)//if ray focuses beyond xend, or doesn't focus
				{
					double y=extrapolate_x(l1[0].x, l1[0].y, l1[1].x, l1[1].y, xend);
					lines_insert(l1[0].x, l1[0].y, xend, y);
				}
				else
					lines_insert(l1[0].x, l1[0].y, ray_spread[nhit], 0);
				++nhit;
			}
		}
	}
	ray_idx[ridx_count]=nlines;
	ray_spread_mean=0;
	double variance=-1;
	meanvar(ray_spread, nhit, &ray_spread_mean, &variance);
	for(int kr=0;kr<nhit;++kr)
		ray_spread[kr]-=ray_spread_mean;
	//free(ray_spread);
	variance=sqrt(variance);//standard deviation
	if(history_enabled)
	{
		history[hist_idx]=variance;
		++hist_idx, hist_idx%=w;
	}
	spread=variance;
}
#ifdef WAVELENGTH_IS_VARIABLE
	#define	EVAL()	eval2(elements, ecount, n_float, ap, 300)
#else
	#define	EVAL()	eval3(elements, ecount, r_idx, ray_idx, ridx_count, ap, 300)
#endif
//	#define	EVAL()	spread=eval(ap, nrays, n_float, R1, t1, xdist, R2a, t2, R2b, xCCD, xCut)
double			calc_loss()
{
	double focal_length=100;
	focal_length-=ray_spread_mean;
	return spread+focal_length*focal_length;
}
void			change_diopter(double *r, double delta)
{
	if(*r)
		*r=1/(1 / *r+delta);
	else
		*r=1/delta;
}
void			calc_grad(double *grad, double step, int *var_idx, int nvars)
{
	history_enabled=0;
	memset(grad, 0, ecount*4*sizeof(double));
	for(int ke=1;ke<ecount;++ke)//exclude first glass element
	{
		GlassElem *ge=elements+ke;
		if(ge->active)
		{
			for(int kv=0;kv<nvars;++kv)
			{
				ge->vars[var_idx[kv]]+=step;
				EVAL();
				ge->vars[var_idx[kv]]-=step;
				grad[nvars*ke+var_idx[kv]]=calc_loss();
			}
		}
	}
	EVAL();
	double loss=calc_loss();
	for(int ke=1;ke<ecount;++ke)
	{
		if(elements[ke].active)
		{
			for(int kv=0;kv<nvars;++kv)
				grad[var_idx[kv]]-=loss;
		}
	}
	history_enabled=1;
}
void			update_params(double *grad, int *var_idx, int nvars)
{
	for(int ke=1;ke<ecount;++ke)//bypass first lens
	{
		GlassElem *ge=elements+ke;
		if(ge->active)
		{
			for(int kv=0;kv<nvars;++kv)
				ge->vars[var_idx[kv]]+=grad[ecount*ke+var_idx[kv]];
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
#define			real2screenX(XCOMP)	(int)floor(((XCOMP)-Xstart)*Xr)
#define			real2screenY(YCOMP)	(int)floor((Yend-(YCOMP))*Yr)
void			draw_line(double x1, double y1, double x2, double y2, double Xstart, double Xr, double Yend, double Yr)
{
	MoveToEx(ghMemDC, real2screenX(x1), real2screenY(y1), 0);
	LineTo(ghMemDC, real2screenX(x2), real2screenY(y2));
}
double			draw_arc(double x, double R, double ap, double Xstart, double Xr, double Yend, double Yr, int nsegments)//returns top x
{
	double r2=R*R;
	int x1, y1, x2, y2;
	if(!R)
	{
		x1=real2screenX(x);
		y1=real2screenY(-ap*0.5), y2=real2screenY(ap*0.5);
		MoveToEx(ghMemDC, x1, y1, 0);
		LineTo(ghMemDC, x1, y2);
		return x;
	}
	double ty, tx;
	for(int k=-nsegments;k<=nsegments;++k)
	{
		ty=ap*k/nsegments*0.5, tx=x+R-sgn_star(R)*sqrt(r2-ty*ty);
		x2=real2screenX(tx), y2=real2screenY(ty);
		if(k>-nsegments)
		{
			MoveToEx(ghMemDC, x1, y1, 0);
			LineTo(ghMemDC, x2, y2);
		}
		x1=x2, y1=y2;
	}
	return tx;
}
int				osc_color(int counter, int totalcount)
{
	unsigned char r, g, b;
	r=0;
	g=256*counter/totalcount;
	b=0;

	//double f=2*M_PI/totalcount;
	//unsigned char
	//	r=(unsigned char)(255*cos(f*counter)),
	//	g=(unsigned char)(255*cos(f*counter+M_PI*2/3)),
	//	b=(unsigned char)(255*cos(f*counter+M_PI*4/3));

	return b<<16|g<<8|r;//OpenGL & WinAPI
//	return r<<16|g<<8|b;//DIB
}
/*double			gauss(double x){return exp(-x*x);}
double			wavelength2color(double lambda, int idx)
{
	double
		//		R	G	B
		shift[]={560, 535, 450},//nm
		coeff[]={1./45, 1./45, 1./30};
	return gauss(coeff[idx]*(lambda-shift[idx]));
}
int				wavelength2rgb(double lambda)
{
	unsigned char
		r=(unsigned char)(255*wavelength2color(lambda, 0)),
		g=(unsigned char)(255*wavelength2color(lambda, 1)),
		b=(unsigned char)(255*wavelength2color(lambda, 2));
	return b<<16|g<<8|r;//OpenGL & WinAPI
//	return r<<16|g<<8|b;//DIB
}//*/
void			render()
{
	memset(rgb, 0xFF, rgbn*sizeof(int));
	
	//Rectangle(ghMemDC, rand()%R.right, rand()%R.bottom, rand()%R.right, rand()%R.bottom);
	double DY=DX*h/(w*AR_Y);
	if(h)
	{
		double Xr=w/DX, Yr=h/DY;//unity in pixels
		double Ystart=VY-DY/2, Yend=VY+DY/2, Xstart=VX-DX/2, Xend=VX+DX/2;
		if(!clearScreen)
		{
			hPen=(HPEN)SelectObject(ghMemDC, hPen), hBrush=(HBRUSH)SelectObject(ghMemDC, hBrush);
			double Xstepx2=Xstep*2, Ystepx2=Ystep*2;
			for(double y=ceil(Yend/Ystepx2)*Ystepx2, yEnd=floor(Ystart/Ystep)*Ystep-Ystepx2;y>yEnd;y-=Ystepx2)
			{
				for(double x=floor(Xstart/Xstepx2)*Xstepx2, xEnd=ceil (Xend/Xstep)*Xstep;x<xEnd;x+=Xstepx2)
				{
					double
						ax1=(x		-Xstart)*Xr, ay1=(Yend-y		)*Yr, ax2=(x+Xstep	-Xstart)*Xr, ay2=(Yend-y-Ystep	)*Yr,
						bx1=(x+Xstep-Xstart)*Xr, by1=(Yend-y-Ystep	)*Yr, bx2=(x+Xstepx2-Xstart)*Xr, by2=(Yend-y-Ystepx2)*Yr;
					Rectangle(ghMemDC, (int)(ax1)-(ax1<0), (int)(ay1)-(ay1<0)+1, (int)(ax2)-(ax2<0), (int)(ay2)-(ay2<0)+1);
					Rectangle(ghMemDC, (int)(bx1)-(bx1<0), (int)(by1)-(by1<0)+1, (int)(bx2)-(bx2<0), (int)(by2)-(by2<0)+1);
				}
			}
			hPen=(HPEN)SelectObject(ghMemDC, hPen), hBrush=(HBRUSH)SelectObject(ghMemDC, hBrush);
		}
#ifdef WAVELENGTH_IS_VARIABLE
		HPEN hPenRay=CreatePen(PS_SOLID, 1, wavelength2rgb(lambda));
		hPenRay=(HPEN)SelectObject(ghMemDC, hPenRay);
		for(int kl=0;kl<nlines;++kl)
		{
			Line *ln=lines+kl;
			MoveToEx(ghMemDC, real2screenX(ln->x1), real2screenY(ln->y1), 0);
			LineTo(ghMemDC, real2screenX(ln->x2), real2screenY(ln->y2));
		}
		hPenRay=(HPEN)SelectObject(ghMemDC, hPenRay);
		DeleteObject(hPenRay);
#else
		for(int kv=0;kv<ridx_count;++kv)
		{
			double wavelength=r_idx2wavelength(r_idx[kv]);
			int color=wavelength2rgb(wavelength);
			HPEN hPenRay=CreatePen(PS_SOLID, 1, color);
			hPenRay=(HPEN)SelectObject(ghMemDC, hPenRay);
			for(int kl=ray_idx[kv];kl<ray_idx[kv+1];++kl)
			{
				Line *ln=lines+kl;
				MoveToEx(ghMemDC, real2screenX(ln->x1), real2screenY(ln->y1), 0);
				LineTo(ghMemDC, real2screenX(ln->x2), real2screenY(ln->y2));
			}
			hPenRay=(HPEN)SelectObject(ghMemDC, hPenRay);
			DeleteObject(hPenRay);
		}
#endif

		hPenLens=(HPEN)SelectObject(ghMemDC, hPenLens);
		int segments=25;
		double x=0, yreach=ap*0.5, topx1=0, topx2=0;
		for(int k=0;k<ecount;++k)
		{
			GlassElem *ge=elements+k;
			x+=ge->dist;
			if(ge->active)
				topx1=draw_arc(x, ge->Rl, ap, Xstart, Xr, Yend, Yr, segments);
			x+=ge->th;
			if(ge->active)
			{
				topx2=draw_arc(x, -ge->Rr, ap, Xstart, Xr, Yend, Yr, segments);
				draw_line(topx1, yreach, topx2, yreach, Xstart, Xr, Yend, Yr);
				draw_line(topx1, -yreach, topx2, -yreach, Xstart, Xr, Yend, Yr);
			}
		}
	/*	double topx1, topx2, yreach=ap*0.5;
		topx1=draw_arc(x, R1,		ap, Xstart, Xr, Yend, Yr, segments);
		x+=t1;
		topx2=draw_arc(x, 0,		ap, Xstart, Xr, Yend, Yr, segments);
		draw_line(topx1, yreach, topx2, yreach, Xstart, Xr, Yend, Yr);
		draw_line(topx1, -yreach, topx2, -yreach, Xstart, Xr, Yend, Yr);

		if(correctorOn)
		{
			x+=xdist;
			topx1=draw_arc(x, R2a,	ap, Xstart, Xr, Yend, Yr, segments);
			x+=t2;
			topx2=draw_arc(x, -R2b,	ap, Xstart, Xr, Yend, Yr, segments);
			draw_line(topx1, yreach, topx2, yreach, Xstart, Xr, Yend, Yr);
			draw_line(topx1, -yreach, topx2, -yreach, Xstart, Xr, Yend, Yr);
		}//*/
		hPenLens=(HPEN)SelectObject(ghMemDC, hPenLens);

		if(ray_spread)//draw spread
		{
			hBrushHollow=(HBRUSH)SelectObject(ghMemDC, hBrushHollow);
			int xcenter=real2screenX(ray_spread_mean), ycenter=real2screenY(0);
			for(int k=0;k<nrays;++k)
			{
				int r=(int)(ray_spread[k]*Xr);
				HPEN tempPen=(HPEN)CreatePen(PS_SOLID, 1, osc_color(k, nrays));
				tempPen=(HPEN)SelectObject(ghMemDC, tempPen);
				Ellipse(ghMemDC, xcenter-r, ycenter-r, xcenter+r, ycenter+r);
				tempPen=(HPEN)SelectObject(ghMemDC, tempPen);
				DeleteObject(tempPen);
			}
			hBrushHollow=(HBRUSH)SelectObject(ghMemDC, hBrushHollow);
		}

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

			int y=0;
			GUITPrint(ghMemDC, 0, y, 0, "Crtl +/-\t\tchange wavelength"), y+=16;
			GUITPrint(ghMemDC, 0, y, 0, "T\t\tcenter at focus"), y+=16;
			GUITPrint(ghMemDC, 0, y, 0, "tab\t\tselect glass element"), y+=16;
			GUITPrint(ghMemDC, 0, y, 0, "space\t\ttoggle current glass element"), y+=16;
			GUITPrint(ghMemDC, 0, y, 0, "1+left/right\tchange distance"), y+=16;
			GUITPrint(ghMemDC, 0, y, 0, "2+left/right\tchange left radius"), y+=16;
			GUITPrint(ghMemDC, 0, y, 0, "3+left/right\tchange thickness"), y+=16;
			GUITPrint(ghMemDC, 0, y, 0, "4+left/right\tchange right radius"), y+=16;
			GUITPrint(ghMemDC, 0, y, 0, "F\t\tflip glass element"), y+=16;
			GUITPrint(ghMemDC, 0, y, 0, "O\t\toptimize"), y+=32;

			GUITPrint(ghMemDC, 0, y, 0, "\t1 dist\t2 Rl\t3 Th\t4 Rr"), y+=16;
			for(int k=0;k<ecount;++k)
				GUITPrint(ghMemDC, 0, y, 0, "%c%c: %c\t%g\t%g\t%g\t%g\t%s", current_elem==k?'>':' ', 'A'+k, elements[k].active?'V':'X', elements[k].dist, elements[k].Rl, elements[k].th, elements[k].Rr, current_elem==k?"<-":""), y+=16;
#ifdef WAVELENGTH_IS_VARIABLE
			GUIPrint(ghMemDC, w>>3, h*3>>2, "wavelength %lfnm, n %lf, F %lfcm, Std.Dev %lf mm", lambda, n_float, ray_spread_mean, 10*spread);
#else
			GUIPrint(ghMemDC, w>>3, h*3>>2, "F %lfcm, Std.Dev %g mm", ray_spread_mean, 10*spread);
#endif

		/*	GUIPrint(ghMemDC, 0, y, "Space: Toggle corrector"), y+=16;
			GUITPrint(ghMemDC, 0, y, 0, "1: R2a\t%lf (Ctrl 1 To negate)", R2a), y+=16;
			GUITPrint(ghMemDC, 0, y, 0, "2: R2b\t%lf", R2b), y+=16;
			GUITPrint(ghMemDC, 0, y, 0, "3: thickness\t%lf", t2), y+=16;
			GUITPrint(ghMemDC, 0, y, 0, "4: distance\t%lf", xdist), y+=16;
		//	GUITPrint(ghMemDC, 0, y, 0, "5: xCCD"), y+=16;
			GUIPrint(ghMemDC, 0, y, "Shift R: Reset settings"), y+=16;
			GUIPrint(ghMemDC, 0, y, "Ctrl Wheel: Change ray count");

			GUIPrint(ghMemDC, w>>2, h>>2, "Std.Dev %lf mm", 10*spread);//*/
		}
	}
	
	QueryPerformanceCounter(&li);
	GUIPrint(ghMemDC, 0, h-16, "fps=%.10f, T=%.10fms", freq/(double)(li.QuadPart-nticks), 1000.*(li.QuadPart-nticks)/freq);
//	linelen=sprintf_s(line, 128, "fps=%.10f, T=%.10fms", freq/double(li.QuadPart-nticks), 1000.*(li.QuadPart-nticks)/freq);
//	if(linelen>0)TextOutA(ghMemDC, 0, h-16, line, linelen);
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
			GlassElem *ge=elements+current_elem;
			if(kb['1']||kb[VK_NUMPAD1])//R2a
			{
				if(kb[VK_LEFT	]&&ge->active)	ge->dist-=0.9*dx, e=1;
				if(kb[VK_RIGHT	]&&ge->active)	ge->dist+=0.9*dx, e=1;
			}
			else if(kb['2']||kb[VK_NUMPAD2])//R2b
			{
				if(kb[VK_LEFT	]&&ge->active)	change_diopter(&ge->Rl, -0.01*dx), e=1;
				if(kb[VK_RIGHT	]&&ge->active)	change_diopter(&ge->Rl, 0.01*dx), e=1;
			}
			else if(kb['3']||kb[VK_NUMPAD3])//t2
			{
				if(kb[VK_LEFT	]&&ge->active)	ge->th-=1.2*dx, e=1;
				if(kb[VK_RIGHT	]&&ge->active)	ge->th+=1.2*dx, e=1;
			}
			else if(kb['4']||kb[VK_NUMPAD4])//separation
			{
				if(kb[VK_LEFT	]&&ge->active)	change_diopter(&ge->Rr, 0.01*dx), e=1;
				if(kb[VK_RIGHT	]&&ge->active)	change_diopter(&ge->Rr, -0.01*dx), e=1;
			}
		/*	if(kb['1']||kb[VK_NUMPAD1])//R2a
			{
				if(kb[VK_LEFT	])	R2a-=2*dx, e=1;
				if(kb[VK_RIGHT	])	R2a+=2*dx, e=1;
			}
			else if(kb['2']||kb[VK_NUMPAD2])//R2b
			{
				if(kb[VK_LEFT	])	R2b-=2*dx, e=1;
				if(kb[VK_RIGHT	])	R2b+=2*dx, e=1;
			}
			else if(kb['3']||kb[VK_NUMPAD3])//t2
			{
				if(kb[VK_LEFT	])	t2-=1.2*dx, e=1;
				if(kb[VK_RIGHT	])	t2+=1.2*dx, e=1;
			}
			else if(kb['4']||kb[VK_NUMPAD4])//separation
			{
				if(kb[VK_LEFT	])	xdist-=0.9*dx, e=1;
				if(kb[VK_RIGHT	])	xdist+=0.9*dx, e=1;
			}
			else if(kb['5']||kb[VK_NUMPAD5])//separation
			{
				if(kb[VK_LEFT	])	xCCD-=dx, e=1;
				if(kb[VK_RIGHT	])	xCCD+=dx, e=1;
			}//*/
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
#ifdef WAVELENGTH_IS_VARIABLE
				if(kb[VK_CONTROL])
				{
					if(lambda<750)
						lambda+=1, n_float=wavelength2refractive_index(lambda), e=1;
				}
				else
#endif
				if(kb['X'])	DX/=1.05, AR_Y/=1.05;
				else if(kb['Y'])	AR_Y*=1.05;
				else				DX/=1.05;
				function1();
			}
			if(kb[VK_SUBTRACT	]||kb[VK_BACK	]||kb[VK_OEM_MINUS	])
			{
#ifdef WAVELENGTH_IS_VARIABLE
				if(kb[VK_CONTROL])
				{
					if(lambda>380)
						lambda-=1, n_float=wavelength2refractive_index(lambda), e=1;
				}
				else
#endif
				if(kb['X'])	DX*=1.05, AR_Y*=1.05;
				else if(kb['Y'])	AR_Y/=1.05;
				else				DX*=1.05;
				function1();
			}
			if(e)
				EVAL();
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
				EVAL();
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
		case VK_SPACE:
			elements[current_elem].active=!elements[current_elem].active;
		//	correctorOn=!correctorOn;
			EVAL();
			break;
		case VK_TAB:
			current_elem=mod(current_elem+!kb[VK_SHIFT]-kb[VK_SHIFT], ecount);
			break;
		case 'O'://optimize
			{
				double grad[16];
				static double velocity[16]={0};
				int var_idx[]={1, 3};//only optimize left & right radii
				int nvars=SIZEOF(var_idx);
				calc_grad(grad, 0.01, var_idx, nvars);
				for(int k=0;k<16;++k)
				{
					if(grad[k])
						velocity[k]=velocity[k]*0.5-0.0001*grad[k];
				}
				update_params(velocity, var_idx, nvars);
				EVAL();
			}
			break;
	/*	case '1':
			if(kb[VK_CONTROL])
				R2a=-R2a;
			EVAL();
			break;
		case '2':
			if(kb[VK_CONTROL])
				R2b=-R2b;
			EVAL();
			break;//*/
		case 'C':
			clearScreen=!clearScreen;
			break;
		case 'H':
			for(int k=0;k<w;++k)
				history[k]=-1;
			//memset(history, 0, w*sizeof(double));
			break;
		case 'P':
			{
				int e=0;
				if(kb['2'])
					e=1, elements[current_elem].Rl=0;
				if(kb['4'])
					e=1, elements[current_elem].Rr=0;
				if(e)
					EVAL();
			}
			break;
		case 'T':
			VX=ray_spread_mean, VY=0;
			break;
		case 'F':
			{
				GlassElem *ge=elements+current_elem;
				if(ge->active)
				{
					double temp;
					temp=ge->Rl, ge->Rl=ge->Rr, ge->Rr=temp;
					EVAL();
				}
			}
			break;
		case 'R':
			{
				int e=0;
				if(kb[VK_SHIFT])
				{
					e=1;
					for(int k=1;k<ecount;++k)//exclude first element
					{
						GlassElem *ge=elements+k;
						ge->dist=k?4:0;
						ge->Rl=100;
						ge->th=0.8;
						ge->Rr=100;
					}
				}
				else if(kb['1'])
					e=1, elements[current_elem].dist=current_elem?4:0;
				else if(kb['2'])
					e=1, elements[current_elem].Rl=100;
				else if(kb['3'])
					e=1, elements[current_elem].th=0.8;
				else if(kb['4'])
					e=1, elements[current_elem].Rr=100;
				if(e)
					EVAL();
				else
				{
					if(!kb[VK_CONTROL])
						DX=20, AR_Y=1, function1();
					VX=VY=0;
				}
			}
			//if(kb[VK_SHIFT])
			//{
			//	xdist=5, R2a=50, t2=2, R2b=-50;
			//	EVAL();
			//}
			break;
		case 'E':
			if(kb[VK_CONTROL])
				DX=20;
			else
				DX/=sqrt(AR_Y);//average zoom
		//	DX/=sqrt(AR_Y);
		//	if(kb[VK_CONTROL])
		//		DX=20;
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
	hPen=CreatePen(PS_SOLID, 1, _2dCheckColor);
	hBrush=CreateSolidBrush(_2dCheckColor);
	hPenLens=CreatePen(PS_SOLID, 1, 0x00FF00FF);
	hBrushHollow=(HBRUSH)GetStockObject(NULL_BRUSH);
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
	RegisterClassExA(&wndClassEx);
	ghWnd=CreateWindowExA(0, wndClassEx.lpszClassName, "Lens Designer", WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_CLIPCHILDREN, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 0, 0, hInstance, 0);//20220504
	
		wnd_resize();
		function1();
		EVAL();

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