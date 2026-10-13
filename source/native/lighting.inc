/* Screen-space native-world illumination; depth-based shadows are approximate.
 * This does not pretend to provide material-aware shading or off-screen shadows. */
static void upload_lights(GLuint shader_program,const WbmcControl *pose){
 uint32_t *p=(void*)(t.mem+0x320000);uint32_t seq=__atomic_load_n(p,__ATOMIC_ACQUIRE);
 float records[32][8],positions[8][4],colors[8][4];uint32_t count=0;
 if(seq&&!(seq&1)&&p[1]==1&&p[2]==t.epoch&&p[3]<=32){
  uint32_t n=p[3];memcpy(records,p+16,n*sizeof(records[0]));__atomic_thread_fence(__ATOMIC_ACQUIRE);
  if(seq==__atomic_load_n(p,__ATOMIC_ACQUIRE))for(uint32_t i=0;i<n&&count<8;i++){
   int valid=1;for(int k=0;k<8;k++)valid&=isfinite(records[i][k]);
   if(!valid||records[i][6]<=0||records[i][6]>15)continue;
   positions[count][0]=records[i][0];positions[count][1]=records[i][2];positions[count][2]=records[i][1];positions[count][3]=records[i][6]*.7f+2;
   for(int k=0;k<3;k++)colors[count][k]=fmaxf(0,fminf(1,records[i][3+k]));colors[count][3]=1;count++;
  }
 }
 glUniform1i(glGetUniformLocation(shader_program,"lightCount"),count);
 if(count){glUniform4fv(glGetUniformLocation(shader_program,"lights"),count,(float*)positions);glUniform4fv(glGetUniformLocation(shader_program,"lightColors"),count,(float*)colors);}
 float right[3]={render_dir[1]*render_up[2]-render_dir[2]*render_up[1],render_dir[2]*render_up[0]-render_dir[0]*render_up[2],render_dir[0]*render_up[1]-render_dir[1]*render_up[0]};
 glUniform3fv(glGetUniformLocation(shader_program,"eye"),1,render_pos);glUniform3fv(glGetUniformLocation(shader_program,"forwardDir"),1,render_dir);
 glUniform3fv(glGetUniformLocation(shader_program,"upDir"),1,render_up);glUniform3fv(glGetUniformLocation(shader_program,"rightDir"),1,right);
 glUniform2f(glGetUniformLocation(shader_program,"lens"),tan(host_fov*M_PI/360),(float)win_w/win_h);
}
