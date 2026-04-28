#include "main.h"
#include <stdio.h>
#include "../Drivers/my_drivers/Inc/BNO086.h"

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    while (1)
    {
        printf("alive\r\n");
        HAL_Delay(1000);
    }
}
