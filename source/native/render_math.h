#pragma once
#include <math.h>
static inline int wb_valid_clips(float n,float f){return isfinite(n)&&isfinite(f)&&n>0&&f>n;}
static inline float wb_linear_depth(float d,float n,float f){return 2*n*f/(f+n-(2*d-1)*(f-n));}
static inline int wb_depth_visible(float md,float hd,float mn,float mf,float hn,float hf){
 if(!wb_valid_clips(mn,mf)||!wb_valid_clips(hn,hf)||!isfinite(md)||!isfinite(hd))return 0;
 return hd>=.999999f||wb_linear_depth(md,mn,mf)<=wb_linear_depth(hd,hn,hf)+.03f;
}
