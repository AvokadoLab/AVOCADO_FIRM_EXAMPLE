#include "st2_driver.h"
#include "st2_hardware.h"

ST2_DRIVER st2;

static uint8_t step2Value[4][4] = {
  {1, 1, 0, 0},
  {0, 1, 1, 0},
  {0, 0, 1, 1},
  {1, 0, 0, 1}
};

static uint8_t st2_step = 0;
static float st2_dT;
static float st2_currentCnt_float;
static float st2_duty;
static float st2_speed;

void ST2_goNext(bool dir){
  if(dir) st2_step += 1;
  else st2_step += 3;
  st2_step %= 4;
}

void ST2_interPolate(int32_t speed){
  
  uint8_t i;
  float x1, x2;
  float y1, y2;
  
  if(speed < 0) speed = -speed;
  
  for(i = 0; i < TABLE_LEN; i++){
    if(speed < motor_table[i][0]) break;
  }
  
  // Interpolate
  if(i == 0){
    st2_duty = motor_table[0][1];
    return;
  }else if(i >= TABLE_LEN - 1){
    st2_duty = motor_table[TABLE_LEN - 1][1];
    return;
  }
  
  x1 = motor_table[i-1][0];
  x2 = motor_table[i][0];
  y1 = motor_table[i-1][1];
  y2 = motor_table[i][1];
  
  st2_duty = (y2 - y1) / (x2 - x1) * (speed - x1) + y1;
}

void ST2_Init(uint16_t freq){
  st2_dT = 1.0f / (float)freq;
  st2_currentCnt_float = 0.0f;
}

void ST2_Loop(){ // Should be called periodically
  
  static int32_t cnt_prev;
  
  st2_speed = ((float)st2.input_cnt - st2_currentCnt_float) / st2_dT;
  
  // speed validation
  if(st2.speed < 0) st2.speed = -st2.speed;
  if(motor_table[TABLE_LEN - 1][0] < st2.speed) st2.speed = (int32_t)motor_table[TABLE_LEN - 1][0];
  
  if(st2_speed > st2.speed) st2_speed = st2.speed;
  else if(st2_speed < -st2.speed) st2_speed = -st2.speed;
  
  st2_currentCnt_float += st2_speed * st2_dT;
  cnt_prev = st2.current_cnt;
  st2.current_cnt = (int32_t)st2_currentCnt_float;
  
  if(cnt_prev != st2.current_cnt) ST2_goNext(st2_speed >= 0.0f);
}

void ST2_getPWM(float pwmDuty[4]){
  
  uint8_t* pinValue = step2Value[st2_step];
  
  ST2_interPolate(st2_speed); // Calculate proper duty
  
  pwmDuty[0] = pinValue[0] ? st2_duty : 0; // A+
  pwmDuty[1] = pinValue[1] ? st2_duty : 0; // B+
  pwmDuty[2] = pinValue[2] ? st2_duty : 0; // A-
  pwmDuty[3] = pinValue[3] ? st2_duty : 0; // B-
}