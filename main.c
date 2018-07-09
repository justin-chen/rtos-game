#include <lpc17xx.h>
#include <stdbool.h>
#include "GLCD.h"
#include <RTL.h>
#include <stdlib.h>
#include <stdio.h>

OS_SEM sem_reveal, sem_fill;
OS_TID tsk_read_js, tsk_reveal_board, tsk_incr_timer, tsk_signal_flag, tsk_flag;

int n[9][9] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0}};

int revealed[9][9] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0}};

int flags[9][9] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0}};

int x = 0, y = 0;
int flag_count = 10;
int num_revealed = 0;
int bits[8] = {28, 29, 31, 2, 3, 4, 5, 6};
bool game_over = false;

void fill(int x, int y)
{
    unsigned char num_mines[1];
    if (x >= 0 && x < 9 && y >= 0 && y < 9 && revealed[x][y] == 0)
    {
        revealed[x][y] = 1;
        num_revealed += 1;

        if (flags[x][y] == 1)
        {
            flags[x][y] = 0;
            flag_count += 1;
        }
        if (n[x][y] == 0)
        {
            GLCD_DisplayString(x * 3 + 2, y * 4 + 2, 0, ".");
            fill(x + 1, y);
            fill(x - 1, y);
            fill(x + 1, y + 1);
            fill(x - 1, y - 1);
            fill(x - 1, y + 1);
            fill(x + 1, y - 1);
            fill(x, y + 1);
            fill(x, y - 1);
        }
        else
        {
            sprintf(num_mines, "%d", n[x][y]);
            GLCD_DisplayString(x * 3 + 2, y * 4 + 2, 0, num_mines);
        }
    }
}

__task void reveal()
{
    unsigned char num_mines[1];
    int i, j;
    while (true)
    {
        os_sem_wait(&sem_reveal, 0xffff);
        if (n[x][y] == -1 && flags[x][y] == 0)
        { //mine hit, reveal all mines and end game
            for (i = 0; i < 9; i++)
            {
                for (j = 0; j < 9; j++)
                {
                    if (n[i][j] == -1)
                    {
                        GLCD_DisplayString(i * 3 + 2, j * 4 + 2, 0, "X");
                    }
                }
            }
            game_over = true;
            GLCD_DisplayString(3, 16, 1, ":^(");
        }
        else if (n[x][y] == 0)
        { //no surrounding mines
            fill(x, y);
            os_sem_send(&sem_fill);
        }
        else
        { //1 to 8 surrounding mines
            if (flags[x][y] == 0 && revealed[x][y] == 0)
            {
                sprintf(num_mines, "%d", n[x][y]);
                GLCD_DisplayString(x * 3 + 2, y * 4 + 2, 0, num_mines);
                revealed[x][y] = 1;
                num_revealed += 1;
            }
        }
        if (num_revealed == 71 && flag_count == 0)
        {
            game_over = true;
            GLCD_DisplayString(3, 16, 1, ":^)");
        }
        os_tsk_pass();
    }
}

__task void displayTimer()
{
    int c, d, t = 0;
    unsigned char time[3];
    GLCD_DisplayString(1, 16, 1, "000");
    while (true)
    {
        for (c = 1; c <= 4000; c++)
        {
            for (d = 1; d <= 4500; d++)
            {
            }
        }
        t += 1;
        if (t % 1000 < 10)
            sprintf(time, "00%d", t % 1000);
        else if (t % 1000 < 100)
            sprintf(time, "0%d", t % 1000);
        else
            sprintf(time, "%d", t % 1000);
        GLCD_DisplayString(1, 16, 1, time);
        os_tsk_pass();
    }
}

__task void signalFlag()
{
    int button;
    int prev = 0;
    while (true)
    {
        button = LPC_GPIO2->FIOPIN; //read button value
        if (!(button & 1 << 10))
        {                  //if button is pressed
            if (prev == 0) //check if button is not held
            {
                os_sem_send(&sem_fill);
                prev = 1;
            }
        }
        else
        {
            prev = 0;
        }
        os_tsk_pass();
    }
}

__task void flag()
{
    int mask;
    int i;
    while (true)
    {
        os_sem_wait(&sem_fill, 0xffff);
        if (revealed[x][y] == 0 && flags[x][y] == 0 && flag_count > 0)
        {
            GLCD_DisplayString(x * 3 + 2, y * 4 + 2, 0, "F");
            flags[x][y] = 1;
            flag_count -= 1;
        }
        else if (revealed[x][y] == 0 && flags[x][y] == 1)
        {
            GLCD_DisplayString(x * 3 + 2, y * 4 + 2, 0, " ");
            flags[x][y] = 0;
            flag_count += 1;
        }
        for (i = 0; i < 4; i++)
        {
            mask = 1 << i;
            LPC_GPIO1->FIOCLR |= 1 << bits[i];
            LPC_GPIO2->FIOCLR |= 1 << bits[i];
            if ((flag_count & mask))
            {
                if (i < 3)
                    LPC_GPIO1->FIOSET |= 1 << bits[i];
                else
                    LPC_GPIO2->FIOSET |= 1 << bits[i];
            }
        }
        if (num_revealed == 71 && flag_count == 0)
        {
            game_over = true;
            GLCD_DisplayString(3, 16, 1, ":^)");
        }
        os_tsk_pass();
    }
}

__task void readJoystick()
{
    int button;
    int prev = 0;
    GLCD_DisplayString(x * 3 + 1, y + 1, 0, "*");

    while (true)
    {
        button = LPC_GPIO1->FIOPIN;
        if (!(button & 1 << 24) || !(button & 1 << 26) || !(button & 1 << 23) || !(button & 1 << 25) || !(button & 1 << 20))
        {
            if (prev == 0)
            {
                GLCD_DisplayString(3 * x + 1, y * 4 + 1, 0, " ");
                if (!(button & 1 << 24))
                { //RIGHT
                    if (y < 8)
                        y += 1;
                    prev = 1;
                }
                else if (!(button & 1 << 26))
                { //LEFT
                    if (y > 0)
                        y -= 1;
                    prev = 1;
                }
                else if (!(button & 1 << 23))
                { //UP
                    if (x > 0)
                        x -= 1;
                    prev = 1;
                }
                else if (!(button & 1 << 25))
                { //DOWN
                    if (x < 8)
                        x += 1;
                    prev = 1;
                }
                else if (!(button & 1 << 20))
                { //PRESS
                    os_sem_send(&sem_reveal);
                    prev = 1;
                }
                GLCD_DisplayString(3 * x + 1, y * 4 + 1, 0, "*");
            }
        }
        else
        {
            prev = 0;
        }
        os_tsk_pass();
    }
}

__task void createTasks()
{
    os_sem_init(&sem_reveal, 0);
    os_sem_init(&sem_fill, 0);
    tsk_read_js = os_tsk_create(readJoystick, 1);
    tsk_reveal_board = os_tsk_create(reveal, 1);
    tsk_incr_timer = os_tsk_create(displayTimer, 1);
    tsk_signal_flag = os_tsk_create(signalFlag, 1);
    tsk_flag = os_tsk_create(flag, 1);

    while (true)
    {
        if (game_over)
        {
            os_tsk_delete(tsk_read_js);
            os_tsk_delete(tsk_reveal_board);
            os_tsk_delete(tsk_incr_timer);
            os_tsk_delete(tsk_signal_flag);
            os_tsk_delete(tsk_flag);
        }
        os_tsk_pass();
    }
}

void displayBoard()
{
    int i, j, mask;
    GLCD_Init();
    GLCD_Clear(Blue);
    GLCD_SetBackColor(Blue);
    GLCD_SetTextColor(White);
    for (i = 0; i < 216; i++)
    {
        for (j = 0; j < 216; j++)
        {
            GLCD_PutPixel(5 + i, 7 + j);
        }
    }
    GLCD_SetTextColor(Black);
    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < 218; j++)
        {
            GLCD_PutPixel((i + 1) * 24 + 5, 6 + j); //draw columns
            GLCD_PutPixel(5 + j, (i + 1) * 24 + 7); //draw rows
        }

        mask = 1 << i;
        if ((flag_count & mask))
        {
            if (i < 3)
                LPC_GPIO1->FIOSET |= 1 << bits[i];
            else
                LPC_GPIO2->FIOSET |= 1 << bits[i];
        }
    }
    GLCD_SetBackColor(White);
    GLCD_SetTextColor(Black);
}

void generateValues()
{
    int i, j, k;
    int xc[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    int yc[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int nx, ny;
    srand(1);
    for (k = 0; k < 10; k++)
    {
        i = rand() % 9;
        j = rand() % 9;
        if (n[i][j] != -1)
            n[i][j] = -1;
        else
            k--;
    }
    for (i = 0; i < 9; i++)
    {
        for (j = 0; j < 9; j++)
        {
            if (n[i][j] != -1)
            {
                for (k = 0; k < 8; k++)
                {
                    nx = i + xc[k];
                    ny = j + yc[k];
                    if (nx >= 0 && nx < 9 && ny >= 0 && ny < 9)
                    {
                        if (n[nx][ny] == -1)
                        {
                            n[i][j] += 1;
                        }
                    }
                }
            }
        }
    }
}

int main(void)
{
    LPC_GPIO1->FIODIR |= 0xB0000000;
    LPC_GPIO2->FIODIR |= 0x0000007C;
    displayBoard();
    generateValues();
    printf("%s\n", "Init UART printf()");
    os_sys_init(createTasks);
}