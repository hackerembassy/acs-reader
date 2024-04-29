#ifndef DIAL_H_
#define DIAL_H_

void InitDial(int interrupt, int pulse);

void pincode_handler_task(void *pvParameters);

void dial_task(void *pvParameters);

#endif